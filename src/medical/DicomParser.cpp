#include "medical/DicomParser.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace mi_3d
{
    DicomSlice DicomParser::ParseFile(const std::string& filepath)
    {
        // first create the DicomSlice object to hold the values
        DicomSlice slice; // creates a DicomSlice with all default values (zeros, empty strings)
        std::ifstream file(filepath, std::ios::binary);
        /*
            std::ifstream reads the file at filepath
            std::ios::binary — CRITICAL on Windows
                Without it: Windows translates bytes 0x0D 0x0A (carriage return + newline)
                into just 0x0A. That corrupts binary data.
                With it: reads bytes exactly as they are on disk. No translation.
        */

        if (!file.is_open())
        {
            std::cerr << "Cannot open: " << filepath << std::endl;
            return slice;
            /*
                What gets returned?
                    - The slice object with ALL default values:
                      empty strings, zeros, empty pixelData vector.
                    - The caller checks: if (slice.rows == 0) → parsing failed.
                    - Same pattern as Shader::ReadFile returning "" on failure.
            */
        }

        // ---- DETECT PREAMBLE + DICM ----
        char magic[4];
        file.seekg(128);
        /*
            seekg(128) = "move the cursor to byte 128"
                - ifstream has an internal cursor starting at byte 0
                - seekg jumps it to an absolute position
                - bytes 0-127 are the preamble (usually all zeros, we skip them)
        */
        file.read(magic, 4);
        /*
            reads 4 bytes starting from WHERE THE CURSOR IS (byte 128)
                - so it reads bytes 128, 129, 130, 131
                - cursor is now at byte 132 automatically
                - file.read always reads from current cursor and advances it
        */

        if (magic[0] == 'D' && magic[1] == 'I' && magic[2] == 'C' && magic[3] == 'M')
        {
            // Standard Part 10 file
            // cursor is already at byte 132 — tags start here
            // no seekg needed — file.read already moved us forward
        }
        else
        {
            // No preamble — our generated files start with tags at byte 0
            file.seekg(0);
            // rewind cursor back to the beginning
        }

        // ---- PARSE TAGS IN A LOOP ----
        // BOTH cases (with or without DICM) end up here
        // cursor is at the right position either way
        while (!file.eof()) // this means if the file is not at the end
        {
            uint16_t group = ReadUint16(file);   // 2 bytes → group number
            uint16_t element = ReadUint16(file);   // 2 bytes → element number
            char vr[2];
            file.read(vr, 2);                      // 2 bytes → VR (two ASCII chars like "UI", "DS")
            uint32_t length = ReadUint16(file);   // 2 bytes → how many bytes the value occupies

            /*
                BUT: some VRs use 4-byte length instead of 2-byte
                VR = "OB", "OW", "SQ", "UN" → skip 2 bytes padding, then read 4-byte length
                All other VRs → 2-byte length (what we already read above)
            */
            if ((vr[0] == 'O' && (vr[1] == 'B' || vr[1] == 'W')) ||
                (vr[0] == 'S' && vr[1] == 'Q') ||
                (vr[0] == 'U' && vr[1] == 'N'))
            {
                // the 2 bytes we read as "length" were actually padding — discard them
                length = ReadUint32(file);  // real length is 4 bytes
            }
            // Handle undefined length (0xFFFFFFFF) — skip this tag entirely
            if (length == 0xFFFFFFFF)
            {
                // Sequence with undefined length — we can't skip it by size
                // Scan forward for the sequence delimiter tag (FFFE,E0DD)
                while (!file.eof())
                {
                    uint16_t g = ReadUint16(file);
                    uint16_t e = ReadUint16(file);
                    if (g == 0xFFFE && e == 0xE0DD)
                    {
                        ReadUint32(file); // skip the delimiter's length field (always 0)
                        break;
                    }
                    // Back up 2 bytes — we read element but might need it as next group
                    file.seekg(-2, std::ios::cur);
                }
                continue; // skip to next tag
            }

            // ---- CHECK WHICH TAG THIS IS ----
            /*
                The tag is identified by (group, element) pair
                We compare against the tags we care about
                If it matches → read the value into the right struct field
                If it doesn't → skip forward by [length] bytes
            */
            if (group == 0x0010 && element == 0x0010)
            {
                slice.patientName = ReadString(file, length);
            }
            else if (group == 0x0010 && element == 0x0020)
            {
                slice.patientID = ReadString(file, length);
            }
            else if (group == 0x0028 && element == 0x0010)
            {
                slice.rows = ReadUint16(file);
                /*
                    Why ReadUint16 and not ReadString?
                        - VR for Rows is "US" (Unsigned Short) = binary 2-byte integer
                        - Not ASCII text — actual bytes representing a number
                        - ReadUint16 handles the endian-agnostic conversion
                */
            }
            else if (group == 0x0028 && element == 0x0011)
            {
                slice.columns = ReadUint16(file);
            }
            else if (group == 0x0028 && element == 0x0030)
            {
                // Pixel Spacing — stored as DS (Decimal String) like "0.5\\0.5"
                std::string spacing = ReadString(file, length);
                /*
                    Two values separated by backslash
                    Need to split and convert to float
                */
                size_t backslash = spacing.find('\\');
                if (backslash != std::string::npos)
                {
                    slice.pixelSpaceingX = std::stof(spacing.substr(0, backslash));
                    slice.pixelSpaceingY = std::stof(spacing.substr(backslash + 1));
                }
            }
            else if (group == 0x0018 && element == 0x0050)
            {
                // Slice Thickness — DS (Decimal String) like "2.0"
                std::string thickness = ReadString(file, length);
                slice.sliceThickness = std::stof(thickness);
            }
            else if (group == 0x0020 && element == 0x0032)
            {
                // Image Position Patient — DS like "0.0\\0.0\\50.0" (x\y\z)
                std::string pos = ReadString(file, length);
                size_t first = pos.find('\\');
                size_t second = pos.find('\\', first + 1);
                if (first != std::string::npos && second != std::string::npos)
                {
                    slice.imagePositionX = std::stof(pos.substr(0, first));
                    slice.imagePositionY = std::stof(pos.substr(first + 1, second - first - 1));
                    slice.imagePositionZ = std::stof(pos.substr(second + 1));
                }
            }
            else if (group == 0x0020 && element == 0x0013)
            {
                // Instance Number — IS (Integer String) like "1"
                std::string num = ReadString(file, length);
                slice.instanceNumber = std::stoi(num);
            }
            else if (group == 0x0028 && element == 0x0100)
            {
                slice.bitsAllocated = ReadUint16(file);
            }
            else if (group == 0x0028 && element == 0x0101)
            {
                slice.bitsStored = ReadUint16(file);
            }
            else if (group == 0x0028 && element == 0x0102)
            {
                // High Bit — we read it but don't store it (skip)
                SkipBytes(file, length);
            }
            else if (group == 0x0028 && element == 0x0103)
            {
                slice.pixelRepresentation = ReadUint16(file);
            }
            else if (group == 0x0028 && element == 0x1052)
            {
                std::string intercept = ReadString(file, length);
                slice.rescaleIntercept = std::stof(intercept);
            }
            else if (group == 0x0028 && element == 0x1053)
            {
                std::string slope = ReadString(file, length);
                slice.rescaleSlope = std::stof(slope);
            }
            else if (group == 0x7FE0 && element == 0x0010)
            {
              //  std::cout << "Found pixel data! Length: " << length << std::endl;
                uint32_t pixelCount = length / 2;
                slice.pixelData.resize(pixelCount);
                file.read(reinterpret_cast<char*>(slice.pixelData.data()), length);
                break;
            }
            else
            {
                // tag we don't care about — skip its value
                SkipBytes(file, length);
            }
        }

        return slice;
        /*
            slice now contains:
                - patientName = "TEST^PATIENT"
                - rows = 256, columns = 256
                - pixelSpaceing = 0.5, 0.5
                - sliceThickness = 2.0
                - imagePosition = 0.0, 0.0, Z (depends on which slice)
                - pixelData = 65536 int16_t values (the image)
        */
        
    }

    std::vector<DicomSlice> DicomParser::ParseDirectory(const std::string& directoryPath)
    {
        std::vector<DicomSlice> sliceVec; // creating the vector container for storing the sliced content
        
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (entry.path().extension() == ".dcm") // first it check at the begin what the file type is
            {
                DicomSlice slice = ParseFile(entry.path().string());
                if (slice.rows > 0)
                {
                    sliceVec.push_back(std::move(slice));

                }
            }
        }
        
        std::sort(sliceVec.begin(), sliceVec.end(),
            [](const DicomSlice& a, const DicomSlice& b)
            {
                return a.imagePositionZ < b.imagePositionZ;
            }
        );

      /*  std::cout << "Loaded" << sliceVec.size() << "sliceVec" << std::endl;*/

        if (!sliceVec.empty())
        {
            std::cout <<"First slice Z:"<< sliceVec.front().imagePositionZ << std::endl;
            std::cout << "Last slice Z:" << sliceVec.back().imagePositionZ << std::endl;
        }

        return sliceVec;
    }

    uint16_t DicomParser::ReadUint16(std::ifstream& file)
    {
        uint8_t bytes[2];
        file.read(reinterpret_cast<char*>(bytes), 2);
        return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
    }


    uint32_t DicomParser::ReadUint32(std::ifstream& file)
    {
        uint8_t bytes[4];
        file.read(reinterpret_cast<char*>(bytes), 4);
        return static_cast<uint32_t>(bytes[0])
            | (static_cast<uint32_t>(bytes[1]) << 8)
            | (static_cast<uint32_t>(bytes[2]) << 16)
            | (static_cast<uint32_t>(bytes[3]) << 24);
    }

    std::string DicomParser::ReadString(std::ifstream& file, uint32_t length)
    {
        std::string result(length, '\0');
        file.read(&result[0], length);

        // trim trailing spaces and nulls (DICOM pads strings)
        while (!result.empty() && (result.back() == ' ' || result.back() == '\0'))
        {
            result.pop_back();
        }

        return result;
    }

    void DicomParser::SkipBytes(std::ifstream& file, uint32_t count)
    {
        file.seekg(count, std::ios::cur);
    }



}
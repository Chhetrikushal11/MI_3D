#pragma once
#include <string>
#include <vector>
#include <cstdint> // for uint16_t and uint32_t
#include <fstream> // to read the file

namespace mi_3d
{
	struct DicomSlice
	{
		// for Patient information
		std::string patientName;
		std::string patientID;

		// for Image Dimension
		uint16_t rows = 0;
		uint16_t columns = 0;

		// physical spacing (mm)
		float pixelSpaceingX = 0.0f;
		float pixelSpaceingY = 0.0f;
		float sliceThickness = 0.0f;

		// 3d poisition of this slice
		float imagePositionX = 0.0f;
		float imagePositionY = 0.0f;
		float imagePositionZ = 0.0f;

		// pixel format
		uint16_t bitsAllocated = 0;
		uint16_t bitsStored = 0;
		uint16_t pixelRepresentation = 0; // 0-unsigned, 1- signed

		// Hounsfiled conversation
		float rescaleSlope = 1.0f;
		float rescaleIntercept = 0.0f;

		// slice ordering 
		int instanceNumber = 0;

		// the actual image pixels
		std::vector<int16_t>pixelData;

	};


	class DicomParser
	{
	public:
		DicomSlice ParseFile(const std::string& filepath);
		std::vector<DicomSlice> ParseDirectory(const std::string& directoryPath);
	private:
		uint16_t ReadUint16(std::ifstream& file);
		uint32_t  ReadUint32(std::ifstream& file);
		std::string ReadString(std::ifstream& file, uint32_t length);
		void SkipBytes(std::ifstream& file, uint32_t count);

	};
}
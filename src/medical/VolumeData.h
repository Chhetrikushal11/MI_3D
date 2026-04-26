#pragma once

#include <iostream>
#include <cstdint>
#include <vector>

#include "medical/DicomParser.h"

namespace mi_3d
{
	class VolumeData
	{
	public:
		VolumeData();
		// delete copy constructor and operator
		VolumeData(const VolumeData&) = delete;
		VolumeData& operator=(const VolumeData&) = delete;
		void Build(const std::vector<DicomSlice>& slices);
		uint32_t GetWidth() const { return _mSliceWidth; }
		uint32_t GetHeight() const { return _mSliceHeight; }
		uint32_t GetDepth() const { return _mDepth; }
		float GetSpacingX() const { return _mPixelSpacingX; }
		float GetSpacingY() const { return _mPixelSpacingY; }
		float GetSpacingZ() const { return _mPixelSpacingZ; }
		const std::vector<int16_t>& GetVoxels() const { return _mVoxels; }

	private:
		uint32_t _mSliceWidth; // for columns
		uint32_t _mSliceHeight; // for rows
		float  _mPixelSpacingX; // for every slices
		float  _mPixelSpacingY; // for every slices
		float _mPixelSpacingZ; // for every slices

		uint32_t _mDepth; // for 100 slices

		// to store actual pixel data
		std::vector<int16_t> _mVoxels;
		
	};
}
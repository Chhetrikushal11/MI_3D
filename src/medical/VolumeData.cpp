#include "medical/VolumeData.h"
#include <algorithm> 
namespace mi_3d
{
	VolumeData::VolumeData()
		: _mSliceWidth{ 0 },
		_mSliceHeight{ 0 },
		_mPixelSpacingX{ 0.0f },
		_mPixelSpacingY{ 0.0f },
		_mPixelSpacingZ{ 0.0f },
		_mDepth{0}
	{
		std::cout << "The object of VolumeData is created" << std::endl;
	}
	void VolumeData::Build(const std::vector<DicomSlice>& slices)
	{
		if (slices.empty())
		{
			std::cerr << "VolumeData: No slices to build from" << std::endl;
			return;
		}

		_mSliceWidth = slices[0].columns; // for each file we have at beinging the value of columns and rows
		_mSliceHeight = slices[0].rows;
		_mDepth = static_cast<uint32_t>(slices.size()); // how??
		_mPixelSpacingX = slices[0].pixelSpaceingX;
		_mPixelSpacingY = slices[0].pixelSpaceingY;
		


		// Step 4: calculate Z spacing — how? (hint: two adjacent slices)
		if (slices.size() > 1)
		{
			_mPixelSpacingZ = slices[1].imagePositionZ - slices[0].imagePositionZ;
		}
		else
		{
			_mPixelSpacingZ = slices[0].sliceThickness;
		}


		// Step 5: resize _mVoxels — how big?
		uint32_t sliceSize = _mSliceWidth * _mSliceHeight;
		_mVoxels.resize(sliceSize * _mDepth);

		// Step 6: loop through slices, copy each one's pixels — where does slice i go?
		for (uint32_t i = 0; i < _mDepth; i++)
		{
			std::copy(
				slices[i].pixelData.begin(),
				slices[i].pixelData.end(),
				_mVoxels.begin() + (i * sliceSize)
			);  
			// the line _mVoxels.begin() + (i * sliceSize) why?
			// we are saying ro slices[i] read the pixelData from begining to end
		}

		std::cout << "Volume built: " << _mSliceWidth << "x" << _mSliceHeight << "x" << _mDepth << std::endl;
		std::cout << "Spacing: " << _mPixelSpacingX << "x" << _mPixelSpacingY << "x" << _mPixelSpacingZ << " mm" << std::endl;
		std::cout << "Total voxels: " << _mVoxels.size() << std::endl;

	}
}
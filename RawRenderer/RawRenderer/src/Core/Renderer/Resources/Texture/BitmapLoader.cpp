#include "BitmapLoader.h"

#include <stdexcept>
#include "thirdParty/stb/include/stb_image.h"

#include "Utilities/OutputPrint.h"

std::shared_ptr<RBitmap> RBitmap::FromFile(const std::string& filename, int channels /*= 4*/)
{
	PRINTOUT(L"Loading image: " << filename.c_str() << std::endl;);

	std::shared_ptr<RBitmap> image{ new RBitmap };

	assert(image);

	if (stbi_is_hdr(filename.c_str())) {
		float* pixels = stbi_loadf(filename.c_str(), &image->Width, &image->Height, &image->NumChannels, channels);
		if (pixels) {
			image->Data.reset(reinterpret_cast<unsigned char*>(pixels));
			image->bHDR = true;
		}
	}
	else {
		unsigned char* pixels = stbi_load(filename.c_str(), &image->Width, &image->Height, &image->NumChannels, channels);
		if (pixels) {
			image->Data.reset(pixels);
			image->bHDR = false;
		}
	}
	if (channels > 0) {
		image->NumChannels = channels;
	}

	assert(image->Data);

	return image;
}

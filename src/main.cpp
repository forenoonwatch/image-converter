#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>

#include <string>
#include <vector>

#include "stb_image.h"
//#include "stb_image_write.h"

#include <b64/cencode.h>

#define HEADER_SIZE_BYTES		12
#define PIXEL_INDEX_SIZE_BYTES	3
#define COLOR_INDEX_SIZE_BYTES	2

#define FILE_EXTENSION			".rbxmx"

#define COLOR_FUZZ_PERCENT		0.1f

struct Header {
	uint32_t width;
	uint32_t height;
	uint32_t colorDataSize;
} __attribute__((packed));

struct Color3 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct ColorIndex {
	uint32_t pixelIndex;
	uint16_t colorIndex;
};

void image_resize(unsigned char* destImage, const unsigned char* sourceImage,
		int originalWidth, int originalHeight,
		int numChannels, int desiredWidth, int desiredHeight);

// TODO: unordered_set<Color3> for faster lookups
void generate_color_indices(const unsigned char* data, int width, int height, int numChannels,
		std::vector<Color3>& colors, std::vector<ColorIndex>& indices);
float percent_difference(float a, float b);
bool color_equals(const Color3& a, const Color3& b);

void get_file_name(char* result, const char* sourceFileName);

void write_rbxmx_file(FILE* fileOut, const std::vector<Color3>& colors,
		const std::vector<ColorIndex>& indices, int width, int height);

char* encode_image_data(const std::vector<Color3>& colors, const std::vector<ColorIndex>& indices,
		int width, int height, size_t* totalDataSize);
char* encode_rbxmx_attributes(const char** attribNames, const char** attribData, const size_t* attribDataSizes, uint32_t numAttributes,
		size_t* encodeLength);

int main(int argc, char** argv) {
	if (argc < 3) {
		printf("Usage %s image_size file_path\n");
		return 1;
	}

	int desiredHeight = std::atoi(argv[1]);

	if (desiredHeight <= 0) {
		printf("Invalid image size: %s\n", argv[1]);
		return 1;
	}

	int width, height, numChannels;
	unsigned char* image = stbi_load(argv[2], &width, &height, &numChannels, 0);

	if (!image) {
		printf("Failed to load image %s\n", argv[2]);
		return 1;
	}

	int desiredWidth = static_cast<int>(static_cast<double>(width) / static_cast<double>(height) * desiredHeight);

	printf("Resized image to %d, %d\n", desiredWidth, desiredHeight);

	unsigned char* resizedImage = (unsigned char*)malloc(desiredWidth * desiredHeight * numChannels);
	
	image_resize(resizedImage, image, width, height, numChannels, desiredWidth, desiredHeight);

	//stbi_write_png("resized.png", desiredWidth, desiredHeight, numChannels, resizedImage, numChannels * desiredWidth);

	stbi_image_free(image);

	std::vector<Color3> imageColors;
	std::vector<ColorIndex> imageIndices;

	generate_color_indices(resizedImage, desiredWidth, desiredHeight, numChannels, imageColors, imageIndices);

	printf("%d colors, %d indices\n", imageColors.size(), imageIndices.size());

	free(resizedImage);

	char* fileName = (char*)malloc(strlen(argv[2]) + strlen(FILE_EXTENSION));
	get_file_name(fileName, argv[2]);
	strcat(fileName, FILE_EXTENSION);

	FILE* file = fopen(fileName, "w");

	if (!file) {
		printf("File %s failed to open\n", fileName);
		return 1;
	}

	write_rbxmx_file(file, imageColors, imageIndices, desiredWidth, desiredHeight);

	fclose(file);

	printf("Wrote image to file %s\n", fileName);

	free(fileName);

	return 0;
}

void image_resize(unsigned char* destImage, const unsigned char* sourceImage,
		int originalWidth, int originalHeight,
		int numChannels, int desiredWidth, int desiredHeight) {
	for (int y = 0; y < desiredHeight; ++y) {
		for (int x = 0; x < desiredWidth; ++x) {
			double u = static_cast<double>(x) / static_cast<double>(desiredWidth);
			double v = static_cast<double>(y) / static_cast<double>(desiredHeight);

			int srcX = u * originalWidth;
			int srcY = v * originalHeight;

			int srcI = ((srcY * originalWidth) + srcX) * numChannels;
			int dstI = ((y * desiredWidth) + x) * numChannels;

			for (int channel = 0; channel < numChannels; ++channel) {
				destImage[dstI + channel] = sourceImage[srcI + channel];
			}
		}
	}
}

void get_file_name(char* result, const char* sourceFileName) {
	const char* p = sourceFileName + strlen(sourceFileName) - 1;

	while (p >= sourceFileName && *p != '.') {
		--p;
	}

	strncpy(result, sourceFileName, p - sourceFileName);
	result[p - sourceFileName] = '\0';
}

void generate_color_indices(const unsigned char* data, int width, int height, int numChannels,
		std::vector<Color3>& colors, std::vector<ColorIndex>& indices) {
	int i;
	int foundColor;
	Color3 color;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			i = ((y * width) + x) * numChannels;

			color.r = data[i];
			color.g = data[i + 1];
			color.b = data[i + 2];

			foundColor = -1;

			for (int j = 0; j < colors.size(); ++j) {
				if (color_equals(color, colors[j])) {
					foundColor = j;
					break;
				}
			}

			uint32_t pixelIndex = static_cast<uint32_t>((y * width) + x);

			if (foundColor >= 0) {
				indices.push_back({pixelIndex, static_cast<uint16_t>(foundColor)});
			}
			else {
				colors.push_back(color);
				indices.push_back({pixelIndex, static_cast<uint16_t>(colors.size() - 1)});
			}
		}
	}
}

float percent_difference(float a, float b) {
	return fabs(a - b) / ((a + b) * 0.5f);
}

bool color_equals(const Color3& a, const Color3& b) {
	return percent_difference(static_cast<float>(a.r), static_cast<float>(b.r)) < COLOR_FUZZ_PERCENT
			&& percent_difference(static_cast<float>(a.g), static_cast<float>(b.g)) < COLOR_FUZZ_PERCENT
			&& percent_difference(static_cast<float>(a.b), static_cast<float>(b.b)) < COLOR_FUZZ_PERCENT;
}

void write_rbxmx_file(FILE* fileOut, const std::vector<Color3>& colors,
		const std::vector<ColorIndex>& indices, int width, int height) {
	FILE* formatFile = fopen("../raw-data.txt", "r");

	if (!formatFile) {
		puts("Failed to open format file, make sure you run this from the right directory");
		exit(1);
	}

	char c;
	bool formatStringOpen = false;
	std::vector<char> formatString;

	int colorDataSize = colors.size() * 3;

	while ((c = fgetc(formatFile)) != EOF) {
		if (c == '`') {
			if (formatStringOpen) {
				formatStringOpen = false;
				formatString.push_back('\0');

				if (strcmp(formatString.data(), "attributes") == 0) {
					size_t totalDataSize;
					char* imageData = encode_image_data(colors, indices, width, height, &totalDataSize);
					const char* attribNames[] = {"RawData"};

					size_t encodeLength;
					char* base64Data = encode_rbxmx_attributes(attribNames, const_cast<const char**>(&imageData), &totalDataSize, 1, &encodeLength);

					free(imageData);

					fwrite(base64Data, sizeof(char), encodeLength, fileOut);
					free(base64Data);
				}
				else if (strcmp(formatString.data(), "image_width") == 0) {
					std::string widthStr = std::to_string(width);
					fwrite(widthStr.c_str(), sizeof(char), widthStr.length(), fileOut);
				}
				else if (strcmp(formatString.data(), "image_height") == 0) {
					std::string heightStr = std::to_string(height);
					fwrite(heightStr.c_str(), sizeof(char), heightStr.length(), fileOut);
				}
			}
			else {
				formatStringOpen = true;
				formatString.clear();
			}
		}
		else {
			if (formatStringOpen) {
				formatString.push_back(c);
			}
			else {
				fputc(c, fileOut);
			}
		}
	}

	fclose(formatFile);
}

template <typename T>
T reverse_bytes(T v) {
	T result {};

	for (int i = 0; i < sizeof(T); ++i) {
		result |= ((v >> (8 * i)) & 0xFF) << (8 * (sizeof(T) - i - 1));
	}

	return result;
}

template <typename T>
void print_binary(T v) {
	for (size_t i = 0; i < sizeof(T) * 8; ++i) {
		putchar(((v >> i) & 1) ? '1' : '0');

		if (i % 8 == 0) {
			putchar(' ');
		}
	}

	putchar('\n');
}

char* encode_image_data(const std::vector<Color3>& colors, const std::vector<ColorIndex>& indices,
		int width, int height, size_t* totalDataSize) {
	size_t colorDataSize = colors.size() * 3;
	size_t indexDataSize = indices.size() * (COLOR_INDEX_SIZE_BYTES + PIXEL_INDEX_SIZE_BYTES);

	*totalDataSize = sizeof(Header) + colorDataSize + indexDataSize;

	char* rawData = (char*)malloc(*totalDataSize);

	Header header;
	header.width = width;
	header.height = height;
	header.colorDataSize = colorDataSize;

	uint8_t* p = reinterpret_cast<uint8_t*>(rawData);

	memcpy(p, &header, sizeof(Header));
	p += sizeof(Header);
	
	for (const auto& color : colors) {
		memcpy(p, &color.r, 1);
		++p;

		memcpy(p, &color.g, 1);
		++p;

		memcpy(p, &color.b, 1);
		++p;
	}

	for (const auto& index : indices) {
		uint32_t pixelIndex = index.pixelIndex;
		uint32_t colorIndex = index.colorIndex;

		memcpy(p, &pixelIndex, PIXEL_INDEX_SIZE_BYTES);
		p += PIXEL_INDEX_SIZE_BYTES;

		memcpy(p, &colorIndex, COLOR_INDEX_SIZE_BYTES);
		p += COLOR_INDEX_SIZE_BYTES;
	}

	return rawData;
}

char* encode_rbxmx_attributes(const char** attribNames, const char** attribData, const size_t* attribDataSizes, uint32_t numAttributes,
		size_t* encodeLength) {
	uint32_t* nameLengths = (uint32_t*)malloc(numAttributes * sizeof(uint32_t));

	size_t totalSize = sizeof(uint32_t);

	for (uint32_t i = 0; i < numAttributes; ++i) {
		nameLengths[i] = static_cast<uint32_t>(strlen(attribNames[i]));

		totalSize += nameLengths[i] + sizeof(uint32_t); // name + name size
		totalSize += attribDataSizes[i];
		totalSize += 1; // splitter/type
		totalSize += sizeof(uint32_t); // attrib size
	}

	char* inData = (char*)malloc(totalSize);
	char* p = inData;

	memcpy(p, &numAttributes, sizeof(uint32_t));
	p += sizeof(uint32_t);

	for (uint32_t i = 0; i < numAttributes; ++i) {
		memcpy(p, &nameLengths[i], sizeof(uint32_t));
		p += sizeof(uint32_t);

		memcpy(p, attribNames[i], nameLengths[i] * sizeof(char));
		p += nameLengths[i] * sizeof(char);

		*p = 2; // TODO: probably the attribute type for string
		++p;

		uint32_t attribSize = attribDataSizes[i];
		memcpy(p, &attribSize, sizeof(uint32_t));
		p += sizeof(uint32_t);

		memcpy(p, attribData[i], attribDataSizes[i]);
		p += attribDataSizes[i];
	}

	char* outData = (char*)calloc(totalSize * 2, sizeof(char));

	*encodeLength = 0;

	base64_encodestate state;
	base64_init_encodestate(&state);
	*encodeLength += base64_encode_block(inData, totalSize, outData, &state);
	*encodeLength += base64_encode_blockend(outData + *encodeLength, &state);

	free(inData);
	free(nameLengths);

	return outData;
}

#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>

struct MinMax {
	float minVal;
	float maxVal;
	MinMax(float minVal = 0, float maxVal = 0)
		: minVal(minVal)
		, maxVal(maxVal) {}
};

/**
 * resolution is the width of the preview, in pixels, or steps or whatever.
 * the height of the preview is -1 to +1
 */

static std::vector<MinMax> createPreviewFromWav(const std::vector<float> &wav, int resolution = 128) {
	std::vector<MinMax> preview;
	preview.reserve(resolution);
	double multiplier = (wav.size() - 1) / (double) (resolution);
	for (int i = 0; i < resolution; i++) {
		int from	 = std::min(static_cast<int>(i * multiplier), (int) wav.size() - 1);
		int to		 = std::min(static_cast<int>((i + 1) * multiplier), (int) wav.size() - 1);
		float minVal = wav[from];
		float maxVal = minVal;

		printf("%d %d\n", from, to);
		for (int p = from; p < to; p++) {
			minVal = std::min(minVal, wav[p]);
			maxVal = std::max(maxVal, wav[p]);
		}
		minVal = std::clamp(minVal, -1.f, 1.f);
		maxVal = std::clamp(maxVal, -1.f, 1.f);
		preview.push_back({minVal, maxVal});
	}

	return preview;
}

struct rgba {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;
	rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
		: r(r)
		, g(g)
		, b(b)
		, a(a) {}
	rgba(uint32_t hex = 0x000000) {
		r = (hex >> 16) & 0xFF;
		g = (hex >> 8) & 0xFF;
		b = hex & 0xFF;
		a = 255;
	}
};

/**
 * Returns a pixel array suitable to create an image. Image width is specified
 * by the preview length as set in createPreviewFromWav.
 */
std::vector<rgba> renderToBitmap(const std::vector<MinMax> &preview,
								 int height,
								 rgba bg = rgba(0, 0, 0),
								 rgba fg = rgba(255, 255, 255)) {
	int width = preview.size();

	std::vector<rgba> img(width * height, bg);

	for (int i = 0; i < preview.size(); i++) {
		int from = mapf(preview[i].minVal, 1, -1, height - 1, 0);
		int to	 = mapf(preview[i].maxVal, 1, -1, height - 1, 0);
		for (int a = from; a <= to; a++) {
			img[i + a * width] = fg;
		}
	}

	return img;
}

bool writePrevFile(std::vector<MinMax> &prev, std::string path) {
	std::vector<uint8_t> data;
	data.reserve(prev.size() * 2);
	for (const auto &p: prev) {
		data.push_back((p.minVal + 1.f) * 127.5f);
		data.push_back((p.maxVal + 1.f) * 127.5f);
	}

	std::ofstream outfile(path, std::ios::out | std::ios::binary);
	if (outfile.fail()) return false;
	outfile.write((const char *) data.data(), data.size());
	if (outfile.fail()) return false;
	outfile.close();
	if (outfile.fail()) return false;
	return true;
}

bool writeToBinary(const std::vector<float> &wav, const std::string &outputFile, int width = 256) {
	auto preview = createPreviewFromWav(wav, width);
	return writePrevFile(preview, outputFile);
}
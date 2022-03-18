
#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>

#include "blahdio/audio_reader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


static float clampf(float inp, float from, float to) {
	if(inp < from) return from;
	if(inp>to) return to;
	return inp;
}

static float mapf(float inp, float inMin, float inMax, float outMin, float outMax, bool clamp = false) {
	float norm = (inp - inMin) / (inMax - inMin);
	float f = outMin + (outMax - outMin) * norm;
	if(clamp) {
		if(outMax > outMin) {
			return clampf(f, outMin, outMax);
		} else {
			return clampf(f, outMax, outMin);
		}
	}
	return f;
}

bool verbose = false;

static std::vector<float> readFileToMono(std::string path) {

	std::vector<float> out;
	// Assume that the file type is WAV. Blahdio will try that first, but if it fails
	// to read a valid WAV header it will also try MP3, FLAC and WavPack.
	blahdio::AudioReader reader(path);

	// Read the header. 
	reader.read_header(); // Will throw an exception if the file
	                      // type couldn't be deduced.

	const auto num_frames = reader.get_num_frames();
	const auto num_channels = reader.get_num_channels();
	const auto sample_rate = reader.get_sample_rate();
	const auto bit_depth = reader.get_bit_depth();
	
	double duration = num_frames / (double) sample_rate;
	
	if(verbose) {
		printf("File: %s\n", path.c_str());
		printf("duration: %.1fs\nframes: %llu\nchannels: %d\nsample rate: %d\nbit depth: %d\n", duration, num_frames, num_channels, sample_rate, bit_depth);
	}
	blahdio::AudioReader::Callbacks reader_callbacks;

	reader_callbacks.should_abort = []() { return false; };

	reader_callbacks.return_chunk = [&](const void* data, std::uint64_t frame, std::uint32_t num_frames) {

		float *d = (float*)data;
		
		
		if(num_frames==1) {
			out.insert(out.end(), d, d + num_frames);
		} else {
			for(int i = 0; i < num_frames; i+= num_channels) {
				float total = 0;
				for(int j = 0; j < num_channels; j++) {
					total += d[i+j];
				}
				out.push_back(total / (float)num_channels);
			}
		}
	};

	// Read file 512 frames at a time
	reader.read_frames(reader_callbacks, 512); // Will throw an exception if an error
	                                           // occurs during reading
	return out;
}



struct MinMax {
	float minVal;
	float maxVal;
	MinMax(float minVal = 0, float maxVal = 0) : minVal(minVal), maxVal(maxVal) {}
};

/**
 * resolution is the width of the preview, in pixels, or steps or whatever.
 * the height of the preview is -1 to +1
 */
static std::vector<MinMax> createPreviewFromWav(const std::vector<float> &wav, int resolution = 128) {
	
	std::vector<MinMax> preview;
	for(int i = 0; i < resolution; i++) {
		double from = i * (wav.size()-1) / (double)(resolution-1);
		double to = (i+1) * (wav.size()-1) / (double)(resolution-1);
		float minVal = wav[(int)from];
		float maxVal = minVal;

		for(int p = from; p < to; p++) {
			minVal = std::min(minVal, wav[p]);
			maxVal = std::max(maxVal, wav[p]);
		}
		minVal = clampf(minVal, -1, 1);
		maxVal = clampf(maxVal, -1, 1);
		preview.push_back({minVal, maxVal});
	}

	return preview;
}


struct rgba {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;
	rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
	rgba(uint32_t hex = 0x000000) {
		r = (hex >> 16) & 0xFF;
		g = (hex >> 8) & 0xFF;
		b = hex & 0xFF;
		a = 255;
	}
};


/**
 * Returns a pixel array suitable to create an image. Image width is specified by the preview length
 * as set in createPreviewFromWav.
 */
std::vector<rgba> renderToBitmap(const std::vector<MinMax> &preview, int height, rgba bg = rgba(0, 0, 0), rgba fg = rgba(255, 255, 255)) {
	int width = preview.size();

	
	std::vector<rgba> img(width * height, bg);
	
	for(int i = 0; i < preview.size(); i++) {

		int from = mapf(preview[i].minVal, 1, -1, height-1, 0);
		int to = mapf(preview[i].maxVal, 1, -1, height-1, 0);
		for(int a = from; a <= to; a++) {
			img[i + a * width] = fg;
		}
	}

	return img;
}

void printUsage() {
		printf(
		"Usage:                                                             \n"
		"       wav2png audio.wav -o wav.png [options]                      \n"
		"                                                                   \n"
		"       Options:                                                    \n"
		"                 -o file sets the output (default is [filename].png\n"
		"                 -w nn   sets the width (default 256)              \n"
		"                 -h nn   sets the height (default 256)             \n"
		"                 -v      verbose flag                              \n"
		"                 -b      binary output flag (.prev format)         \n"
	);

}

bool tryToFindFlag(std::string argName, int argc, char** argv, bool defaultVal = false) {
	for(int i = 0; i < argc; i++) {
		if(argv[i]==argName) return true;
	}
	return false;
}

int tryToFindArgValue(std::string argName, int argc, char** argv, int defaultVal) {
	try {
		for(int i = 0; i < argc-1; i++) {
			if(argv[i]==argName) {
				return std::stoi(argv[i+1]);
			}
		}
	} catch(const std::invalid_argument &err) {
		printf("invalid value for %s - using default of %d\n", argName.c_str(), defaultVal);
	}

	return defaultVal;
}


std::string tryToFindArgValue(std::string argName, int argc, char** argv, std::string defaultVal) {
	for(int i = 0; i < argc-1; i++) {
		if(argv[i]==argName) {
			return argv[i+1];
		}
	}
	return defaultVal;
}


std::string createDefaultOutName(std::string orig, std::string ext) {
	auto lastDot = orig.rfind('.');
	if(lastDot==-1) {
		return orig + "." + ext;
	}

	return orig.substr(0, lastDot+1) + ext;
}










bool writePrevFile(std::vector<MinMax> &prev, std::string path) {


	std::vector<uint8_t> data;
	data.reserve(prev.size() * 2);
	for(const auto &p : prev) {
		data.push_back((p.minVal + 1.f)*127.5f);
		data.push_back((p.maxVal + 1.f)*127.5f);
	}

	std::ofstream outfile(path, std::ios::out | std::ios::binary);
	if(outfile.fail()) return false;
	outfile.write((const char*)data.data(), data.size());
	if(outfile.fail()) return false;
	outfile.close();
	if(outfile.fail()) return false;
	return true;
}











int main(int argc, char** argv) {

	if(argc<2) {
		printUsage();
		return 1;
	}
	verbose = tryToFindFlag("-v", argc, argv, false);
	std::string path = argv[1];
	
	bool binary = tryToFindFlag("-b", argc, argv, false);


	std::string defaultOut = createDefaultOutName(path, binary?"prev":"png");

	std::string outputFile = tryToFindArgValue("-o", argc, argv, defaultOut);

	try {

		auto wav = readFileToMono(path);

		int width = tryToFindArgValue("-w", argc, argv, 256);
		int height = tryToFindArgValue("-h", argc, argv, 256);


		if(verbose) {
			if(binary) {
				printf("rendering waveform to binary file of length %d  path: %s\n", width, outputFile.c_str());
			} else {
				printf("rendering waveform to %d x %d image to file %s\n", width, height, outputFile.c_str());
			}
		}

		auto preview = createPreviewFromWav(wav, width);


		if(binary) {
			bool res = writePrevFile(preview, outputFile);
			if(!res) {
				printf("Error writing prev file\n");
				return 1;
			}
		} else {
			auto bmp = renderToBitmap(preview, height);
			
			int res = stbi_write_png(outputFile.c_str(), width, height, 4, bmp.data(), width * 4);
			if(!res) {
				printf("failed to write file properly");
			}
			return !res;
		}
	} catch (const std::runtime_error &err) {
		printf("There was an error - perhaps input file doesn't exist\n");
		return 1;
	}

	return 0;
}




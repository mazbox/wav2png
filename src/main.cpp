#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <wav2png/wav2png.h>

bool verbose = false;

static std::vector<float> readFlac(const std::string &path) {
	unsigned int channels, sampleRate;
	drflac_uint64 totalFrames;
	float *data = drflac_open_file_and_read_pcm_frames_f32(path.c_str(), &channels, &sampleRate, &totalFrames, nullptr);
	if (!data) return {};

	std::vector<float> mono(totalFrames);
	for (drflac_uint64 i = 0; i < totalFrames; i++) {
		float sum = 0;
		for (unsigned int c = 0; c < channels; c++) {
			sum += data[i * channels + c];
		}
		mono[i] = sum / (float)channels;
	}

	if (verbose) {
		printf("File: %s\nformat: FLAC\nduration: %.1fs\nframes: %llu\nchannels: %d\nsample rate: %d\n",
			   path.c_str(), totalFrames / (double)sampleRate, totalFrames, channels, sampleRate);
	}

	drflac_free(data, nullptr);
	return mono;
}

static std::vector<float> readWav(const std::string &path) {
	unsigned int channels, sampleRate;
	drwav_uint64 totalFrames;
	float *data = drwav_open_file_and_read_pcm_frames_f32(path.c_str(), &channels, &sampleRate, &totalFrames, nullptr);
	if (!data) return {};

	std::vector<float> mono(totalFrames);
	for (drwav_uint64 i = 0; i < totalFrames; i++) {
		float sum = 0;
		for (unsigned int c = 0; c < channels; c++) {
			sum += data[i * channels + c];
		}
		mono[i] = sum / (float)channels;
	}

	if (verbose) {
		printf("File: %s\nformat: WAV\nduration: %.1fs\nframes: %llu\nchannels: %d\nsample rate: %d\n",
			   path.c_str(), totalFrames / (double)sampleRate, totalFrames, channels, sampleRate);
	}

	drwav_free(data, nullptr);
	return mono;
}

static std::vector<float> readMp3(const std::string &path) {
	drmp3_config config;
	drmp3_uint64 totalFrames;
	float *data = drmp3_open_file_and_read_pcm_frames_f32(path.c_str(), &config, &totalFrames, nullptr);
	if (!data) return {};

	std::vector<float> mono(totalFrames);
	for (drmp3_uint64 i = 0; i < totalFrames; i++) {
		float sum = 0;
		for (unsigned int c = 0; c < config.channels; c++) {
			sum += data[i * config.channels + c];
		}
		mono[i] = sum / (float)config.channels;
	}

	if (verbose) {
		printf("File: %s\nformat: MP3\nduration: %.1fs\nframes: %llu\nchannels: %d\nsample rate: %d\n",
			   path.c_str(), totalFrames / (double)config.sampleRate, totalFrames, config.channels, config.sampleRate);
	}

	drmp3_free(data, nullptr);
	return mono;
}

static std::string getExtension(const std::string &path) {
	auto dot = path.rfind('.');
	if (dot == std::string::npos) return "";
	std::string ext = path.substr(dot + 1);
	for (auto &c : ext) c = tolower(c);
	return ext;
}

static std::vector<float> readFileToMono(const std::string &path) {
	auto ext = getExtension(path);

	// Try by extension first
	if (ext == "flac") return readFlac(path);
	if (ext == "wav") return readWav(path);
	if (ext == "mp3") return readMp3(path);

	// Try all formats
	auto data = readWav(path);
	if (!data.empty()) return data;
	data = readFlac(path);
	if (!data.empty()) return data;
	data = readMp3(path);
	return data;
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
		"                 -b      binary output flag (.prev format)         \n");
}

bool tryToFindFlag(std::string argName, int argc, char **argv, bool defaultVal = false) {
	for (int i = 0; i < argc; i++) {
		if (argv[i] == argName) return true;
	}
	return false;
}

int tryToFindArgValue(std::string argName, int argc, char **argv, int defaultVal) {
	try {
		for (int i = 0; i < argc - 1; i++) {
			if (argv[i] == argName) {
				return std::stoi(argv[i + 1]);
			}
		}
	} catch (const std::invalid_argument &err) {
		printf("invalid value for %s - using default of %d\n", argName.c_str(), defaultVal);
	}
	return defaultVal;
}

std::string tryToFindArgValue(std::string argName, int argc, char **argv, std::string defaultVal) {
	for (int i = 0; i < argc - 1; i++) {
		if (argv[i] == argName) {
			return argv[i + 1];
		}
	}
	return defaultVal;
}

std::string createDefaultOutName(std::string orig, std::string ext) {
	auto lastDot = orig.rfind('.');
	if (lastDot == std::string::npos) {
		return orig + "." + ext;
	}
	return orig.substr(0, lastDot + 1) + ext;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printUsage();
		return 1;
	}

	verbose = tryToFindFlag("-v", argc, argv, false);
	std::string path = argv[1];
	bool binary = tryToFindFlag("-b", argc, argv, false);
	std::string defaultOut = createDefaultOutName(path, binary ? "prev" : "png");
	std::string outputFile = tryToFindArgValue("-o", argc, argv, defaultOut);

	auto wav = readFileToMono(path);

	if (wav.empty()) {
		printf("Error: could not read audio file '%s'\n", path.c_str());
		return 1;
	}

	int width = tryToFindArgValue("-w", argc, argv, 256);
	int height = tryToFindArgValue("-h", argc, argv, 256);

	if (verbose) {
		if (binary) {
			printf("rendering waveform to binary file of length %d  path: %s\n", width, outputFile.c_str());
		} else {
			printf("rendering waveform to %d x %d image to file %s\n", width, height, outputFile.c_str());
		}
	}

	if (binary) {
		return writeToBinary(wav, outputFile, width) ? 0 : 1;
	}

	auto preview = createPreviewFromWav(wav, width);
	auto bmp = renderToBitmap(preview, height);

	int res = stbi_write_png(outputFile.c_str(), width, height, 4, bmp.data(), width * 4);
	if (!res) {
		printf("failed to write file properly\n");
	}
	return !res;
}

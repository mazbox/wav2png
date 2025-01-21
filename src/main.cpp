

#include "blahdio/audio_reader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static float clampf(float inp, float from, float to) {
  if (inp < from)
    return from;
  if (inp > to)
    return to;
  return inp;
}

static float mapf(float inp, float inMin, float inMax, float outMin,
                  float outMax, bool clamp = false) {
  float norm = (inp - inMin) / (inMax - inMin);
  float f = outMin + (outMax - outMin) * norm;
  if (clamp) {
    if (outMax > outMin) {
      return clampf(f, outMin, outMax);
    } else {
      return clampf(f, outMax, outMin);
    }
  }
  return f;
}

#include "wav2png.h"

bool verbose = false;

static std::vector<float> readFileToMono(std::string path) {

  std::vector<float> out;
  // Assume that the file type is WAV. Blahdio will try that first, but if it
  // fails to read a valid WAV header it will also try MP3, FLAC and WavPack.
  blahdio::AudioReader reader(path);

  // Read the header.
  reader.read_header(); // Will throw an exception if the file
                        // type couldn't be deduced.

  const auto num_frames = reader.get_num_frames();
  const auto num_channels = reader.get_num_channels();
  const auto sample_rate = reader.get_sample_rate();
  const auto bit_depth = reader.get_bit_depth();

  double duration = num_frames / (double)sample_rate;

  if (verbose) {
    printf("File: %s\n", path.c_str());
    printf("duration: %.1fs\nframes: %llu\nchannels: %d\nsample rate: %d\nbit "
           "depth: %d\n",
           duration, num_frames, num_channels, sample_rate, bit_depth);
  }
  blahdio::AudioReader::Callbacks reader_callbacks;

  reader_callbacks.should_abort = []() { return false; };

  reader_callbacks.return_chunk = [&](const void *data, std::uint64_t frame,
                                      std::uint32_t num_frames) {
    float *d = (float *)data;

    if (num_frames == 1) {
      out.insert(out.end(), d, d + num_frames);
    } else {
      for (int i = 0; i < num_frames; i += num_channels) {
        float total = 0;
        for (int j = 0; j < num_channels; j++) {
          total += d[i + j];
        }
        out.push_back(total / (float)num_channels);
      }
    }
  };

  // Read file 512 frames at a time
  reader.read_frames(reader_callbacks, 512); // Will throw an exception if an
                                             // error occurs during reading
  return out;
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

bool tryToFindFlag(std::string argName, int argc, char **argv,
                   bool defaultVal = false) {
  for (int i = 0; i < argc; i++) {
    if (argv[i] == argName)
      return true;
  }
  return false;
}

int tryToFindArgValue(std::string argName, int argc, char **argv,
                      int defaultVal) {
  try {
    for (int i = 0; i < argc - 1; i++) {
      if (argv[i] == argName) {
        return std::stoi(argv[i + 1]);
      }
    }
  } catch (const std::invalid_argument &err) {
    printf("invalid value for %s - using default of %d\n", argName.c_str(),
           defaultVal);
  }

  return defaultVal;
}

std::string tryToFindArgValue(std::string argName, int argc, char **argv,
                              std::string defaultVal) {
  for (int i = 0; i < argc - 1; i++) {
    if (argv[i] == argName) {
      return argv[i + 1];
    }
  }
  return defaultVal;
}

std::string createDefaultOutName(std::string orig, std::string ext) {
  auto lastDot = orig.rfind('.');
  if (lastDot == -1) {
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

  try {

    auto wav = readFileToMono(path);

    int width = tryToFindArgValue("-w", argc, argv, 256);
    int height = tryToFindArgValue("-h", argc, argv, 256);

    if (verbose) {
      if (binary) {
        printf("rendering waveform to binary file of length %d  path: %s\n",
               width, outputFile.c_str());
      } else {
        printf("rendering waveform to %d x %d image to file %s\n", width,
               height, outputFile.c_str());
      }
    }

    if (binary) {
      return writeToBinary(wav, outputFile, width) ? 1 : 0;
    }

    auto preview = createPreviewFromWav(wav, width);
    auto bmp = renderToBitmap(preview, height);

    int res = stbi_write_png(outputFile.c_str(), width, height, 4, bmp.data(),
                             width * 4);
    if (!res) {
      printf("failed to write file properly");
    }
    return !res;
  } catch (const std::runtime_error &err) {
    printf("There was an error - perhaps input file doesn't exist\n");
    return 1;
  }

  return 0;
}

# wav2png

Renders a wav file into a png or a binary file for use elsewhere.

```
		Usage:                                                             
		       wav2png audio.wav -o wav.png [options]                      
		                                                                   
		       Options:                                                    
		                 -o file sets the output (default is [filename].png
		                 -w nn   sets the width (default 256)              
		                 -h nn   sets the height (default 256)             
		                 -v      verbose flag                              
		                 -b      binary output flag (.prev format)         
```

The .prev format is a file of sequential min and max values of successive chunks of the waveform as unsigned 8-bit ints (eg. ```uint8_t```)

So if you set a width of 128 with ```-w 128``` then there will be 128 pairs of ```uint8_t```'s (min followed by the max)

e.g.

```
struct MinMax {
	uint8_t minValue;
	uint8_t maxValue;
}

std::vector<MinMax> prevFile;
```
// probe.cpp

#include <iostream>
#include "RtAudio.h"

int main()
{
  RtAudio *audio = 0;

  // Default RtAudio constructor
  try {
    audio = new RtAudio();
  }
  catch (RtAudioError &error) {
    error.printMessage();
    exit(EXIT_FAILURE);
  }

  // Determine the number of devices available
  int devices = audio->getDeviceCount();

  // Scan through devices for various capabilities
  RtAudio::DeviceInfo info;
  for (int i=0; i<devices; i++) {

    try {
      info = audio->getDeviceInfo(i);
    }
    catch (RtAudioError &error) {
      error.printMessage();
      break;
    }

    // Print, for example, the maximum number of output channels for each device
    std::cout << "device = " << i;
    std::cout << ": name = " << info.name;
    std::cout << ": maximum input channels = " << info.inputChannels;
    std::cout << ": maximum output channels = " << info.outputChannels << "\n";
  }

  // Clean up
  delete audio;

  return 0;
}
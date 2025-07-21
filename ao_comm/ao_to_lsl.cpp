#include <ao_comm/ao_to_lsl.hpp>
// #include <ao_comm_deps/AO/include/EthernetStandAlone.h>
#include <ao_comm_deps/lsl/include/lsl_cpp.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random> // for random mockup data
#include <thread>
#include <valarray>

// ------------------------------ AO lib compatibility ----------------
// int16 is defined within the AO lib, doing it manually here to keep the code
// consistent
#ifdef _WIN32
// Windows (MSVC, MinGW)
typedef short int16;
#else
// Linux, macOS, and most POSIX systems
#include <stdint.h>
typedef int16_t int16;
#endif

struct MAC_ADDR {
  uint8_t addr[6];
};
// --------------------------------------------------------------------

void connect_to_ao(int *mac_adress_iter, size_t size_of_iter) {
  int next;

  MAC_ADDR dsp;
  for (int i = 0; i < size_of_iter; i++) {
    next = mac_adress_iter[i];
    dsp.addr[i] = next;
  }
  std::cout << "connecting to AO - DSP MAC address -> ";
  for (size_t i = 0; i < 6; i++) {
    std::cout << std::hex << dsp.addr[i] << ':';
  }
  std::cout << std::dec << std::endl; // switch back to dec printing

  // int connect = DefaultStartConnection(&dsp, 0);
  int connect = 1;

  std::cout << "DefaultStartConnection returned:" << connect << std::endl;

  std::chrono::seconds dura(1);
  std::this_thread::sleep_for(dura);

  // std::cout << "isconnected ->" << isConnected() << std::endl;  // We would
  // use somthing like this in the actual app bool isconnected = isConnected();
  bool isconnected = true;
  std::cout << "isconnected ->" << isconnected << std::endl;

  if (!isconnected) {
    std::cout << "connection failed" << std::endl;
  } else {
    std::cout << "connection successful " << std::endl;
  }
}

// static bool checkBufferIsEmpty(int16 *buffer) {
//   bool isEmpty = true;
//   for (int i = 0; i < sizeof(buffer); i++) {
//     if (buffer[i] != 0) {
//       isEmpty = false;
//     }
//   }
//   return isEmpty;
// }
//
// static void checkBufferState(int16 *buffer, bool *empty_returns_track,
//                              int size_of_empty_track, int run) {
//   bool emptyBuffer = checkBufferIsEmpty(buffer);
//
//   int idx = run % size_of_empty_track;
//   empty_returns_track[idx] = emptyBuffer;
//
//   // Now check if any is false -> at least one had data
//   int i;
//   for (i = 0; i < size_of_empty_track; i++) {
//     if (!empty_returns_track[i]) {
//       break;
//     }
//   }
//   bool bufferEmptySinceNRuns = (i == size_of_empty_track);
//
//   if (bufferEmptySinceNRuns) {
//     std::cout << "Buffer was empty for " << size_of_empty_track << "runs"
//               << std::endl;
//   }
// }

// What we would use with the AO API
// static std::vector<std::vector<float>> sliceData(int16 *data, int actualData,
//                                                  int n_channels) {
//   // Get number of samples per channel
//   int nChannelData = actualData / n_channels;
//
//   // Init vector of shape (nChannelData, nChannels)
//   std::vector<std::vector<float>> resData(nChannelData,
//                                           std::vector<float>(n_channels, 0));
//
//   int curActualData = 0;
//   for (int i = 0; i < n_channels; i++) {
//     for (int j = 0; j < nChannelData; j++) {
//       resData[j][i] = data[curActualData];
//       curActualData += 1;
//     }
//   }
//   return resData;
// }

static std::vector<std::vector<float>> sliceData_random() {
  constexpr int NUM_CHANNELS = 24;
  constexpr int SAMPLES_PER_CHANNEL = 10;

  // Random number generator setup
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int16_t> dist(-32768,
                                              32767); // Typical 16-bit range

  // Initialize result vector (10 samples × 16 channels)
  std::vector<std::vector<float>> resData(SAMPLES_PER_CHANNEL,
                                          std::vector<float>(NUM_CHANNELS));

  // Fill with random 16-bit integers converted to float
  for (int sample = 0; sample < SAMPLES_PER_CHANNEL; ++sample) {
    for (int channel = 0; channel < NUM_CHANNELS; ++channel) {
      resData[sample][channel] = static_cast<float>(dist(gen));
    }
  }

  return resData;
}

void printSlicedData(const std::vector<std::vector<float>> &slicedData) {
  // Print header (channel numbers)
  std::cout << "\nSample # | ";
  for (int ch = 0; ch < slicedData[0].size(); ++ch) {
    std::cout << "Ch " << ch << "      ";
  }
  std::cout << "\n";

  // Print separator line
  std::cout << std::string(10 + slicedData[0].size() * 12, '-') << "\n";

  // Print data (rows: samples, columns: channels)
  for (int sample = 0; sample < slicedData.size(); ++sample) {
    for (float val : slicedData[sample]) {
      std::cout << std::fixed << val << " ";
    }
    std::cout << "\n";
  }
}

void stream_ao_data_to_lsl(lsl::stream_outlet *outlet, int *channels_arr,
                           int num_channels, std::atomic_bool &stop_thread,
                           int refresh_rate_ms = 1,
                           int buffer_size_ms = 1000000,
                           int warn_after_n_empty_calls = 10) {
  // ULONG TS_Begin = 0;
  // int16 *buffer = new int16[buffer_size_ms * num_channels];
  // int actualData = 0;
  int funcRet;
  // bool *empty_returns_track = new bool[warn_after_n_empty_calls];

  std::cout << "Buffering channels..." << std::endl;
  // for (int i = 0; i < num_channels; i++) {
  //   AddBufferChannel(channels_arr[i], buffer_size_ms);
  // }

  std::cout << "Creating the sliceData" << std::endl;
  // What would be used with actual API
  // std::vector<std::vector<float>> slicedData =
  //     sliceData(buffer, actualData, num_channels);

  std::vector<std::vector<float>> slicedData = sliceData_random();

  std::cout << "Start streaming" << std::endl;
  int run = 0;
  while (!stop_thread) {
    std::this_thread::sleep_for(std::chrono::milliseconds(refresh_rate_ms));

    // Fill buffer with data from AO
    // funcRet = GetAlignedData(buffer, buffer_size_ms, &actualData,
    // channels_arr,
    //                          num_channels, &TS_Begin);
    funcRet = 0;

    if (funcRet != 0) {
      std::cout << "GetAlignedData returned: " << funcRet << std::endl;
      if (funcRet == 8) { // probably 8 is returned if we are too fast (no new
                          // data - as it is incoming in 15ms chunks)
        continue;         // just scipt to next iteration
      }
    }

    /* These are the matlab return cases as the SDK docu does not list C++
     * I do not think they are fully correct though (event with the correct
     channels we get values of 8 returned, probably just empty as the error
     vanishes if a slow enough update rate is chosen) 0 - No compiling error 3 -
     Can’t do stimulation on the specified channel 4 - The system is not
     connected 5 - The Device Driver is Null 6 - The name of the channel is not
     existed 7 - Out of range 8 -  The channel id is not existed 9 -  Null
     parameter 10 - The system already connected 11 -  Index not found 12 -
     There is no match 13 - Sampling rate is not the same 14 - Gap in data 15 -
     Wrong value for the Duration parameter
    */

    // Check if buffer was empty for _n runs - not needed for mockup
    // checkBufferState(buffer, empty_returns_track, warn_after_n_empty_calls,
    //                  run);

    // Get number of data per channel
    // int nChannelData = actualData / num_channels;

    // Slice data and bring it into the following shape: (n_times, n_chs)
    // std::vector<std::vector<float>> slicedData =
    //     sliceData(buffer, actualData, num_channels);
    std::vector<std::vector<float>> slicedData = sliceData_random();

    // data to lsl
    for (int j = 0; j < slicedData.size(); j++) {
      outlet->push_sample(slicedData.at(j));
    }

    if (run % 1000 == 0) {
      std::cout << "Run: " << run << std::endl;
      // std::cout << "Data for this run:" << std::endl;
      // printSlicedData(slicedData);
    }
    // if (nChannelData != 0 | actualData != 0) {
    //   std::cout << "Got nChannelData: " << nChannelData
    //             << ", actualData: " << actualData << std::endl;
    // }

    run++;
  }
}

#include <ao_comm/ao_to_lsl.hpp>
#include <ao_comm_deps/AO/include/EthernetStandAlone.h>
#include <ao_comm_deps/lsl/include/lsl_cpp.h>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>

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

  int connect = DefaultStartConnection(&dsp, 0);

  std::cout << "DefaultStartConnection returned:" << connect << std::endl;

  std::chrono::seconds dura(1);
  std::this_thread::sleep_for(dura);

  std::cout << "isconnected ->" << isConnected() << std::endl;

  if (isConnected() == FALSE) {
    std::cout << "connection failed" << std::endl;
  } else {
    std::cout << "connection successful " << std::endl;
  }
}

static bool checkBufferIsEmpty(int16 *buffer) {
  bool isEmpty = true;
  for (int i = 0; i < sizeof(buffer); i++) {
    if (buffer[i] != 0) {
      isEmpty = false;
    }
  }
  return isEmpty;
}

static void checkBufferState(int16 *buffer, bool *empty_returns_track,
                             int size_of_empty_track, int run) {
  bool emptyBuffer = checkBufferIsEmpty(buffer);

  int idx = run % size_of_empty_track;
  empty_returns_track[idx] = emptyBuffer;

  // Now check if any is false -> at least one had data
  int i;
  for (i = 0; i < size_of_empty_track; i++) {
    if (!empty_returns_track[i]) {
      break;
    }
  }
  bool bufferEmptySinceNRuns = (i == size_of_empty_track);

  if (bufferEmptySinceNRuns) {
    std::cout << "Buffer was empty for " << size_of_empty_track << "runs"
              << std::endl;
  }
}

static std::vector<std::vector<float>> sliceData(int16 *data, int actualData,
                                                 int n_channels) {
  // Get number of samples per channel
  int nChannelData = actualData / n_channels;

  // Init vector of shape (nChannelData, nChannels)
  std::vector<std::vector<float>> resData(nChannelData,
                                          std::vector<float>(n_channels, 0));

  int curActualData = 0;
  for (int i = 0; i < n_channels; i++) {
    for (int j = 0; j < nChannelData; j++) {
      resData[j][i] = data[curActualData];
      curActualData += 1;
    }
  }
  return resData;
}

void stream_ao_data_to_lsl(lsl::stream_outlet *outlet, int *channels_arr,
                           int num_channels, int refresh_rate_ms = 1,
                           int buffer_size_ms = 1000000,
                           int warn_after_n_empty_calls = 10) {
  ULONG TS_Begin = 0;
  int16 *buffer = new int16[buffer_size_ms * num_channels];
  int actualData = 0;
  int funcRet;
  bool *empty_returns_track = new bool[warn_after_n_empty_calls];
  // int channels_arr2[2] = {10272, 10273};
  // int num_channels2 = 2;
  //
  std::cout << "Buffering channels..." << std::endl;
  for (int i = 0; i < num_channels; i++) {
    AddBufferChannel(channels_arr[i], buffer_size_ms);
  }

  std::cout << "Creating the sliceData" << std::endl;
  std::vector<std::vector<float>> slicedData =
      sliceData(buffer, actualData, num_channels);

  std::cout << "Start streaming" << std::endl;
  int run = 0;
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(refresh_rate_ms));

    // Fill buffer with data from AO
    funcRet = GetAlignedData(buffer, buffer_size_ms, &actualData, channels_arr,
                             num_channels, &TS_Begin);

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

    // Check if buffer was empty for _n runs
    checkBufferState(buffer, empty_returns_track, warn_after_n_empty_calls,
                     run);

    // Get number of data per channel
    int nChannelData = actualData / num_channels;

    // Slice data and bring it into the following shape: (n_times, n_chs)
    std::vector<std::vector<float>> slicedData =
        sliceData(buffer, actualData, num_channels);

    // data to lsl
    for (int j = 0; j < nChannelData; j++) {
      outlet->push_sample(slicedData.at(j));
    }

    if (run % 1000 == 0) {
      std::cout << "Run: " << run << std::endl;
    }
    // if (nChannelData != 0 | actualData != 0) {
    //   std::cout << "Got nChannelData: " << nChannelData
    //             << ", actualData: " << actualData << std::endl;
    // }

    run++;
  }
}

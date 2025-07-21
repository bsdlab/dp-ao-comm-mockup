#include <ao_comm/ao_to_lsl.hpp>
#include <ao_comm/tcp_server.hpp>
#include <ao_comm/toml.hpp>
#include <ao_comm_deps/lsl/include/lsl_cpp.h>
#include <atomic>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char *argv[]) {
  std::cout << "Started app" << std::endl;

  // read the config
  toml::table config;
  try {
    config = toml::parse_file("./config/general.toml");
  } catch (const toml::parse_error &err) {
    std::cerr << "Parsing failed:\n" << err << "\n";
    return 1;
  }

  // Connect to the Neuro Omega and collect data
  int mac_adress_iter[6]; // = {0x6c, 0xec, 0xeb, 0xff, 0xeb, 0x18};
  for (int i = 0; i < 6; i++) {
    mac_adress_iter[i] = config["AO"]["ao_mac"][i].as_integer()->get();
  }
  connect_to_ao(mac_adress_iter, 6);

  // Spawn the LSL stream (only data outlet for now, add the function outlet in
  // a later iteration as it was never used before anyways)

  const char *stream_name =
      config["LSL"]["out_lsl_stream_name"].as_string()->get().c_str();
  int num_channels = config["AO"]["channels"].as_array()->size();
  int sampling_rate_hz = config["LSL"]["sampling_rate_hz"].as_integer()->get();
  int buffer_size_lsl_s =
      config["LSL"]["max_buffer_size_s"].as_integer()->get();
  int refresh_rate_ms = config["AO"]["refresh_rate_ms"].as_integer()->get();
  int buffer_size_ao_ms = config["AO"]["buffer_size_ms"].as_integer()->get();

  // convert channesl array to simple int[]
  int channels_arr[256]; // large enough for the AO to avoid dynamic allocation
                         // with `num_channels`

  // Loop through the TOML array and assign values to the regular int[] array
  for (int i = 0; i < num_channels; ++i) {
    channels_arr[i] = config["AO"]["channels"][i].as_integer()->get();
  }

  std::cout << "Trying to start LSL stream" << std::endl;

  std::cout << "Starting LSL outlet with:"
            << "\n   stream_name: " << stream_name
            << "\n   num_channels:" << num_channels
            << "\n   sampling_rate_hz:" << sampling_rate_hz
            << "\n   buffer_size_s:" << buffer_size_lsl_s << std::endl;

  lsl::stream_info dataOutletInfo =
      lsl::stream_info(stream_name, "LFP_ECOG_EOG", num_channels,
                       sampling_rate_hz, lsl::cf_float32, "id_ao_stream");

  lsl::stream_outlet *data_lsl_outlet =
      new lsl::stream_outlet(dataOutletInfo, 1, buffer_size_lsl_s);

  std::cout << "LSL connection started - outlet at " << data_lsl_outlet
            << std::endl;

  // Spawn the TCP socket in separate thread
  std::atomic_bool stop_thread = false;
  std::thread tcp_th(
      run_server, config["Server"]["ip"].as_string()->get().c_str(),
      config["Server"]["port"].as_integer()->get(), std::ref(stop_thread),
      config["Server"]["sleep_ms"].as_integer()->get());

  //  // Run the data loopre
  std::cout << "Starting to stream to LSL with " << (1000 / refresh_rate_ms)
            << "Hz intervals" << std::endl;

  int warn_after_empty_calls = 10;
  stream_ao_data_to_lsl(data_lsl_outlet, channels_arr, num_channels,
                        std::ref(stop_thread), refresh_rate_ms,
                        buffer_size_ao_ms, warn_after_empty_calls);

  // // Clean up
  std::cout << "Joining threads" << std::endl;
  tcp_th.join();
  delete data_lsl_outlet;

  return 0;
}

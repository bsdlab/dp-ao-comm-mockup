#ifndef AO_TO_LSL
#define AO_TO_LSL

#include <ao_comm_deps/lsl/include/lsl_cpp.h>
#include <cstddef>

void connect_to_ao(int *mac_adress_iter, size_t size_of_iter);

void stream_ao_data_to_lsl(lsl::stream_outlet *outlet, int *channels_arr,
                           int num_channels, int refresh_rate_ms,
                           int buffer_size, int warn_after_n_empty_calls);
#endif // AO_TO_LSL

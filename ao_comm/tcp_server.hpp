#include <atomic>

#ifndef RECEIVER_H
#define RECEIVER_H

void run_server(const char *ip_addr, int port, std::atomic_bool &stop_thread,
                int sleep_ms = 1);

#endif // RECEIVER_H

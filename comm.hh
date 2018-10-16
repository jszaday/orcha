#pragma once

#include <mpi.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <future>
#include <mutex>
#include <thread>

namespace orcha {
  using id_t = std::uint64_t;

  namespace comm {
    const int REQUEST = 0;
    const int MAX_LEN = 1;
    const int MPI_PERIOD_MS = 100;

    extern std::thread k_comms_;
    extern std::mutex k_mpi_lock;
    extern std::atomic<bool> k_running_;

    using comm_t = MPI::Intracomm; // MPI::Comm;
    const comm_t& global(void);

    // extern decltype(MPI::COMM_WORLD) k_global;
    void send_value(const comm_t &comm, int source, int tag);
    std::string receive_value(const comm_t &comm, int source, int tag);
    std::future<std::string> request_value(const comm_t &comm, int source, int tag);
    bool poll_for_message(const comm_t &comm);

    int rank(const comm_t &comm);
    int size(const comm_t &comm);
    void initialize(int &argc, char** &argv);
    void finalize(void);
    void barrier(const orcha::comm::comm_t &comm);

    id_t make_tag(int source, int tag);
  }
}

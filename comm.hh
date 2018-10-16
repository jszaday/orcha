#pragma once

#include <mpi.h>

#include "distrib.hh"

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <mutex>
#include <thread>

namespace orcha {
  namespace comm {
    const int REQUEST = 0;
    const int MAX_LEN = 1;
    const int MPI_PERIOD_MS = 100;

    using comm_t  = MPI::Comm;
    extern decltype(MPI::COMM_WORLD) k_global;

    extern std::thread k_comms_;
    extern std::mutex k_mpi_lock;
    extern std::atomic<bool> k_running_;

    void send_value(const comm_t &comm, int source, int tag);
    std::string receive_value(const comm_t &comm, int source, int tag);
    std::future<std::string> request_value(const comm_t &comm, int source, int tag);
    bool poll_for_message(const comm_t &comm);

    int rank(const comm_t &comm);
    int size(const comm_t &comm);
    const comm_t& global(void);
    void initialize(int &argc, char** &argv);
    void finalize(void);
    void barrier(const orcha::comm::comm_t &comm);

    id_t make_tag(int source, int tag);
  }
}

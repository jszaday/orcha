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
    const char REQUEST = 'R';
    const char VALUE = 'V';
    const int MPI_PERIOD_US = 10;

    extern std::thread k_comms_;
    extern std::mutex k_mpi_lock;
    extern std::atomic<bool> k_running_;

    using comm_t = MPI::Intracomm; // MPI::Comm;
    const comm_t& global(void);

    std::future<std::string> request_value(id_t tag, bool send = true);

    // extern decltype(MPI::COMM_WORLD) k_global;
    void send_value(const comm_t& comm, int source, int tag);
    void send_buffer(const comm_t& comm, int source, int tag, const std::string& buffer);
    void receive_value(const orcha::comm::comm_t& comm, int source, int tag, char* buffer, int count);
    bool poll_for_message(const comm_t& comm);
    void send_requests(const comm_t& comm);

    void broadcast(const comm_t& comm, int source, std::string& buffer);

    int rank(const comm_t& comm);
    int size(const comm_t& comm);
    void initialize(int& argc, char**& argv);
    void finalize(void);
    void barrier(const orcha::comm::comm_t& comm);

    id_t make_tag(int source, int tag);
}
}

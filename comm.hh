#pragma once

#include "core.hh"

#include <mpi.h>
#include <chrono>
#include <exception>

namespace orcha {
  namespace comm {
    const int REQUEST = 0;
    const int MAX_LEN = 1;

    std::mutex k_mpi_lock;

    using comm_t  = MPI::Comm;
    auto k_global = MPI::COMM_WORLD;

    void send_value(const comm_t &comm, int source, int tag);
    std::string receive_value(const comm_t &comm, int source, int tag);
    std::future<std::string> request_value(const comm_t &comm, int source, int tag);
    bool wait_on_message(const comm_t &comm, int period);

    inline int rank(const comm_t &comm);
    inline int size(const comm_t &comm);
    inline const comm_t& global(void);
    inline void initialize(int &argc, char** &argv);
    inline void finalize(void);

    inline id_t make_tag(int source, int tag);
  }
}

inline orcha::id_t orcha::comm::make_tag(int source, int tag) {
  return (((orcha::id_t)source) << 32) | tag;
}

std::string orcha::comm::receive_value(const orcha::comm::comm_t &comm, int source, int tag) {
  MPI::Status status;
  comm.Probe(source, tag, status);
  auto  count = status.Get_count(MPI::CHAR);
  auto buffer = new char[count];
  comm.Recv(buffer, count, MPI::CHAR, source, tag);
  auto s = std::string(buffer, count);
  delete buffer;
  return s;
}

std::future<std::string> orcha::comm::request_value(const orcha::comm::comm_t &comm, int source, int tag) {
  return std::move(std::async(std::launch::async, [&comm, source, tag] {
    std::lock_guard<std::mutex> guard(k_mpi_lock);
    comm.Send(&REQUEST, 1, MPI::INT, source, tag);
    return receive_value(comm, source, tag);
  }));
}

bool orcha::comm::wait_on_message(const orcha::comm::comm_t &comm, int period) {
  std::lock_guard<std::mutex> guard(k_mpi_lock);
  std::this_thread::sleep_for(std::chrono::milliseconds(period));
  if (comm.Iprobe(MPI::ANY_SOURCE, MPI::ANY_TAG)) {
    MPI::Status status; int buffer[MAX_LEN];
    comm.Recv(buffer, MAX_LEN, MPI::INT, MPI::ANY_SOURCE, MPI::ANY_TAG, status);
    switch(buffer[0]) {
      case REQUEST:
        send_value(comm, status.Get_source(), status.Get_tag());
        return true;
    }
  }
  return false;
}

void orcha::comm::send_value(const orcha::comm::comm_t &comm, int source, int tag) {
  id_t tag_ = make_tag(source, tag);
  if (k_pending_.find(tag_) == k_pending_.end()) {
    throw std::runtime_error("could not locate tag");
  }
  auto fut = std::move(k_pending_[tag_]);
  fut.wait(); auto val = fut.get();
  comm.Send(val.c_str(), val.size(), MPI::CHAR, source, tag);
}

inline int orcha::comm::rank(const orcha::comm::comm_t &comm) {
  return comm.Get_rank();
}

inline int orcha::comm::size(const orcha::comm::comm_t &comm) {
  return comm.Get_size();
}

inline void orcha::comm::initialize(int &argc, char** &argv) {
  int provided = MPI::Init_thread(argc, argv, MPI::THREAD_SERIALIZED);

  if (!(provided == MPI::THREAD_SERIALIZED || provided == MPI::THREAD_MULTIPLE)) {
    throw std::runtime_error("unsupported mpi thread support level");
  }
}

inline void orcha::comm::finalize(void) {
  MPI::Finalize();
}

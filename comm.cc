#include "comm.hh"
#include "core.hh"

namespace orcha {
  namespace comm {
    std::thread k_comms_;
    std::mutex k_mpi_lock;
    std::atomic<bool> k_running_;

    // decltype(MPI::COMM_WORLD) global() = MPI::COMM_WORLD;
  }
}

const orcha::comm::comm_t& orcha::comm::global(void) {
  static MPI::Intracomm k_global_ = MPI::COMM_WORLD;
  return k_global_;
}

orcha::id_t orcha::comm::make_tag(int source, int tag) {
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

bool orcha::comm::poll_for_message(const orcha::comm::comm_t &comm) {
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
  id_t tag_ = make_tag(comm.Get_rank(), tag);
  k_pending_lock_.lock();
  if (k_pending_.find(tag_) == k_pending_.end()) {
    throw std::runtime_error("could not locate tag");
  }
  auto fut = std::move(k_pending_[tag_]);
  k_pending_.erase(tag_);
  k_pending_lock_.unlock();
  fut.wait(); auto val = fut.get();
  comm.Send(val.c_str(), val.size(), MPI::CHAR, source, tag);
}

int orcha::comm::rank(const orcha::comm::comm_t &comm) {
  return comm.Get_rank();
}

int orcha::comm::size(const orcha::comm::comm_t &comm) {
  return comm.Get_size();
}

void orcha::comm::initialize(int &argc, char** &argv) {
  int provided = MPI::Init_thread(argc, argv, MPI::THREAD_SERIALIZED);
  if (!(provided == MPI::THREAD_SERIALIZED || provided == MPI::THREAD_MULTIPLE)) {
    throw std::runtime_error("unsupported mpi thread support level");
  }
  k_running_.store(true, std::memory_order_release);
  // start the communications thread
  k_comms_ = std::move(std::thread([] {
    // while we are still running
    while (k_running_.load(std::memory_order_acquire)) {
      // wait for MPI_PERIOD_MS milliseconds
      std::this_thread::sleep_for(std::chrono::milliseconds(MPI_PERIOD_MS));
      // poll for a message
      std::lock_guard<std::mutex> guard(k_mpi_lock);
      poll_for_message(global());
    }
  }));
}

void orcha::comm::finalize(void) {
  k_running_.store(false, std::memory_order_release);
  k_comms_.join();
  std::lock_guard<std::mutex> guard(k_mpi_lock);
  MPI::Finalize();
}

void orcha::comm::barrier(const orcha::comm::comm_t &comm) {
  bool flag;

  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(MPI_PERIOD_MS));
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    flag = k_pending_.size() != 0;
  } while (flag);

  std::lock_guard<std::mutex> guard(k_mpi_lock);
  comm.Barrier();
}

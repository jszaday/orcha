#include "comm.hh"
#include "core.hh"

namespace orcha {
  namespace comm {
    std::thread k_comms_;
    std::mutex k_mpi_lock;
    std::mutex k_req_lock;
    std::atomic<bool> k_running_;
    std::map<id_t, std::promise<std::string>> k_requests_;
    std::vector<id_t> k_request_queue_;
  }
}

const orcha::comm::comm_t& orcha::comm::global(void) {
  static MPI::Intracomm k_global_ = MPI::COMM_WORLD;
  return k_global_;
}

orcha::id_t orcha::comm::make_tag(int source, int tag) {
  return (((orcha::id_t)source) << 32) | tag;
}

std::future<std::string> orcha::comm::request_value(orcha::id_t tag) {
  std::lock_guard<std::mutex> guard(k_req_lock);
  k_request_queue_.emplace_back(tag);
  k_requests_[tag] = std::promise<std::string>();
  return std::move(k_requests_[tag].get_future());
}

bool orcha::comm::poll_for_message(const orcha::comm::comm_t &comm) {
  if (comm.Iprobe(MPI::ANY_SOURCE, MPI::ANY_TAG)) {
    MPI::Status status; comm.Probe(MPI::ANY_SOURCE, MPI::ANY_TAG, status);
    auto count  = status.Get_count(MPI::CHAR),
         source = status.Get_source(), tag = status.Get_tag();
    auto buffer = new char[count];
    comm.Recv(buffer, count, MPI::CHAR, source, tag, status);
    switch(buffer[0]) {
      case REQUEST:
        boost::asio::post(k_pool_, std::bind(send_value, comm, source, tag)); break;
      case VALUE:
        receive_value(comm, source, tag, buffer + 1, count - 1); break;
      default:
        throw std::runtime_error("unsupported message type " + std::to_string(buffer[0]));
    }
    delete buffer;
    return true;
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
  fut.wait(); auto val = fut.get().insert(0, &VALUE, 1);
  std::lock_guard<std::mutex> guard(k_mpi_lock);
  comm.Send(val.c_str(), val.size(), MPI::CHAR, source, tag);
}

void orcha::comm::receive_value(const orcha::comm::comm_t &comm, int source, int tag, char* buffer, int count) {
  auto tag_ = make_tag(source, tag);
  k_requests_[tag_].set_value(std::string(buffer, count));
  k_requests_.erase(tag_);
}

void orcha::comm::send_requests(const comm_t &comm) {
  std::lock_guard<std::mutex> guard(k_req_lock);
  while (!k_request_queue_.empty()) {
    auto tag_ = k_request_queue_.back();
    int tag = tag_, source = tag_ >> 32;
    comm.Send(&REQUEST, 1, MPI::CHAR, source, tag);
    k_request_queue_.pop_back();
  }
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
      // wait for MPI_PERIOD_US microseconds
      std::this_thread::sleep_for(std::chrono::microseconds(MPI_PERIOD_US));
      // poll for a message
      std::lock_guard<std::mutex> guard(k_mpi_lock);
      send_requests(global());
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
  k_pool_.join();
  std::lock_guard<std::mutex> guard(k_mpi_lock);
  comm.Barrier();
}

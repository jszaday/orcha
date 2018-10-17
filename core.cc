#include "core.hh"
#include "comm.hh"

namespace orcha {
  boost::asio::thread_pool k_pool_(k_thread_pool_size_);
  std::map<id_t, std::future<std::string>> k_pending_;
  std::mutex k_pending_lock_;

  void request(id_t tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    k_pending_[tag] = std::move(comm::request_value(comm::global(), tag >> 32, tag));
  }

  void produce(id_t array, id_t entry, id_t element, id_t out_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    k_pending_[out_tag] = boost::asio::post(k_pool_, boost::asio::use_future([entry, element] {
      return (k_funs_[entry](element))({});
    }));
  }

  void consume(id_t array, id_t entry, id_t element, id_t in_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    if (k_pending_.find(in_tag) == k_pending_.end()) {
      request(in_tag);
    }
    boost::asio::post(k_pool_, [entry, element, in_tag] {
      k_pending_lock_.lock();
      auto f = std::move(k_pending_[in_tag]);
      k_pending_.erase(in_tag);
      k_pending_lock_.unlock();
      f.wait(); (k_funs_[entry](element))(f.get());
    });
  }

  void produce_consume(id_t array, id_t entry, id_t element, id_t in_tag, id_t out_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    if (k_pending_.find(in_tag) == k_pending_.end()) {
      request(in_tag);
    }
    k_pending_[out_tag] = boost::asio::post(k_pool_, boost::asio::use_future([entry, element, in_tag] {
      k_pending_lock_.lock();
      auto f = std::move(k_pending_[in_tag]);
      k_pending_.erase(in_tag);
      k_pending_lock_.unlock();
      f.wait(); return (k_funs_[entry](element))(f.get());;
    }));
  }

  id_t fresh_tag(id_t arr, id_t entry, id_t element) {
    static int tag = 0;
    return comm::make_tag(k_arrs_[arr]->node_for(element), tag++);
  }
}

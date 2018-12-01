#include "core.hh"
#include "comm.hh"

namespace orcha {
  boost::asio::thread_pool k_pool_(k_thread_pool_size_);
  std::map<id_t, std::future<std::string>> k_pending_;
  std::mutex k_pending_lock_;

  void produce(id_t array, id_t entry, id_t element, id_t out_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    k_pending_[out_tag] = boost::asio::post(k_pool_, boost::asio::use_future([entry, element] {
      return (k_funs_[entry](element))({});
    }));
  }

  void consume(id_t array, id_t entry, id_t element, id_t in_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    if (k_pending_.find(in_tag) == k_pending_.end()) {
      k_pending_[in_tag] = std::move(comm::request_value(in_tag));
    }
    boost::asio::post(k_pool_, [entry, element, in_tag] {
      k_pending_lock_.lock();
      auto f = std::move(k_pending_[in_tag]);
      k_pending_.erase(in_tag);
      k_pending_lock_.unlock();
      f.wait(); (k_funs_[entry](element))(f.get());
    });
  }

  template<typename T>
  std::future<T> consume_as(id_t in_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    if (k_pending_.find(in_tag) == k_pending_.end()) {
      k_pending_[in_tag] = std::move(comm::request_value(in_tag));
    }
    return boost::asio::post(k_pool_, [in_tag] {
      k_pending_lock_.lock();
      auto f = std::move(k_pending_[in_tag]);
      k_pending_.erase(in_tag);
      k_pending_lock_.unlock();
      f.wait(); auto s = f.get();
      archive::istream_t is(s);
      archive::iarchive_t ia(is);
      T t; ia >> t;
      return t;
    });
  }

  void produce_consume(id_t array, id_t entry, id_t element, id_t in_tag, id_t out_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    if (k_pending_.find(in_tag) == k_pending_.end()) {
      k_pending_[in_tag] = std::move(comm::request_value(in_tag));
    }
    k_pending_[out_tag] = boost::asio::post(k_pool_, boost::asio::use_future([entry, element, in_tag] {
      k_pending_lock_.lock();
      auto f = std::move(k_pending_[in_tag]);
      k_pending_.erase(in_tag);
      k_pending_lock_.unlock();
      f.wait(); return (k_funs_[entry](element))(f.get());;
    }));
  }

  static int tag = 0;
  
  id_t fresh_tag(id_t arr, id_t entry, id_t element) {
    return comm::make_tag(k_arrs_[arr]->node_for(element), tag++);
  }

  id_t fresh_tag(id_t node) {
    return comm::make_tag(node, tag++);
  }
}

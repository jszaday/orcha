#include "core.hh"
#include "comm.hh"

namespace orcha {
  std::map<id_t, std::future<std::string>> k_pending_;
  std::mutex k_pending_lock_;

  static void request(id_t tag) {
    k_pending_[tag] = std::move(comm::request_value(comm::k_global, tag >> 32, tag));
  }

  static void produce(id_t array, id_t entry, id_t element, id_t out_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    k_pending_[out_tag] = std::async(std::launch::async, [entry, element] {
      return (k_funs_[entry](element))({});
    });
  }

  static void consume(id_t array, id_t entry, id_t element, id_t in_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    if (k_pending_.find(in_tag) == k_pending_.end()) {
      request(in_tag);
    }
    std::async(std::launch::async, [entry, element, in_tag] {
      k_pending_lock_.lock();
      auto f = std::move(k_pending_[in_tag]);
      k_pending_.erase(in_tag);
      k_pending_lock_.unlock();
      f.wait(); (k_funs_[entry](element))(f.get());
    });
  }

  static void produce_consume(id_t array, id_t entry, id_t element,
                              id_t in_tag, id_t out_tag) {
    std::lock_guard<std::mutex> guard(k_pending_lock_);
    if (k_pending_.find(in_tag) == k_pending_.end()) {
      request(in_tag);
    }
    k_pending_[out_tag] = std::async(std::launch::async, [entry, element, in_tag] {
      k_pending_lock_.lock();
      auto f = std::move(k_pending_[in_tag]);
      k_pending_.erase(in_tag);
      k_pending_lock_.unlock();
      f.wait(); return (k_funs_[entry](element))(f.get());;
    });
  }
}

template<typename K, typename V, typename R>
inline orcha::TaggedValues<R> orcha::strate(orcha::RegisteredFunction<K, V, R> f, orcha::IndexSpace<K> fis) {
  TaggedValues<R> out_tags;
  for (int i = 0; i < fis.size(); i++) {
    id_t ord = f.ordinal_for(fis[i]),
         out_tag = fresh_tag(f.arr_id(), f.id(), ord);
    if (f.is_local(fis[i])) {
      produce(f.arr_id(), f.id(), ord, out_tag);
    }
    out_tags.push_back(out_tag);
  }
  return std::move(out_tags);
}

template<typename K, typename V, typename T>
inline void orcha::strate(orcha::RegisteredFunction<K, V, void, T> f, orcha::IndexSpace<K> fis, orcha::TaggedValues<T> in_tags) {
  if (fis.size() != in_tags.size()) {
    throw std::out_of_range("size_mismatch");
  }
  for (int i = 0; i < fis.size(); i++) {
    id_t ord = f.ordinal_for(fis[i]), in_tag = in_tags[i];
    if (f.is_local(fis[i])) {
      consume(f.arr_id(), f.id(), ord, in_tag);
    }
  }
}

template<typename K, typename V, typename R, typename T>
inline orcha::TaggedValues<R> orcha::strate(orcha::RegisteredFunction<K, V, R, T> f, orcha::IndexSpace<K> fis, orcha::TaggedValues<T> in_tags) {
  if (fis.size() != in_tags.size()) {
    throw std::out_of_range("size_mismatch");
  }
  TaggedValues<R> out_tags;
  for (int i = 0; i < fis.size(); i++) {
    id_t ord = f.ordinal_for(fis[i]), in_tag = in_tags[i],
         out_tag = fresh_tag(f.arr_id(), f.id(), ord);
    if (f.is_local(fis[i])) {
      produce_consume(f.arr_id(), f.id(), ord, in_tag, out_tag);
    }
    out_tags.push_back(out_tag);
  }
  return std::move(out_tags);
}

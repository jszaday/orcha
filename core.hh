#pragma once

#include "distrib.hh"

#include <future>
#include <mutex>

namespace orcha {
  extern std::map<id_t, std::future<std::string>> k_pending_;
  extern std::mutex k_pending_lock_;

  template<typename T>
  class TaggedValues {
  public:
    using vector_type = std::vector<id_t>;
    using size_type   = typename vector_type::size_type;

    TaggedValues() { }

    TaggedValues(TaggedValues<T> &o) {
      base_ = o.base_;
    }

    TaggedValues(TaggedValues<T> &&o) {
      base_ = std::move(o.base_);
    }

    inline void push_back(const id_t& tag) {
      base_.push_back(tag);
    }

    inline size_type size() const {
      return base_.size();
    }

    inline const T operator[] (const size_type& i) const {
      return base_[i];
    }
  private:
    vector_type base_;
  };

  inline id_t fresh_tag(id_t arr, id_t entry, id_t ord) {
    static id_t tag = 0;
    return tag++;
  }

  template<typename K, typename V, typename R>
  inline TaggedValues<R> strate(RegisteredFunction<K, V, R> f, IndexSpace<K> fis);

  template<typename K, typename V, typename T>
  inline void strate(RegisteredFunction<K, V, void, T> f, IndexSpace<K> fis, TaggedValues<T> in_tags);

  template<typename K, typename V, typename R, typename T>
  inline TaggedValues<R> strate(RegisteredFunction<K, V, R, T> f, IndexSpace<K> fis, TaggedValues<T> in_tags);
}

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

  void request(id_t tag);
  void produce(id_t array, id_t entry, id_t element, id_t out_tag);
  void consume(id_t array, id_t entry, id_t element, id_t in_tag);
  void produce_consume(id_t array, id_t entry, id_t element, id_t in_tag, id_t out_tag);

  id_t fresh_tag(id_t arr, id_t entry, id_t element);

  template<typename K, typename V, typename R>
  inline TaggedValues<R> strate(RegisteredFunction<K, V, R> f, IndexSpace<K> fis) {
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
  inline void strate(RegisteredFunction<K, V, void, T> f, IndexSpace<K> fis, TaggedValues<T> in_tags) {
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
  inline TaggedValues<R> strate(RegisteredFunction<K, V, R, T> f, IndexSpace<K> fis, TaggedValues<T> in_tags){
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
}

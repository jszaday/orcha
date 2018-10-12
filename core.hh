#pragma once

#include "distrib.hh"

namespace orcha {
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

    inline const T& operator[] (const size_type& i) const {
      return base_[i];
    }
  private:
    vector_type base_;
  };

  inline id_t fresh_tag(id_t arr, id_t entry, id_t ord) {
    static id_t tag = 0;
    return ++tag;
  }

  static void produce(id_t array, id_t entry, id_t element, id_t out_tag) {

  }

  static void consume(id_t array, id_t entry, id_t element, id_t in_tag) {

  }

  static void produce_consume(id_t array, id_t entry, id_t element,
                              id_t in_tag, id_t out_tag) {

  }

  template<typename K, typename R>
  inline TaggedValues<R> strate(RegisteredFunction<K, R, void> f, IndexSpace<K> fis) {
    TaggedValues<R> out_tags;
    for (int i = 0; i < fis.size(); i++) {
      id_t ord = fis.ordinal_for(fis[i]),
           out_tag = fresh_tag(f.arr_id_, f.id_, ord);
      if (f.is_local(fis[i])) {
        produce(f.arr_id_, f.id_, ord, out_tag);
      }
      out_tags.push_back(out_tag);
    }
    return std::move(out_tags);
  }

  template<typename K, typename T>
  inline void strate(RegisteredFunction<K, void, T> f, IndexSpace<K> fis, TaggedValues<T> in_tags) {
    if (fis.size() != in_tags.size()) {
      throw std::out_of_range("size_mismatch");
    }
    for (int i = 0; i < fis.size(); i++) {
      id_t ord = fis.ordinal_for(fis[i]), in_tag = in_tags[i];
      if (f.is_local(fis[i])) {
        consume(f.arr_id_, f.id_, ord, in_tag);
      }
    }
  }

  template<typename K, typename R, typename T>
  inline TaggedValues<R> strate(RegisteredFunction<K, R, T> f, IndexSpace<K> fis, TaggedValues<T> in_tags) {
    if (fis.size() != in_tags.size()) {
      throw std::out_of_range("size_mismatch");
    }
    TaggedValues<R> out_tags;
    for (int i = 0; i < fis.size(); i++) {
      id_t ord = fis.ordinal_for(fis[i]), in_tag = in_tags[i],
           out_tag = fresh_tag(f.arr_id_, f.id_, ord);
      if (f.is_local(fis[i])) {
        produce_consume(f.arr_id_, f.id_, ord, in_tag, out_tag);
      }
      out_tags.push_back(out_tag);
    }
    return std::move(out_tags);
  }
}

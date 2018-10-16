#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace orcha {

  // TODO make this class lazy as heck
  template<typename T>
  class IndexSpace {
  public:
    using vector_type = std::vector<T>;
    using size_type = typename vector_type::size_type;
    using iterator_type = typename vector_type::const_iterator;

    /* empty constructor */
    IndexSpace(void) { }

    template<typename V, typename F>
    IndexSpace(const V &base, F mapper) {
      base_ = std::vector<T>(base.size());
      std::transform(base.begin(), base.end(), base_.begin(), mapper);
    };

    IndexSpace(iterator_type first, iterator_type last) {
      base_ = std::vector<T>(first, last);
    }

    IndexSpace(const T n) {
      base_ = std::vector<T>(n);
      std::iota(base_.begin(), base_.end(), 0);
    };

    IndexSpace(const T begin, const T end, const int step = 1) {
      base_ = std::vector<T>(int(end - begin) / step);
      size_type j = 0; T i = begin;
      for ( ; i != end ; i += step, j++ ) base_[j] = i;
    };

    inline size_type     size()  const { return base_.size(); }
    inline iterator_type begin() const { return base_.begin(); }
    inline iterator_type end()   const { return base_.end(); }

    inline const size_type index_of(T t) const {
      auto v = std::distance(base_.begin(), find(base_.begin(), base_.end(), t));
      if ( v >= 0 ) {
        return (size_type) v;
      } else {
        throw std::out_of_range("ordinal_of");
      }
    }

    inline const T& operator[] (const size_type& i) const {
      return base_[i];
    }

    inline bool contains(T t) const {
      return std::find(begin(), end(), t) != end();
    }

    inline IndexSpace<T> subspace(const size_type& first, const size_type& last) const {
      return std::move(IndexSpace<T>(begin() + first, begin() + last));
    }

  private:
    vector_type base_;
  };

  inline IndexSpace<size_t> index_space(const size_t begin, const size_t end, const int step = 1) {
    return IndexSpace<size_t>(begin, end, step);
  }

  inline IndexSpace<size_t> index_space(const size_t n) {
    return IndexSpace<size_t>(n);
  }

  template<typename T, typename V, typename F>
  inline IndexSpace<T> map(const V &base, const F &&mapper) {
    return IndexSpace<T>(base, mapper);
  }
}

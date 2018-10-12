#pragma once

#include "ispace.hh"
#include "archive.hh"
#include "functional.hh"

#include <map>
#include <memory>
#include <utility>

namespace orcha {

  namespace mapping {
    template<typename K>
    IndexSpace<K> automatic(const IndexSpace<K> &is) {
      return is;
    }
  }

  class AbstractArray {
  public:
    virtual ~AbstractArray() {}
  };

  using id_t  = std::uint64_t;
  using arr_t = std::shared_ptr<AbstractArray>;
  using reg_t = std::function<archive::function_t(id_t)>;

  std::vector<arr_t> k_arrs_;
  std::vector<reg_t> k_funs_;

  template<typename K, typename V>
  class DistributedArray : public AbstractArray {
    const id_t id_; std::map<K, V*> local_;
    const IndexSpace<K> global_;
  public:
    // TODO fix mapping strategy!!
    template<typename S, typename... As>
    DistributedArray(id_t id, S strategy, const IndexSpace<K> &is, As... args)
    : id_(id), global_(is) {
      for (auto i : is) {
        local_[i] = new V(i, &args...);
      }
    }

    ~DistributedArray() {
      for (auto kv : local_) {
        delete kv.second;
      }
    }

    inline bool is_local(K i) const {
      return local_.find(i) != local_.end();
    }

    inline V& operator[] (const K& k) {
      return *local_[k];
    };

    inline id_t id() const {
      return id_;
    }

    inline const id_t ordinal_for(K k) {
      return global_.index_of(k);
    }

    inline const K& key_for(id_t i) {
      return global_[i];
    }
  };

  template<class K, class V, class R, class... As>
  class RegisteredFunction {
    const id_t id_;
    const std::shared_ptr<DistributedArray<K, V>> arr_;
  public:
    RegisteredFunction(id_t id, std::shared_ptr<DistributedArray<K, V>> arr)
    : id_(id), arr_(arr) { }

    inline const id_t id() const{
      return id_;
    }

    inline const id_t arr_id() const {
      return arr_->id();
    }

    inline const id_t ordinal_for(K k) const {
      return arr_->ordinal_for(k);
    }

    inline bool is_local(K k) const {
      return arr_->is_local(k);
    }
  };

  template<typename V, typename K, typename S, typename... As>
  std::shared_ptr<DistributedArray<K, V>> distribute(S strategy, const IndexSpace<K> &is, As... args) {
    std::shared_ptr<DistributedArray<K, V>> arr = std::make_shared<DistributedArray<K, V>>(k_arrs_.size(), &strategy, is, &args...);
    k_arrs_.push_back(arr);
    return std::move(arr);
  }

  template<typename K, typename V, typename R, typename... As>
  RegisteredFunction<K, V, R, As...> register_function(std::shared_ptr<DistributedArray<K, V>> arr, R(V::*p)(As...)) {
    reg_t f = [arr, p] (id_t i) {
      return archive::wrap(functional::bind(p, (*arr)[arr->key_for(i)]));
    };
    k_funs_.push_back(std::move(f));
    return RegisteredFunction<K, V, R, As...>(k_funs_.size() - 1, arr);
  }
}

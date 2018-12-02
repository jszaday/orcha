#pragma once

#include "archive.hh"
#include "comm.hh"
#include "functional.hh"
#include "ispace.hh"

#include <map>
#include <memory>
#include <utility>

namespace orcha {

template <typename K>
class Mapping {
public:
    virtual K key_for(id_t i) const = 0;
    virtual id_t is_local(K k) const = 0;
    virtual id_t ordinal_for(K k) const = 0;
    virtual id_t node_for(id_t element) const = 0;
    virtual const IndexSpace<K>& local(void) const = 0;
    virtual const IndexSpace<K>& global(void) const = 0;

    id_t node_for(K k)
    {
        return node_for(ordinal_for(k));
    };
};

template <typename K>
class AutoMap : public Mapping<K> {
    const IndexSpace<K> global_;
    IndexSpace<K> local_;

public:
    AutoMap(const IndexSpace<K>& is)
        : global_(is)
    {
        auto rank = comm::rank(comm::global()),
             size = comm::size(comm::global());
        if (size < global_.size()) {
            auto spacing = global_.size() / size;
            auto start = spacing * rank,
                 end = (rank + 1 == size) ? global_.size() : spacing * (rank + 1);
            local_ = std::move(global_.subspace(start, end));
        } else if (rank < global_.size()) {
            local_ = std::move(global_.subspace(rank, rank + 1));
        }
    }

    K key_for(id_t i) const override
    {
        return global_[i];
    }

    id_t is_local(K k) const override
    {
        return local_.contains(k);
    }

    id_t ordinal_for(K k) const override
    {
        return global_.index_of(k);
    }

    const IndexSpace<K>& local(void) const override
    {
        return local_;
    }

    const IndexSpace<K>& global(void) const override
    {
        return global_;
    }

    id_t node_for(id_t element) const override
    {
        auto spacing = global_.size() / comm::size(comm::global());
        return element / spacing;
    }
};

template <typename K>
const Mapping<K>* automap(const IndexSpace<K>& is)
{
    return new AutoMap<K>(is);
}

class AbstractArray {
public:
    virtual id_t node_for(id_t element) const = 0;
};

using arr_t = std::shared_ptr<AbstractArray>;
using reg_t = std::function<archive::function_t(id_t)>;

extern std::vector<arr_t> k_arrs_;
extern std::vector<reg_t> k_funs_;

template <typename K, typename V>
class DistributedArray : public AbstractArray {
    const id_t id_;
    std::map<K, V*> local_;
    const Mapping<K>* mapping_;

public:
    template <typename... As>
    DistributedArray(id_t id, const Mapping<K>* mapping, As... args)
        : id_(id)
        , mapping_(mapping)
    {
        for (auto i : mapping_->local()) {
            local_[i] = new V(i, args...);
        }
    }

    ~DistributedArray()
    {
        for (auto kv : local_) {
            delete kv.second;
        }
        if (mapping_)
            delete mapping_;
    }

    inline bool is_local(K k) const
    {
        return mapping_->is_local(k);
    }

    inline V* operator[](const K& k)
    {
        return local_[k];
    };

    inline id_t id() const
    {
        return id_;
    }

    inline const id_t ordinal_for(K k)
    {
        return mapping_->ordinal_for(k);
    }

    inline const K key_for(id_t i)
    {
        return mapping_->key_for(i);
    }

    id_t node_for(id_t element) const override
    {
        return mapping_->node_for(element);
    }
};

template <class K, class V, class R, class... As>
class RegisteredFunction {
    const id_t id_;
    const std::shared_ptr<DistributedArray<K, V>> arr_;

public:
    RegisteredFunction(id_t id, std::shared_ptr<DistributedArray<K, V>> arr)
        : id_(id)
        , arr_(arr)
    {
    }

    inline const id_t id() const
    {
        return id_;
    }

    inline const id_t arr_id() const
    {
        return arr_->id();
    }

    inline const id_t ordinal_for(K k) const
    {
        return arr_->ordinal_for(k);
    }

    inline bool is_local(K k) const
    {
        return arr_->is_local(k);
    }
};

template <typename V, typename K, typename... As>
std::shared_ptr<DistributedArray<K, V>> distribute(const Mapping<K>* mapping, As... args)
{
    std::shared_ptr<DistributedArray<K, V>> arr = std::make_shared<DistributedArray<K, V>>(k_arrs_.size(), mapping, args...);
    k_arrs_.push_back(arr);
    return std::move(arr);
}

template <typename K, typename V, typename R, typename... As>
RegisteredFunction<K, V, R, As...> register_function(std::shared_ptr<DistributedArray<K, V>> arr, R (V::*p)(As...))
{
    reg_t f = [arr, p](id_t i) {
        return archive::wrap(functional::bind<V>(p, (*arr)[arr->key_for(i)]));
    };
    k_funs_.push_back(std::move(f));
    return RegisteredFunction<K, V, R, As...>(k_funs_.size() - 1, arr);
}
}

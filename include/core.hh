#pragma once

#include "distrib.hh"

#include <boost/asio.hpp>
#include <future>
#include <condition_variable>
#include <mutex>

namespace orcha {
extern std::map<id_t, std::future<std::string>> k_pending_;
extern std::mutex k_pending_lock_;
extern std::condition_variable k_pending_var_;
extern boost::asio::thread_pool k_pool_;

const int k_thread_pool_size_ = 4;

template <typename T>
class TaggedValues {
public:
    using value_type = id_t;
    using vector_type = std::vector<value_type>;
    using size_type = typename vector_type::size_type;

    TaggedValues() {}

    TaggedValues(TaggedValues<T>& o)
    {
        base_ = o.base_;
    }

    TaggedValues(TaggedValues<T>&& o)
    {
        base_ = std::move(o.base_);
    }

    inline void push_back(const id_t& tag)
    {
        base_.push_back(tag);
    }

    inline size_type size() const
    {
        return base_.size();
    }

    inline const value_type operator[](const size_type& i) const
    {
        return base_[i];
    }

private:
    vector_type base_;
};

void produce(id_t array, id_t entry, id_t element, id_t out_tag);
void consume(id_t array, id_t entry, id_t element, id_t in_tag);
void produce_consume(id_t array, id_t entry, id_t element, id_t in_tag, id_t out_tag);

inline void barrier(void)
{
    comm::barrier(comm::global());
}

template <typename T>
std::future<T> consume_as(id_t in_tag)
{
    if (k_pending_.find(in_tag) == k_pending_.end()) {
        k_pending_lock_.lock();
        k_pending_[in_tag] = std::move(comm::request_value(in_tag));
        k_pending_lock_.unlock();
    }
    return std::async(std::launch::deferred, [in_tag]() -> T {
        k_pending_lock_.lock();
        auto f = std::move(k_pending_[in_tag]);
        k_pending_.erase(in_tag);
        k_pending_lock_.unlock();
        f.wait();
        auto s = f.get();
        archive::istream_t is(s);
        archive::iarchive_t ia(is);
        T t;
        ia >> t;
        return t;
    });
}

id_t fresh_tag(id_t arr, id_t entry, id_t element);
id_t fresh_tag(id_t node);

template <typename T, typename U>
inline TaggedValues<std::tuple<T, U>> zip2(TaggedValues<T> ts, TaggedValues<U> us)
{
    TaggedValues<std::tuple<T, U>> out_tags;
    if (ts.size() != us.size()) {
        throw std::out_of_range("size_mismatch");
    }
    auto my_node = comm::rank(comm::global());
    auto per_node = ts.size() / comm::size(comm::global());
    for (int i = 0; i < ts.size(); i++) {
        id_t which = i / per_node;
        id_t out_tag = fresh_tag(which);
        if (which == my_node) {
            auto t = ts[i], u = us[i];
            std::function<std::string(void)> f = [t, u]() {
                auto tv = consume_as<T>(t),
                     uv = consume_as<U>(u);
                tv.wait();
                uv.wait();
                archive::ostream_t os;
                archive::oarchive_t oa(os);
                oa << std::make_tuple(tv.get(), uv.get());
                return os.str();
            };
            k_pending_lock_.lock();
            k_pending_[out_tag] = boost::asio::post(k_pool_, boost::asio::use_future(f));
            k_pending_lock_.unlock();
            k_pending_var_.notify_all();
        }
        out_tags.push_back(out_tag);
    }
    return std::move(out_tags);
}

template <typename T, typename U, typename V, typename W>
inline TaggedValues<std::tuple<T, U, V, W>> zip4(TaggedValues<T> ts, TaggedValues<U> us, TaggedValues<V> vs, TaggedValues<W> ws)
{
    TaggedValues<std::tuple<T, U, V, W>> out_tags;
    if (ts.size() != us.size() || vs.size() != ws.size() || ts.size() != vs.size()) {
        throw std::out_of_range("size_mismatch");
    }
    auto my_node = comm::rank(comm::global());
    auto per_node = ts.size() / comm::size(comm::global());
    for (int i = 0; i < ts.size(); i++) {
        id_t which = i / per_node;
        id_t out_tag = fresh_tag(which);
        if (which == my_node) {
            auto t = ts[i], u = us[i], v = vs[i], w = ws[i];
            std::function<std::string(void)> f = [t, u, v, w]() {
                auto tv = consume_as<T>(t),
                     uv = consume_as<U>(u),
                     vv = consume_as<V>(v),
                     wv = consume_as<W>(w);
                tv.wait();
                uv.wait();
                vv.wait();
                wv.wait();
                archive::ostream_t os;
                archive::oarchive_t oa(os);
                oa << std::make_tuple(tv.get(), uv.get(), vv.get(), wv.get());
                return os.str();
            };
            k_pending_lock_.lock();
            k_pending_[out_tag] = boost::asio::post(k_pool_, boost::asio::use_future(f));
            k_pending_lock_.unlock();
            k_pending_var_.notify_all();
        }
        out_tags.push_back(out_tag);
    }
    return std::move(out_tags);
}

template <typename K, typename V, typename R>
inline TaggedValues<R> strate(RegisteredFunction<K, V, R> f, IndexSpace<K> fis)
{
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

template <typename K, typename V, typename T>
inline void strate(RegisteredFunction<K, V, void, T> f, IndexSpace<K> fis, TaggedValues<T> in_tags)
{
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

template <typename K, typename V, typename R, typename T>
inline TaggedValues<R> strate(RegisteredFunction<K, V, R, T> f, IndexSpace<K> fis, TaggedValues<T> in_tags)
{
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

template <typename K, typename V, typename T>
inline void bcast(RegisteredFunction<K, V, void, T> f, IndexSpace<K> fis, T val)
{
    archive::ostream_t os;
    archive::oarchive_t oa(os);
    oa << val;
    auto sval = os.str();
    for (int i = 0; i < fis.size(); i++) {
        if (f.is_local(fis[i])) {
            id_t element = f.ordinal_for(fis[i]),
                 entry = f.id();
            boost::asio::post(k_pool_, [entry, element, sval] {
                (k_funs_[entry](element))(sval);
            });
        }
    }
}

template <typename T, typename F, typename R>
inline R reduce(TaggedValues<T> ts, F reducer, R init)
{
    R val;
    auto rank = comm::rank(comm::global()),
         size = comm::size(comm::global());
    auto per_node = ts.size() / size;
    id_t src_tag, dst_tag;
    if ((rank % 2) == 0) {
        dst_tag = fresh_tag(rank);
        src_tag = fresh_tag(rank - 1);
    } else {
        src_tag = fresh_tag(rank - 1);
        dst_tag = fresh_tag(rank);
    }
    if (rank == 0) {
        val = init;
    } else {
        auto s = comm::request_value(src_tag, false).get();
        archive::istream_t is(s);
        archive::iarchive_t ia(is);
        ia >> val;
    }
    for (int i = rank * per_node; i < (rank + 1) * per_node; i++) {
        val = reducer(val, consume_as<T>(ts[i]).get());
    }
    archive::ostream_t os;
    archive::oarchive_t oa(os);
    oa << val;
    std::string s = os.str();
    if (rank == (size - 1)) {
        comm::broadcast(comm::global(), rank, s);
    } else {
        comm::send_buffer(comm::global(), rank + 1, dst_tag, s);
        comm::broadcast(comm::global(), size - 1, s);
        archive::istream_t is(s);
        archive::iarchive_t ia(is);
        ia >> val;
    }
    return val;
}
}

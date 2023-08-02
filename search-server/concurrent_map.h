#pragma once
#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <execution>
#include <atomic>
#include <string>
#include <iostream>

//#include "log_duration.h"
//#include "test_framework.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);
    
    struct Bucket {
        std::map <Key, Value> map_;
        std::mutex mtx;
    };

    struct Access {
        explicit Access(size_t key, Bucket& bucket) 
        : guard(bucket.mtx), ref_to_value(bucket.map_[key]) {
        }
        std::lock_guard <std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) 
    : vec(bucket_count) {
        //разделить на backet_count частей
    }

    Access operator[](const Key& key) {
        size_t what_bucket = static_cast <uint64_t> (key) % vec.size();
        return Access(key, vec.at(what_bucket));
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        //соединить все части в один map 
        std::map<Key, Value> result;
        for (auto& bucket : vec) {
            std::lock_guard guard_(bucket.mtx);
            result.insert(bucket.map_.begin(), bucket.map_.end());
        }
        return result;
    }
    
    

private:
    // ...
    std::vector <Bucket> vec;
};
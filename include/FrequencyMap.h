//
// Created by cc on 2020/1/9.
//

#ifndef QTALK_V2_FrequencyMap_H
#define QTALK_V2_FrequencyMap_H

#include <list>
#include <vector>
#include <algorithm>
#include <map>
#include <mutex>
#include <iostream>
#include <functional>

template <typename T, typename R>
class FrequencyMap {
public:
    explicit FrequencyMap(unsigned int maxSize,
            std::function<void(const T& t, const R& r)> del_func = {}) {
        _maxListSize = maxSize;
        _del_func = del_func;
    }

public:
    void insert(const T& t, const R& r)
    {
        std::unique_lock<std::mutex> lock(_mutex);

        auto it = std::find_if(_mapItems.begin(), _mapItems.end(), [t](auto &pair){
            return pair.first == t;
        });

        if(it != _mapItems.end())
            _mapItems.erase(it);

        _mapItems.push_back({t, r});

        while (_mapItems.size() > _maxListSize)
        {
            auto old_item = _mapItems.begin();
            _mapItems.erase(old_item);
            if(_del_func)
                _del_func(old_item->first, old_item->second);
        }
    }

    void update(const T& t)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = std::find_if(_mapItems.begin(), _mapItems.end(), [t](auto &pair){
            return pair.first == t;
        });

        if(it != _mapItems.end())
        {
            R r = it->second;
            _mapItems.erase(it);
            _mapItems.push_back({t, r});
        }
    }

    bool contains(const T& t)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = std::find_if(_mapItems.begin(), _mapItems.end(), [t](auto &pair){
            return pair.first == t;
        });

        return (it != _mapItems.end());
    }

    R get(const T& t)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = std::find_if(_mapItems.begin(), _mapItems.end(), [t](auto &pair){
            return pair.first == t;
        });
        if(it != _mapItems.end())
            return it->second;
        else
            return R();
    }

    size_t size()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _mapItems.size();
    }

    std::vector<T> keys()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        std::vector<T> keys;
        for(const auto& it : _mapItems)
        {
            keys.push_back(it.first);
        }
        return keys;
    }

	std::vector<R> values()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		std::vector<R> vals;
		for(const auto& it : _mapItems) {
			vals.push_back(it.second);
		}
		return vals;
	}

    void remove(const T& t)
    {
        _mutex.lock();
        auto it = std::find_if(_mapItems.begin(), _mapItems.end(), [t](auto &pair){
            return pair.first == t;
        });
        if(it != _mapItems.end())
        {
            _mapItems.erase(it);
            _mutex.unlock();
            _del_func(it->first, it->second);
        } else
            _mutex.unlock();
    }

//    typename std::list<std::pair<T, R> >::iterator begin()
//    {
//        return _mapItems.begin();
//    }
//
//    typename std::list<std::pair<T, R> >::iterator end()
//    {
//        return _mapItems.end();
//    }

private:
    std::list<std::pair<T, R> >  _mapItems; //
    unsigned int                 _maxListSize{};
    std::function<void(const T& t, const R& r)> _del_func;

public:
    std::mutex                    _mutex;
};

#endif //QTALK_V2_FrequencyMap_H

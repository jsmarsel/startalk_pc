//
// Created by cc on 2020/10/30.
//

#ifndef QTALK_V2_NJSON_H
#define QTALK_V2_NJSON_H

#include "json.hpp"
#include <iostream>

using nJson = nlohmann::json;

namespace Json {

    inline nJson parse(const std::string& str) {
        nJson res ;
        try { res = nJson::parse(str); } catch (const nJson::exception& e) {
            std::cerr << "json parse error: " << "\n " << e.what() << std::endl;
        }
        return res;
    }

    template<typename T>
    inline T get(const nJson &obj, const std::string& key, const T& defaultVal = {}) {
        T res = defaultVal;
        try {
            if(obj.contains(key)) {
                res = obj[key].get<T>();
            }
        } catch (const nJson::exception& e) {
            std::cerr << "json get error. source:" << obj.dump() << "\n key:" << key << "\n error:" << e.what() << "\n" << std::endl;
        }
        return res;
    }

    template<typename T>
    inline T convert(const nJson &obj, const T& defaultVal = {}) {
        T res = defaultVal;
        try { res = obj.get<T>(); } catch (const nJson::exception& e) {
            std::cerr << "json convert error. source: " << obj.dump() << "\n error:" << e.what() << std::endl;
        }
        return res;
    }
}

#endif //QTALK_V2_NJSON_H

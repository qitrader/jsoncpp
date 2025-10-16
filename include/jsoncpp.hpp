#ifndef __INK19_JSONCPP_HPP__
#define __INK19_JSONCPP_HPP__

#include "jsoncpp_detail.hpp"
#include <boost/json.hpp>
#include <boost/pfr.hpp>
#include <memory>
#include <type_traits>
#include <concepts>

namespace bj = boost::json;

namespace jsoncpp {

template <typename T> class transform {
public:
  static void trans(const bj::value &jv, T &t) {
    bj::object const &jo = jv.as_object();
    boost::pfr::for_each_field(t, [&](auto &&field, auto index) {
      using FieldType = std::decay_t<decltype(field)>;
      std::string_view field_name = boost::pfr::get_name<index, T>();
      if constexpr (detail::HasAliasFieldName<T>::value) {
        field_name = T::__jsoncpp_alias_name(field_name);
      }

      if (jo.contains(field_name)) {
        const bj::value &jv = jo.at(field_name);
        transform<FieldType>::trans(jv, field);
      }
    });
  }
};

template <> class transform<std::string> {
public:
  static void trans(const bj::value &jv, std::string &t) {
    if (jv.is_string()) {
      t = jv.as_string().c_str();
    } else if (jv.is_int64()) {
      t = std::to_string(jv.as_int64());
    } else if (jv.is_uint64()) {
      t = std::to_string(jv.as_uint64());
    } else if (jv.is_double()) {
      t = std::to_string(jv.as_double());
    } else if (jv.is_bool()) {
      t = jv.as_bool() ? "true" : "false";
    } else {
      throw std::runtime_error("unsupported type");
    }
  }
};

template <typename MV> class transform<std::map<std::string, MV>> {
public:
  static void trans(const bj::value &jv, std::map<std::string, MV> &t) {
    bj::object const &jo = jv.as_object();
    for (auto &[key, value] : jo) {
      MV vt;
      transform<MV>::trans(value, vt);
      t[key] = std::move(vt);
    }
  }
};

template <typename T> class transform<std::shared_ptr<T>> {
public:
  static void trans(const bj::value &jv, std::shared_ptr<T> &t) {
    t = std::make_shared<T>();
    transform<T>::trans(jv, *t);
  }
};

template <typename AV> class transform<std::vector<AV>> {
public:
  static void trans(const bj::value &jv, std::vector<AV> &t) {
    bj::array const &ja = jv.as_array();
    for (auto &value : ja) {
      AV vt;
      transform<AV>::trans(value, vt);
      t.push_back(std::move(vt));
    }
  }
};

template <>
class transform<bool> {
public:
  static void trans(const bj::value &jv, bool &t) {
    if (jv.is_string()) {
      std::string fv = jv.as_string().c_str();
      if (!fv.empty()) {
        // 支持多种真值表示
        if (fv == "true" || fv == "1" || fv == "yes" || fv == "on") {
          t = true;
        } else if (fv == "false" || fv == "0" || fv == "no" || fv == "off") {
          t = false;
        } else {
          // 对于其他字符串，尝试转换为数字再判断
          try {
            int num = std::stoi(fv);
            t = num != 0;
          } catch (const std::exception&) {
            t = false; // 转换失败则返回false
          }
        }
      } else {
        t = false;
      }
    } else if (jv.is_bool()) {
      t = jv.as_bool();
    } else if (jv.is_int64()) {
      t = jv.as_int64() != 0;
    } else {
      throw std::runtime_error("unsupported type");
    }
  }
};

template <std::integral T>
class transform<T> {
public:
  static void trans(const bj::value &jv, T &t) {
    if (jv.is_string()) {
      std::string fv = jv.as_string().c_str();
      if (!fv.empty()) {
        t = std::stoi(fv);
      } else {
        t = 0;
      }
    } else if (jv.is_int64()) {
      t = jv.as_int64();
    } else if (jv.is_uint64()) {
      t = jv.as_uint64();
    } else if (jv.is_double()) {
      t = jv.as_double();
    } else {
      throw std::runtime_error("unsupported type");
    }
  }
};

class StringEnum {
public:
  StringEnum(const std::string &value) : value(value) {}

  bool operator==(const StringEnum &other) const {
    return value == other.value;
  }

  bool operator!=(const StringEnum &other) const {
    return value != other.value;
  }

private:
  const std::string value;
};

template <typename T> std::shared_ptr<T> from_json(const std::string &json) {
  auto jv = bj::parse(json);
  auto t = std::make_shared<T>();
  transform<T>::trans(jv, *t);
  return t;
}

}; // namespace jsoncpp

#endif // __INK19_JSONCPP_HPP__

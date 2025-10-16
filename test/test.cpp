#include "jsoncpp.hpp"
#include <gtest/gtest.h>

class ext_data {
public:
    int data;
};

class main_data {
public:
    int a;
    std::string b;
    std::vector<int> c;
    bool d;
    std::map<std::string, int> e;
    ext_data f;
    constexpr static std::string_view __jsoncpp_alias_name(const std::string_view& name) {
        if (name == "f") {
            return "alias_f";
        }
        return name;
    }
};

// 为嵌套对象测试定义类
class nested_data {
public:
    int nested_int;
    std::string nested_str;
};

class container_data {
public:
    nested_data nested;
    std::vector<nested_data> nested_list;
};

// 为嵌套类注册转换器（移到全局作用域）
namespace jsoncpp {
    template<>
    struct transform<nested_data> {
        static void trans(const bj::value &jv, nested_data &t) {
            bj::object const &jo = jv.as_object();
            if (jo.contains("nested_int")) {
                t.nested_int = jo.at("nested_int").as_int64();
            }
            if (jo.contains("nested_str")) {
                t.nested_str = jo.at("nested_str").as_string().c_str();
            }
        }
    };
}

namespace jsoncpp {

template<>
struct transform<ext_data> {
    static void trans(const bj::value &jv, ext_data &t) {
        t.data = 1024;
    }
};

}

TEST(JsonCppTest, Test1) {
    std::string json_str = "{\"a\":1, \"b\":\"hello\", \"c\":[1, \"2\", 3], \"d\": true, \"e\": {\"a\": 1, \"b\": \"2\"}, \"alias_f\": \"1.23456789012345678901234567890\"}";
    auto test = jsoncpp::from_json<main_data>(json_str);

    EXPECT_EQ(test->a, 1);
    EXPECT_EQ(test->b, "hello");
    EXPECT_EQ(test->c.size(), 3);
    EXPECT_EQ(test->c[0], 1);
    EXPECT_EQ(test->c[1], 2);
    EXPECT_EQ(test->c[2], 3);
    EXPECT_EQ(test->d, true);
    EXPECT_EQ(test->e.size(), 2);
    EXPECT_EQ(test->e["a"], 1);
    EXPECT_EQ(test->e["b"], 2);
    EXPECT_EQ(test->f.data, 1024);
}

TEST(JsonCppTest, BasicTypesTest) {
    // 测试基本类型转换
    std::string json_str = "{\"a\":42, \"b\":\"world\", \"d\":false}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    
    EXPECT_EQ(test->a, 42);
    EXPECT_EQ(test->b, "world");
    EXPECT_EQ(test->d, false);
    EXPECT_TRUE(test->c.empty());
    EXPECT_TRUE(test->e.empty());
}

TEST(JsonCppTest, EmptyContainersTest) {
    // 测试空容器
    std::string json_str = "{\"a\":0, \"b\":\"\", \"c\":[], \"d\":false, \"e\":{}, \"alias_f\":\"0\"}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    
    EXPECT_EQ(test->a, 0);
    EXPECT_EQ(test->b, "");
    EXPECT_TRUE(test->c.empty());
    EXPECT_FALSE(test->d);
    EXPECT_TRUE(test->e.empty());
}

TEST(JsonCppTest, ComplexVectorTest) {
    // 测试复杂向量
    std::string json_str = "{\"c\":[10, \"20\", 30, \"40\", 50]}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    
    EXPECT_EQ(test->c.size(), 5);
    EXPECT_EQ(test->c[0], 10);
    EXPECT_EQ(test->c[1], 20);
    EXPECT_EQ(test->c[2], 30);
    EXPECT_EQ(test->c[3], 40);
    EXPECT_EQ(test->c[4], 50);
}

TEST(JsonCppTest, ComplexMapTest) {
    // 测试复杂映射
    std::string json_str = "{\"e\":{\"key1\":1, \"key2\":\"2\", \"key3\":3, \"key4\":\"4\"}}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    
    EXPECT_EQ(test->e.size(), 4);
    EXPECT_EQ(test->e["key1"], 1);
    EXPECT_EQ(test->e["key2"], 2);
    EXPECT_EQ(test->e["key3"], 3);
    EXPECT_EQ(test->e["key4"], 4);
}

TEST(JsonCppTest, FieldAliasTest) {
    // 测试字段别名功能
    std::string json_str = "{\"alias_f\":\"999\"}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    
    EXPECT_EQ(test->f.data, 1024); // 自定义转换器固定返回1024
}

TEST(JsonCppTest, PartialDataTest) {
    // 测试部分数据缺失的情况
    std::string json_str = "{\"a\":100}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    
    EXPECT_EQ(test->a, 100);
    EXPECT_EQ(test->b, ""); // 默认值
    EXPECT_TRUE(test->c.empty()); // 默认值
    EXPECT_FALSE(test->d); // 默认值
    EXPECT_TRUE(test->e.empty()); // 默认值
}

TEST(JsonCppTest, BooleanFromStringTest) {
    // 测试字符串到布尔值的转换
    std::string json_str = "{\"d\":\"true\"}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    EXPECT_TRUE(test->d);
    
    json_str = "{\"d\":\"false\"}";
    test = jsoncpp::from_json<main_data>(json_str);
    EXPECT_FALSE(test->d);
    
    json_str = "{\"d\":\"1\"}";
    test = jsoncpp::from_json<main_data>(json_str);
    EXPECT_TRUE(test->d);
}

TEST(JsonCppTest, IntegerFromStringTest) {
    // 测试字符串到整数的转换
    std::string json_str = "{\"a\":\"123\"}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    EXPECT_EQ(test->a, 123);
}

// 错误处理测试
TEST(JsonCppTest, InvalidJsonTest) {
    // 测试无效JSON
    std::string json_str = "invalid json string";
    EXPECT_THROW(jsoncpp::from_json<main_data>(json_str), std::exception);
}

TEST(JsonCppTest, EmptyJsonTest) {
    // 测试空JSON
    std::string json_str = "{}";
    auto test = jsoncpp::from_json<main_data>(json_str);
    
    EXPECT_EQ(test->a, 0);
    EXPECT_EQ(test->b, "");
    EXPECT_TRUE(test->c.empty());
    EXPECT_FALSE(test->d);
    EXPECT_TRUE(test->e.empty());
}

TEST(JsonCppTest, NestedObjectTest) {
    std::string json_str = "{\"nested\":{\"nested_int\":999, \"nested_str\":\"nested_value\"}, \"nested_list\":[{\"nested_int\":1, \"nested_str\":\"first\"}, {\"nested_int\":2, \"nested_str\":\"second\"}]}";
    auto test = jsoncpp::from_json<container_data>(json_str);
    
    EXPECT_EQ(test->nested.nested_int, 999);
    EXPECT_EQ(test->nested.nested_str, "nested_value");
    EXPECT_EQ(test->nested_list.size(), 2);
    EXPECT_EQ(test->nested_list[0].nested_int, 1);
    EXPECT_EQ(test->nested_list[0].nested_str, "first");
    EXPECT_EQ(test->nested_list[1].nested_int, 2);
    EXPECT_EQ(test->nested_list[1].nested_str, "second");
}

int main() {
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

#include "DataPacket.hpp"
#include "Result.hpp"
#include <string>
#include <iostream>
#include <vector>

using namespace workflow;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { \
        std::cout << "  TEST: " << (name) << " ... "; \
    } while(0)

#define PASS() \
    do { \
        std::cout << "PASSED" << std::endl; \
        tests_passed++; \
    } while(0)

#define FAIL(msg) \
    do { \
        std::cout << "FAILED: " << (msg) << std::endl; \
        tests_failed++; \
    } while(0)

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { FAIL(#expr); return; } \
    } while(0)

#define ASSERT_FALSE(expr) \
    do { \
        if ((expr)) { FAIL(#expr " (expected false)"); return; } \
    } while(0)



void test_datapacket_default_construction() {
    TEST("DataPacket default construction yields empty packet");
    DataPacket dp;
    ASSERT_TRUE(dp.size() == 0);
    ASSERT_TRUE(dp.keys().empty());
    ASSERT_FALSE(dp.has("anything"));
    PASS();
}

void test_datapacket_set_get_int() {
    TEST("DataPacket set/get int preserves value");
    DataPacket dp;
    dp.set("count", 42);
    ASSERT_TRUE(dp.has("count"));
    ASSERT_TRUE(dp.size() == 1);
    auto result = dp.get<int>("count");
    ASSERT_TRUE(result.is_ok());
    ASSERT_TRUE(result.value() == 42);
    PASS();
}

void test_datapacket_set_get_double() {
    TEST("DataPacket set/get double preserves value");
    DataPacket dp;
    dp.set("pi", 3.14159);
    ASSERT_TRUE(dp.has("pi"));
    auto result = dp.get<double>("pi");
    ASSERT_TRUE(result.is_ok());

    double diff = result.value() - 3.14159;
    ASSERT_TRUE(diff < 0.0001 && diff > -0.0001);
    PASS();
}

void test_datapacket_set_get_string() {
    TEST("DataPacket set/get std::string preserves value");
    DataPacket dp;
    dp.set("name", std::string("workflow"));
    ASSERT_TRUE(dp.has("name"));
    auto result = dp.get<std::string>("name");
    ASSERT_TRUE(result.is_ok());
    ASSERT_TRUE(result.value() == "workflow");
    PASS();
}

void test_datapacket_set_get_cstring() {
    TEST("DataPacket set/get const char* preserves value");
    DataPacket dp;
    dp.set<std::string>("label", "hello");
    ASSERT_TRUE(dp.has("label"));
    auto result = dp.get<std::string>("label");
    ASSERT_TRUE(result.is_ok());
    ASSERT_TRUE(result.value() == "hello");
    PASS();
}

void test_datapacket_get_missing_key_returns_error() {
    TEST("DataPacket get on missing key returns error Result");
    DataPacket dp;
    auto result = dp.get<int>("nonexistent");
    ASSERT_TRUE(result.is_error());
    PASS();
}

void test_datapacket_get_wrong_type_returns_error() {
    TEST("DataPacket get with wrong type returns error");
    DataPacket dp;
    dp.set("value", 100);
    auto result = dp.get<double>("value");

    ASSERT_TRUE(result.is_error());
    PASS();
}

void test_datapacket_multiple_entries() {
    TEST("DataPacket with multiple entries tracks keys correctly");
    DataPacket dp;
    dp.set("a", 1);
    dp.set("b", std::string("two"));
    dp.set("c", 3.0);

    ASSERT_TRUE(dp.size() == 3);
    auto keys = dp.keys();

    bool has_a = false, has_b = false, has_c = false;
    for (const auto& k : keys) {
        if (k == "a") has_a = true;
        if (k == "b") has_b = true;
        if (k == "c") has_c = true;
    }
    ASSERT_TRUE(has_a && has_b && has_c);
    PASS();
}

void test_datapacket_overwrite() {
    TEST("DataPacket overwriting a key replaces the value");
    DataPacket dp;
    dp.set("x", 1);
    dp.set("x", 2);
    ASSERT_TRUE(dp.size() == 1);
    auto result = dp.get<int>("x");
    ASSERT_TRUE(result.is_ok());
    ASSERT_TRUE(result.value() == 2);
    PASS();
}

void test_datapacket_has_negative() {
    TEST("DataPacket has() returns false for missing keys");
    DataPacket dp;
    ASSERT_FALSE(dp.has("anything"));
    dp.set("exists", 42);
    ASSERT_TRUE(dp.has("exists"));
    ASSERT_FALSE(dp.has("missing"));
    PASS();
}

void test_datapacket_copy_semantics() {
    TEST("DataPacket copy semantics preserve all data");
    DataPacket original;
    original.set("key1", 10);
    original.set("key2", std::string("value"));

    DataPacket copy = original;
    ASSERT_TRUE(copy.size() == 2);
    ASSERT_TRUE(copy.has("key1"));
    ASSERT_TRUE(copy.has("key2"));
    ASSERT_TRUE(copy.get<int>("key1").value() == 10);


    copy.set("key1", 999);
    ASSERT_TRUE(original.get<int>("key1").value() == 10);
    PASS();
}

void test_datapacket_move_semantics() {
    TEST("DataPacket move semantics transfer data");
    DataPacket original;
    original.set("data", std::string("heavy"));

    DataPacket moved = std::move(original);
    ASSERT_TRUE(moved.size() == 1);
    ASSERT_TRUE(moved.has("data"));
    ASSERT_TRUE(moved.get<std::string>("data").value() == "heavy");

    PASS();
}

void test_datapacket_json_roundtrip() {
    TEST("DataPacket JSON serialization roundtrip");
    DataPacket dp;
    dp.set("id", 12345);
    dp.set("name", std::string("test-user"));
    dp.set("score", 98.6);

    nlohmann::json j = dp.to_json();
    ASSERT_TRUE(j.is_object());

    auto restored_result = DataPacket::from_json(j);
    ASSERT_TRUE(restored_result.is_ok());

    DataPacket restored = restored_result.value();
    ASSERT_TRUE(restored.size() == 3);
    ASSERT_TRUE(restored.get<int>("id").value() == 12345);
    ASSERT_TRUE(restored.get<std::string>("name").value() == "test-user");

    double diff = restored.get<double>("score").value() - 98.6;
    ASSERT_TRUE(diff < 0.0001 && diff > -0.0001);
    PASS();
}



int main() {
    std::cout << "=== DataPacket Unit Tests ===" << std::endl;

    test_datapacket_default_construction();
    test_datapacket_set_get_int();
    test_datapacket_set_get_double();
    test_datapacket_set_get_string();
    test_datapacket_set_get_cstring();
    test_datapacket_get_missing_key_returns_error();
    test_datapacket_get_wrong_type_returns_error();
    test_datapacket_multiple_entries();
    test_datapacket_overwrite();
    test_datapacket_has_negative();
    test_datapacket_copy_semantics();
    test_datapacket_move_semantics();
    test_datapacket_json_roundtrip();

    std::cout << "\n=== Summary === " << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
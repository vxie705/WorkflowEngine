#include "Result.hpp"
#include <string>
#include <cassert>
#include <stdexcept>
#include <iostream>

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

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { FAIL("expected equality"); return; } \
    } while(0)



void test_result_ok_int() {
    TEST("Result<int>::ok stores value correctly");
    auto r = Result<int>::ok(42);
    ASSERT_TRUE(r.is_ok());
    ASSERT_FALSE(r.is_error());
    ASSERT_EQ(r.value(), 42);
    ASSERT_EQ(r.value_or(100), 42);
    PASS();
}

void test_result_error_int() {
    TEST("Result<int>::error stores error message");
    auto r = Result<int>::error("something went wrong", 99);
    ASSERT_TRUE(r.is_error());
    ASSERT_FALSE(r.is_ok());
    ASSERT_EQ(r.error_code(), 99);
    ASSERT_TRUE(r.error_message() == "something went wrong");
    PASS();
}

void test_result_error_value_throws() {
    TEST("Result<int>::error::value() throws runtime_error");
    auto r = Result<int>::error("fail");
    try {
        r.value();
        FAIL("expected exception not thrown");
    } catch (const std::runtime_error& e) {
        PASS();
    } catch (...) {
        FAIL("wrong exception type");
    }
}

void test_result_ok_value_no_throw() {
    TEST("Result<int>::ok::value() does not throw");
    auto r = Result<int>::ok(7);
    try {
        ASSERT_EQ(r.value(), 7);
        PASS();
    } catch (...) {
        FAIL("unexpected exception");
    }
}

void test_result_value_or_on_error() {
    TEST("value_or on error returns default");
    auto r = Result<int>::error("nope");
    ASSERT_EQ(r.value_or(99), 99);
    PASS();
}

void test_result_void_ok() {
    TEST("Result<void>::ok works");
    auto r = Result<void>::ok();
    ASSERT_TRUE(r.is_ok());
    ASSERT_FALSE(r.is_error());
    PASS();
}

void test_result_void_error() {
    TEST("Result<void>::error works");
    auto r = Result<void>::error("void error", 5);
    ASSERT_TRUE(r.is_error());
    ASSERT_EQ(r.error_code(), 5);
    PASS();
}

void test_result_copy_semantics() {
    TEST("Result<int> copy semantics");
    auto r1 = Result<int>::ok(10);
    auto r2 = r1;
    ASSERT_EQ(r1.value(), 10);
    ASSERT_EQ(r2.value(), 10);
    ASSERT_TRUE(r1.is_ok());
    ASSERT_TRUE(r2.is_ok());
    PASS();
}

void test_result_move_semantics() {
    TEST("Result<int> move semantics");
    auto r1 = Result<int>::ok(20);
    auto r2 = std::move(r1);
    ASSERT_EQ(r2.value(), 20);
    ASSERT_TRUE(r2.is_ok());

    PASS();
}



int main() {
    std::cout << "=== Result<T> Unit Tests ===" << std::endl;

    test_result_ok_int();
    test_result_error_int();
    test_result_error_value_throws();
    test_result_ok_value_no_throw();
    test_result_value_or_on_error();
    test_result_void_ok();
    test_result_void_error();
    test_result_copy_semantics();
    test_result_move_semantics();

    std::cout << "\n=== Summary === " << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
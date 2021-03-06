#include "timeout_handler.h"
#include <gtest/gtest.h>
#include <atomic>

#ifdef FAKE_TIME
#define Time FakeTime
#endif

using namespace dronecore;

TEST(TimeoutHandler, Timeout)
{
    Time time;
    TimeoutHandler th(time);

    bool timeout_happened = false;

    void *cookie = nullptr;
    th.add([&timeout_happened]() {
        timeout_happened = true;
    }, 0.5, &cookie);

    time.sleep_for(std::chrono::milliseconds(250));
    th.run_once();
    EXPECT_FALSE(timeout_happened);
    time.sleep_for(std::chrono::milliseconds(500));
    th.run_once();
    EXPECT_TRUE(timeout_happened);

    UNUSED(cookie);
}

TEST(TimeoutHandler, TimeoutNoCookie)
{
    Time time;
    TimeoutHandler th(time);

    bool timeout_happened = false;

    // This time we supply nullptr and don't want a cookie.
    th.add([&timeout_happened]() {
        timeout_happened = true;
    }, 0.5, nullptr);

    time.sleep_for(std::chrono::milliseconds(250));
    th.run_once();
    EXPECT_FALSE(timeout_happened);
    time.sleep_for(std::chrono::milliseconds(500));
    th.run_once();
    EXPECT_TRUE(timeout_happened);
}

TEST(TimeoutHandler, CallTimeoutInTimeoutCallback)
{
    Time time;
    TimeoutHandler th(time);

    bool timeout_happened = false;

    void *cookie1 = nullptr;
    void *cookie2 = nullptr;
    th.add([&th, &timeout_happened, &cookie2]() {
        timeout_happened = true;
        // This tests the case where we want to set yet another timeout when we
        // are called because of a timeout. This tests if there are no locking
        // issues.
        th.add([]() {
        }, 5.0, &cookie2);
    }, 0.5, &cookie1);

    time.sleep_for(std::chrono::milliseconds(250));
    th.run_once();
    EXPECT_FALSE(timeout_happened);
    time.sleep_for(std::chrono::milliseconds(500));
    th.run_once();
    EXPECT_TRUE(timeout_happened);

    UNUSED(cookie1);
    UNUSED(cookie2);
}

TEST(TimeoutHandler, TimeoutRefreshed)
{
    Time time {};
    TimeoutHandler th(time);

    bool timeout_happened = false;

    void *cookie = nullptr;
    th.add([&timeout_happened]() {
        timeout_happened = true;
    }, 0.5, &cookie);

    time.sleep_for(std::chrono::milliseconds(400));
    th.run_once();
    EXPECT_FALSE(timeout_happened);
    th.refresh(cookie);
    time.sleep_for(std::chrono::milliseconds(300));
    th.run_once();
    EXPECT_FALSE(timeout_happened);
    time.sleep_for(std::chrono::milliseconds(300));
    th.run_once();
    EXPECT_TRUE(timeout_happened);

    UNUSED(cookie);
}

TEST(TimeoutHandler, TimeoutRemoved)
{
    Time time {};
    TimeoutHandler th(time);

    bool timeout_happened = false;

    void *cookie = nullptr;
    th.add([&timeout_happened]() {
        timeout_happened = true;
    }, 0.5, &cookie);

    time.sleep_for(std::chrono::milliseconds(250));
    th.run_once();
    EXPECT_FALSE(timeout_happened);
    th.remove(cookie);
    time.sleep_for(std::chrono::milliseconds(500));
    th.run_once();
    EXPECT_FALSE(timeout_happened);
}

TEST(TimeoutHandler, TimeoutRemovedDuringCallback)
{
    Time time {};
    TimeoutHandler th(time);

    bool timeout_happened = false;

    void *cookie = nullptr;
    th.add([&th, &cookie, &timeout_happened]() {
        // This is evil but can potentially happen. We remove our own timeout while
        // being called.
        th.remove(cookie);
        timeout_happened = true;
    }, 0.5, &cookie);

    time.sleep_for(std::chrono::milliseconds(250));
    th.run_once();
    EXPECT_FALSE(timeout_happened);
    time.sleep_for(std::chrono::milliseconds(500));
    th.run_once();
    EXPECT_TRUE(timeout_happened);
}

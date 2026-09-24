/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "brave/components/tor/tor_file_watcher.h"
#include "build/build_config.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tor {
namespace {

constexpr char kInvalidControlport[] =
#if BUILDFLAG(IS_WIN)
    "invalid_controlport_win";
#else
    "invalid_controlport";
#endif
constexpr char kValidControlportNotLocalhost[] =
#if BUILDFLAG(IS_WIN)
    "valid_controlport_not_localhost_win";
#else
    "valid_controlport_not_localhost";
#endif
constexpr char kControlportTooLong[] =
#if BUILDFLAG(IS_WIN)
    "controlport_too_long_win";
#else
    "controlport_too_long";
#endif
constexpr char kControlportMax[] =
#if BUILDFLAG(IS_WIN)
    "controlport_max_win";
#else
    "controlport_max";
#endif
constexpr char kControlportOverflow[] =
#if BUILDFLAG(IS_WIN)
    "controlport_overflow_win";
#else
    "controlport_overflow";
#endif
constexpr char kInvalidControlPortEnd[] =
#if BUILDFLAG(IS_WIN)
    "invalid_control_port_end_win";
#else
    "invalid_control_port_end";
#endif
constexpr char kNormalControlport[] =
#if BUILDFLAG(IS_WIN)
    "normal_controlport_win";
#else
    "normal_controlport";
#endif

}  // namespace

class TorFileWatcherTest : public testing::Test {
 public:
  TorFileWatcherTest() = default;

  void SetUp() override {
    testing::Test::SetUp();
    test_data_dir_ = base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
                         .Append(FILE_PATH_LITERAL("brave"))
                         .Append(FILE_PATH_LITERAL("test"))
                         .Append(FILE_PATH_LITERAL("data"));
  }

  base::FilePath test_data_dir() {
    return test_data_dir_.AppendASCII("tor").AppendASCII("tor_control");
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::FilePath test_data_dir_;
};

TEST_F(TorFileWatcherTest, EatControlCookie) {
  std::vector<uint8_t> cookie;
  base::Time time;

  std::unique_ptr<TorFileWatcher> tor_file_watcher =
      std::make_unique<TorFileWatcher>(
          test_data_dir().AppendASCII("not_valid"));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(cookie.size(), 0u);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  // control_auth_cookie is a folder
  tor_file_watcher = std::make_unique<TorFileWatcher>(test_data_dir());
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(cookie.size(), 0u);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII("empty_auth_cookies"));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(cookie.size(), 0u);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII("auth_cookies_too_long"));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(cookie.size(), 0u);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  constexpr uint8_t expected_auth_cookie[] = {
      0x6c, 0x6e, 0x9d, 0x24, 0x78, 0xe6, 0x6d, 0x69, 0xd3, 0x2d, 0xc9,
      0x90, 0x9a, 0x3c, 0x39, 0x54, 0x2b, 0x37, 0xff, 0x30, 0xda, 0x5a,
      0x90, 0x94, 0x44, 0xa4, 0x3d, 0x30, 0xd5, 0xa9, 0x19, 0xef};

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII("normal_auth_cookies"));
  tor_file_watcher->polling_ = true;
  EXPECT_TRUE(tor_file_watcher->EatControlCookie(cookie, time));
  ASSERT_EQ(cookie.size(), std::size(expected_auth_cookie));
  EXPECT_EQ(base::span(cookie), base::span(expected_auth_cookie));
  EXPECT_NE(time.InMillisecondsFSinceUnixEpoch(), 0u);
}

TEST_F(TorFileWatcherTest, EatControlPort) {
  int port = -1;
  base::Time time;

  std::unique_ptr<TorFileWatcher> tor_file_watcher =
      std::make_unique<TorFileWatcher>(
          test_data_dir().AppendASCII("not_valid"));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  // controlport is a folder
  tor_file_watcher = std::make_unique<TorFileWatcher>(test_data_dir());
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII("empty_controlport"));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII(kInvalidControlport));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII(kValidControlportNotLocalhost));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII(kControlportMax));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII(kControlportTooLong));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII(kControlportOverflow));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, 65536);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  port = -1;
  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII(kInvalidControlPortEnd));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, 0);
  EXPECT_EQ(time.InMillisecondsFSinceUnixEpoch(), 0u);

  port = -1;
  time = base::Time();

  tor_file_watcher = std::make_unique<TorFileWatcher>(
      test_data_dir().AppendASCII(kNormalControlport));
  tor_file_watcher->polling_ = true;
  EXPECT_TRUE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, 5566);
  EXPECT_NE(time.InMillisecondsFSinceUnixEpoch(), 0u);
}

}  // namespace tor

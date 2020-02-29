/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_control.h"

#include "brave/common/tor/tor_control_impl.h"
#include "brave/common/tor/tor_control_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tor {

TEST(TorControlTest, ParseQuoted) {
  const struct {
    const char *input;
    const char *output;
    size_t end;
  } cases[] = {
    { "\"127.0.0.1:41159\"", "127.0.0.1:41159", 17 },
    { "\"unix:/a b/c\"", "unix:/a b/c", 13 },
    { "\"unix:/a\\rb/c\"", "unix:/a\rb/c", 14 },
    { "\"unix:/a\\nb/c\"", "unix:/a\nb/c", 14 },
    { "\"unix:/a\\tb/c\"", "unix:/a\tb/c", 14 },
    { "\"unix:/a\\\\b/c\"", "unix:/a\\b/c", 14 },
    { "\"unix:/a\\\"b/c\"", "unix:/a\"b/c", 14 },
    { "\"unix:/a\\'b/c\"", "unix:/a'b/c", 14 },
    { "\"unix:/a b/c\" \"127.0.0.1:9050\"", "unix:/a b/c", 13 },
    { "\"unix:/a b/c", nullptr, -1 },
    { "\"unix:/a\\fb/c\"", nullptr, -1 },
  };
  size_t i;

  for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
    std::string value;
    size_t end;
    bool ok = TorControlImpl::ParseQuoted(cases[i].input, value, end);
    if (cases[i].output) {
      EXPECT_TRUE(ok) << i;
      EXPECT_EQ(value, cases[i].output) << i;
      EXPECT_EQ(end, cases[i].end) << i;
    } else {
      EXPECT_FALSE(ok) << i;
    }
  }
}

TEST(TorControlTest, ParseKV) {
  const struct {
    const char *input;
    const char *key;
    const char *value;
    size_t end;
  } cases[] = {
    { "foo=bar", "foo", "bar", 7 },
    { "foo=\"bar\"", "foo", "bar", 9 },
    { "foo=\"bar baz\"", "foo", "bar baz", 13 },
    { "foo=\"bar\\\"baz\"", "foo", "bar\"baz", 14 },
    { "foo=\"bar\\\"baz\" quux=\"zot\"", "foo", "bar\"baz", 15 },
    { "foo=barbaz quux=zot", "foo", "barbaz", 11 },
    { "foo=\"bar", nullptr, nullptr, -1 },
  };
  size_t i;

  for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
    std::string key;
    std::string value;
    size_t end;
    bool ok = TorControlImpl::ParseKV(cases[i].input, key, value, end);
    if (cases[i].value) {
      EXPECT_TRUE(ok) << i << ": " << cases[i].input
                      << "\nkey  : " << key
                      << "\nvalue: " << value;
      EXPECT_EQ(key, cases[i].key) << i << ": " << cases[i].input
                      << "\nkey  : " << key
                      << "\nvalue: " << value;
      EXPECT_EQ(value, cases[i].value) << i << ": " << cases[i].input
                      << "\nkey  : " << key
                      << "\nvalue: " << value;
      EXPECT_EQ(end, cases[i].end) << i << ": " << cases[i].input
                      << "\nkey  : " << key
                      << "\nvalue: " << value;
    } else {
      EXPECT_FALSE(ok) << i << ": " << cases[i].input
                       << "\nkey  : " << key
                       << "\nvalue: " << value;
    }
  }
}

}  // namespace tor

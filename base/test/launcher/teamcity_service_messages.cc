/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/test/launcher/teamcity_service_messages.h"

#include <string>

#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"

namespace base {
namespace {

// https://www.jetbrains.com/help/teamcity/service-messages.html#Escaped+Values
std::string_view TeamcityValueEscape(std::string_view s, std::string& result) {
  static constexpr char kSymbolsToEscape[] = "\n\r'|[]";
  const char* s_char = base::ranges::find_first_of(s, kSymbolsToEscape);
  if (s_char == s.end()) {
    return s;
  }

  result.reserve(s.length() + s.length() / 4);
  result.assign(s.begin(), s_char);

  for (; s_char != s.end(); ++s_char) {
    switch (*s_char) {
      case '\n':
        result += "|n";
        break;
      case '\r':
        result += "|r";
        break;
      case '\'':
      case '|':
      case '[':
      case ']':
        result += '|';
        [[fallthrough]];
      default:
        result += *s_char;
    }
  }

  return result;
}

}  // namespace

TeamcityServiceMessages::Message::Message(std::ostream& ostream,
                                          std::string_view name)
    : ostream_(ostream) {
  (*ostream_) << "##teamcity[" << name;
}

TeamcityServiceMessages::Message::~Message() {
  (*ostream_) << "]" << std::endl;
}

TeamcityServiceMessages::Message&
TeamcityServiceMessages::Message::WriteProperty(std::string_view name,
                                                std::string_view value) {
  if (!value.empty()) {
    std::string escaped_value;
    (*ostream_) << " " << name << "='"
                << TeamcityValueEscape(value, escaped_value) << "'";
  }
  return *this;
}

TeamcityServiceMessages::TeamcityServiceMessages(std::ostream& ostream)
    : ostream_(ostream) {}

TeamcityServiceMessages::~TeamcityServiceMessages() = default;

void TeamcityServiceMessages::TestRetrySupport(bool enabled) {
  Message(*ostream_, "testRetrySupport")
      .WriteProperty("enabled", enabled ? "true" : "false");
}

void TeamcityServiceMessages::TestSuiteStarted(std::string_view name) {
  Message(*ostream_, "testSuiteStarted").WriteProperty("name", name);
}

void TeamcityServiceMessages::TestSuiteFinished(std::string_view name) {
  Message(*ostream_, "testSuiteFinished").WriteProperty("name", name);
}

void TeamcityServiceMessages::TestStarted(std::string_view name) {
  Message(*ostream_, "testStarted")
      .WriteProperty("name", name)
      .WriteProperty("captureStandardOutput", "true");
}

void TeamcityServiceMessages::TestFailed(std::string_view name,
                                         std::string_view message,
                                         std::string_view details) {
  Message(*ostream_, "testFailed")
      .WriteProperty("name", name)
      .WriteProperty("message", message)
      .WriteProperty("details", details);
}

void TeamcityServiceMessages::TestIgnored(std::string_view name,
                                          std::string_view message) {
  Message(*ostream_, "testIgnored")
      .WriteProperty("name", name)
      .WriteProperty("message", message);
}

void TeamcityServiceMessages::TestFinished(std::string_view name,
                                           base::TimeDelta duration) {
  Message(*ostream_, "testFinished")
      .WriteProperty("name", name)
      .WriteProperty("duration",
                     base::NumberToString(duration.InMilliseconds()));
}

}  // namespace base

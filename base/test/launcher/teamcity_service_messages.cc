/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/base/test/launcher/teamcity_service_messages.h"

#include "base/strings/string_number_conversions.h"

namespace base {
namespace {

// https://www.jetbrains.com/help/teamcity/service-messages.html#Escaped+Values
class EscapedValue {
 public:
  explicit EscapedValue(std::string_view value) : value_(value) {}

  friend std::ostream& operator<<(std::ostream& stream,
                                  const EscapedValue& escaped_value) {
    static constexpr char kSymbolsToEscape[] = "\n\r'|[]";
    std::string_view s = escaped_value.value_;

    for (size_t pos = s.find_first_of(kSymbolsToEscape); !s.empty();
         pos = s.find_first_of(kSymbolsToEscape)) {
      stream << s.substr(0, pos);
      if (pos == std::string_view::npos) {
        break;
      }

      const char symbol = s[pos];
      switch (symbol) {
        case '\n':
          stream << "|n";
          break;
        case '\r':
          stream << "|r";
          break;
        default:
          stream << '|' << symbol;
      }
      s.remove_prefix(pos + 1);
    }

    return stream;
  }

 private:
  std::string_view value_;
};

}  // namespace

TeamcityServiceMessages::Message::Message(std::ostream& ostream,
                                          std::string_view name)
    : ostream_(ostream) {
  // Use stringstream to format the message before writing it to stdout.
  sstream_ << "##teamcity[" << name;
}

TeamcityServiceMessages::Message::~Message() {
  sstream_ << "]" << std::endl;
  // Important: output into stdout in a single call to not mix with outputs from
  // other threads.
  (*ostream_) << sstream_.str() << std::flush;
}

TeamcityServiceMessages::Message&
TeamcityServiceMessages::Message::WriteProperty(std::string_view name,
                                                std::string_view value) {
  if (!value.empty()) {
    sstream_ << " " << name << "='" << EscapedValue(value) << "'";
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
                                           TimeDelta duration) {
  Message(*ostream_, "testFinished")
      .WriteProperty("name", name)
      .WriteProperty("duration", NumberToString(duration.InMilliseconds()));
}

}  // namespace base

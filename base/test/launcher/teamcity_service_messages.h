/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_SERVICE_MESSAGES_H_
#define BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_SERVICE_MESSAGES_H_

#include <ostream>
#include <sstream>
#include <string_view>

#include "base/memory/raw_ref.h"
#include "base/time/time.h"

namespace base {

// A minimal implementation of Teamcity Service Messages for test reporting.
// https://www.jetbrains.com/help/teamcity/service-messages.html
class TeamcityServiceMessages {
 public:
  explicit TeamcityServiceMessages(std::ostream& ostream);
  TeamcityServiceMessages(const TeamcityServiceMessages&) = delete;
  TeamcityServiceMessages& operator=(const TeamcityServiceMessages&) = delete;
  ~TeamcityServiceMessages();

  void TestRetrySupport(bool enabled);
  void TestSuiteStarted(std::string_view name);
  void TestSuiteFinished(std::string_view name);
  void TestStarted(std::string_view name);
  void TestFailed(std::string_view name,
                  std::string_view message = std::string_view(),
                  std::string_view details = std::string_view());
  void TestIgnored(std::string_view name,
                   std::string_view message = std::string_view());
  void TestFinished(std::string_view name, TimeDelta duration);

 private:
  friend class TeamcityServiceMessagesTest;

  class Message {
   public:
    Message(std::ostream& ostream, std::string_view name);
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;
    ~Message();

    Message& WriteProperty(std::string_view name, std::string_view value);

   private:
    raw_ref<std::ostream> ostream_;
    std::ostringstream sstream_;
  };

  raw_ref<std::ostream> ostream_;
};

}  // namespace base

#endif  // BRAVE_BASE_TEST_LAUNCHER_TEAMCITY_SERVICE_MESSAGES_H_

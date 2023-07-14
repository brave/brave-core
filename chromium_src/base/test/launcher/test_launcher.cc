/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/launcher/test_launcher.h"

#include "base/base_paths.h"
#include "base/environment.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece_forward.h"
#include "base/test/launcher/test_result.h"

#define TestLauncher TestLauncher_ChromiumImpl
#define AddTestResult(...)    \
  AddTestResult(__VA_ARGS__); \
  OnTestResult(__VA_ARGS__)

#include "src/base/test/launcher/test_launcher.cc"

#undef TestLauncher

namespace base {

namespace {

const std::string& GetExecutableName() {
  static const base::NoDestructor<std::string> kExeName([]() {
    base::FilePath file_exe;
    CHECK(base::PathService::Get(base::FILE_EXE, &file_exe));
    file_exe = file_exe.BaseName().RemoveFinalExtension();
    CHECK(base::IsStringASCII(file_exe.value()));
    return file_exe.MaybeAsASCII();
  }());
  return *kExeName;
}

class TeamcityMessages {
 public:
  static bool IsEnabled();

  static void EnableRetrySupport(bool enabled);

  static void SuiteStarted(base::StringPiece name);
  static void SuiteFinished(base::StringPiece name);
  static void TestStarted(base::StringPiece name);
  static void TestFailed(base::StringPiece name,
                         base::StringPiece message = base::StringPiece(),
                         base::StringPiece details = base::StringPiece());
  static void TestIgnored(base::StringPiece name,
                          base::StringPiece message = base::StringPiece());
  static void TestFinished(base::StringPiece name, base::TimeDelta duration);

 private:
  static base::StringPiece Escape(base::StringPiece s, std::string& result);

  static void OpenMsg(base::StringPiece name);
  static void WriteProperty(base::StringPiece name, base::StringPiece value);
  static void CloseMsg();
};

bool TeamcityMessages::IsEnabled() {
  static const bool kIsTeamcity = []() {
    const auto env = base::Environment::Create();
    return env->HasVar("TEAMCITY_VERSION");
  }();
  return kIsTeamcity;
}

void TeamcityMessages::EnableRetrySupport(bool enabled) {
  OpenMsg("testRetrySupport");
  WriteProperty("enabled", enabled ? "true" : "false");
  CloseMsg();
}

void TeamcityMessages::SuiteStarted(base::StringPiece name) {
  OpenMsg("testSuiteStarted");
  WriteProperty("name", name);
  CloseMsg();
}

void TeamcityMessages::SuiteFinished(base::StringPiece name) {
  OpenMsg("testSuiteFinished");
  WriteProperty("name", name);
  CloseMsg();
}

void TeamcityMessages::TestStarted(base::StringPiece name) {
  OpenMsg("testStarted");
  WriteProperty("name", name);
  WriteProperty("captureStandardOutput", "true");
  CloseMsg();
}

void TeamcityMessages::TestFinished(base::StringPiece name,
                                    base::TimeDelta duration) {
  OpenMsg("testFinished");
  WriteProperty("name", name);
  WriteProperty("duration", base::NumberToString(duration.InMilliseconds()));
  CloseMsg();
}

void TeamcityMessages::TestFailed(base::StringPiece name,
                                  base::StringPiece message,
                                  base::StringPiece details) {
  OpenMsg("testFailed");
  WriteProperty("name", name);
  WriteProperty("message", message);
  WriteProperty("details", details);
  CloseMsg();
}

void TeamcityMessages::TestIgnored(base::StringPiece name,
                                   base::StringPiece message) {
  OpenMsg("testIgnored");
  WriteProperty("name", name);
  WriteProperty("message", message);
  CloseMsg();
}

// static
base::StringPiece TeamcityMessages::Escape(base::StringPiece s,
                                           std::string& result) {
  constexpr char kSymbolsToEscape[] = "\n\r'|]";
  const char* s_char = base::ranges::find_first_of(s, kSymbolsToEscape);
  if (s_char == s.end()) {
    return s;
  }

  result.reserve(s.length() + s.length() / 4);
  result.assign(s.begin(), s_char);

  for (; s_char != s.end(); ++s_char) {
    switch (*s_char) {
      case '\n':
        result.append("|n");
        break;
      case '\r':
        result.append("|r");
        break;
      case '\'':
        result.append("|'");
        break;
      case '|':
        result.append("||");
        break;
      case ']':
        result.append("|]");
        break;
      default:
        result.append(s_char, 1);
    }
  }

  return result;
}

void TeamcityMessages::OpenMsg(base::StringPiece name) {
  std::cout << "##teamcity[" << name;
}

void TeamcityMessages::CloseMsg() {
  std::cout << "]" << std::endl;
}

void TeamcityMessages::WriteProperty(base::StringPiece name,
                                     base::StringPiece value) {
  std::string escaped;
  std::cout << " " << name << "='" << Escape(value, escaped) << "'";
}

}  // namespace

TestLauncher::TestLauncher(TestLauncherDelegate* launcher_delegate,
                           size_t parallel_jobs,
                           size_t retry_limit)
    : TestLauncher_ChromiumImpl(launcher_delegate, parallel_jobs, retry_limit) {
  if (TeamcityMessages::IsEnabled()) {
    TeamcityMessages::SuiteStarted(GetExecutableName());
  }
}

TestLauncher::~TestLauncher() {
  if (TeamcityMessages::IsEnabled()) {
    TeamcityMessages::SuiteFinished(GetExecutableName());
    if (teamcity_retry_support_set_) {
      TeamcityMessages::EnableRetrySupport(false);
    }
  }
}

void TestLauncher::OnTestFinished(const TestResult& result) {
  if (TeamcityMessages::IsEnabled()) {
    if (!teamcity_retry_support_set_) {
      TeamcityMessages::EnableRetrySupport(retry_limit_ != 0);
      teamcity_retry_support_set_ = true;
    }
    LogTeamcityTestStart(result);
    current_test_result_ = &result;
  }

  TestLauncher_ChromiumImpl::OnTestFinished(result);

  if (TeamcityMessages::IsEnabled()) {
    current_test_result_ = nullptr;
    LogTeamcityTestFinish(result);
  }
}

void TestLauncher::LogTeamcityTestStart(const TestResult& result) const {
  if (result.status != TestResult::TEST_SKIPPED) {
    TeamcityMessages::TestStarted(result.full_name);
  }
}

void TestLauncher::LogTeamcityTestFinish(const TestResult& result) const {
  if (result.status != TestResult::TEST_SKIPPED) {
    TeamcityMessages::TestFinished(result.full_name, result.elapsed_time);
  } else {
    TeamcityMessages::TestIgnored(result.full_name);
  }
}

void TestLauncher::OnTestResult(const TestResult& result) {
  switch (result.status) {
    case TestResult::TEST_SUCCESS:
    case TestResult::TEST_SKIPPED:
      break;
    case TestResult::TEST_FAILURE:
    case TestResult::TEST_FAILURE_ON_EXIT:
    case TestResult::TEST_TIMEOUT:
    case TestResult::TEST_CRASH:
    case TestResult::TEST_EXCESSIVE_OUTPUT:
      TeamcityMessages::TestFailed(result.full_name);
      break;
    case TestResult::TEST_UNKNOWN:
    case TestResult::TEST_NOT_RUN:
      NOTREACHED();
      break;
  }
}

void TestLauncher::MaybeSaveSummaryAsJSON(
    const std::vector<std::string>& additional_tags) {
  if (TeamcityMessages::IsEnabled() &&
      base::Contains(additional_tags, "BROKEN_TEST_EARLY_EXIT")) {
    // TestLauncher will call exit(1) before returning from OnTestFinished(), so
    // log the shutdown here while we can.
    LogTeamcityTestFinish(*current_test_result_);
    TeamcityMessages::SuiteFinished(GetExecutableName());
  }

  TestLauncher_ChromiumImpl::MaybeSaveSummaryAsJSON(additional_tags);
}

}  // namespace base

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"

namespace {

constexpr char kTestEnvVar[] = "TOOL_SHIM_TEST_VAR";
constexpr char kReportEnvVar[] = "TOOL_SHIM_TEST_REPORT";
constexpr char kExitCodeEnvVar[] = "TOOL_SHIM_TEST_EXIT_CODE";

}  // namespace

// Records argv (excluding argv[0]), cwd, and TOOL_SHIM_TEST_VAR as JSON.
// Writes the report to the path in TOOL_SHIM_TEST_REPORT when set, otherwise
// to stdout. Honors TOOL_SHIM_TEST_EXIT_CODE when set.
int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  auto environment = base::Environment::Create();

  int exit_code = 0;
  if (auto exit_code_value = environment->GetVar(kExitCodeEnvVar)) {
    if (!base::StringToInt(*exit_code_value, &exit_code)) {
      LOG(ERROR) << "Invalid TOOL_SHIM_TEST_EXIT_CODE value\n";
      return 1;
    }
  }

  base::ListValue argv_list;
  for (const auto& arg :
       base::span(base::CommandLine::ForCurrentProcess()->argv()).subspan(1u)) {
#if BUILDFLAG(IS_WIN)
    argv_list.Append(base::WideToUTF8(arg));
#else
    argv_list.Append(arg);
#endif
  }

  base::FilePath cwd;
  if (!base::GetCurrentDirectory(&cwd)) {
    LOG(ERROR) << "Failed to get current directory\n";
    return 1;
  }

  base::DictValue env_dict;
  if (auto value = environment->GetVar(kTestEnvVar)) {
    env_dict.Set(kTestEnvVar, *value);
  }

  base::DictValue report;
  report.Set("argv", std::move(argv_list));
  report.Set("cwd", cwd.AsUTF8Unsafe());
  report.Set("env", std::move(env_dict));

  std::optional<std::string> json = base::WriteJson(report);
  if (!json) {
    LOG(ERROR) << "Failed to serialize report\n";
    return 1;
  }

  if (auto report_path = environment->GetVar(kReportEnvVar)) {
    if (!base::WriteFile(base::FilePath::FromUTF8Unsafe(*report_path), *json)) {
      LOG(ERROR) << "Failed to write report to " << *report_path << "\n";
      return 1;
    }
  } else {
    std::cout << *json;
  }

  return exit_code;
}

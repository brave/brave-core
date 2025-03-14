// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/rule_data_reader.h"

#include <optional>

#include "base/files/file_util.h"

namespace psst {

namespace {

const base::FilePath::CharType kScriptsDir[] = FILE_PATH_LITERAL("scripts");

std::optional<std::string> ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    return std::nullopt;
  }
  return contents;
}

}  // namespace

RuleDataReader::RuleDataReader(const base::FilePath& component_path)
    : prefix_(base::FilePath(component_path).Append(kScriptsDir)) {}

std::optional<std::string> RuleDataReader::ReadUserScript(
    const PsstRule& rule) const {
  const auto user_script_path = base::FilePath(prefix_)
                                    .AppendASCII(rule.Name())
                                    .Append(rule.UserScriptPath());
  return ReadFile(user_script_path);
}

std::optional<std::string> RuleDataReader::ReadTestScript(
    const PsstRule& rule) const {
  const auto test_script_path = base::FilePath(prefix_)
                                    .AppendASCII(rule.Name())
                                    .Append(rule.TestScriptPath());
  return ReadFile(test_script_path);
}
std::optional<std::string> RuleDataReader::ReadPolicyScript(
    const PsstRule& rule) const {
  const auto policy_script_path = base::FilePath(prefix_)
                                      .AppendASCII(rule.Name())
                                      .Append(rule.PolicyScriptPath());
  return ReadFile(policy_script_path);
}

}  // namespace psst

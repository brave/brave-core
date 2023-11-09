// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/matched_rule.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"

namespace psst {

namespace {

const base::FilePath::CharType kScriptsDir[] = FILE_PATH_LITERAL("scripts");

std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}

}  // namespace

MatchedRule::MatchedRule(const std::string& name,
                         const std::string& user_script,
                         const std::string& test_script,
                         const std::string& policy_script,
                         int version)
    : name_(name),
      user_script_(user_script),
      test_script_(test_script),
      policy_script_(policy_script),
      version_(version) {}

MatchedRule::~MatchedRule() = default;
MatchedRule::MatchedRule(const MatchedRule&) = default;

// static
const MatchedRule MatchedRule::CreateMatchedRule(
    const base::FilePath& component_path,
    const std::string& name,
    const base::FilePath& user_script_path,
    const base::FilePath& test_script_path,
    const base::FilePath& policy_script_path,
    const int version) {
  auto prefix =
      base::FilePath(component_path).Append(kScriptsDir).AppendASCII(name);
  auto user_script = ReadFile(base::FilePath(prefix).Append(user_script_path));
  auto test_script = ReadFile(base::FilePath(prefix).Append(test_script_path));
  auto policy_script =
      ReadFile(base::FilePath(prefix).Append(policy_script_path));
  return MatchedRule(name, user_script, test_script, policy_script, version);
}

}  // namespace psst

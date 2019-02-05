/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_APP_BRAVE_COMMAND_LINE_HELPER_H_
#define BRAVE_APP_BRAVE_COMMAND_LINE_HELPER_H_

#include <string>
#include <unordered_set>

#include "base/macros.h"

namespace base {
class CommandLine;
}

class BraveCommandLineHelper {
 public:
  explicit BraveCommandLineHelper(base::CommandLine* command_line);
  inline ~BraveCommandLineHelper() = default;

  void AppendSwitch(const char* switch_key);
  void AppendFeatures(const std::unordered_set<const char*>& enabled,
                      const std::unordered_set<const char*>& disabled);

  const std::unordered_set<std::string>& enabled_features() const;
  const std::unordered_set<std::string>& disabled_features() const;

 private:
  void Parse();
  void ParseCSV(const std::string& value,
                std::unordered_set<std::string>* dest) const;
  void AppendCSV(const char* switch_key,
                 const std::unordered_set<std::string>& values);

  base::CommandLine& command_line_;
  std::unordered_set<std::string> enabled_features_;
  std::unordered_set<std::string> disabled_features_;

  DISALLOW_COPY_AND_ASSIGN(BraveCommandLineHelper);
};

#endif  // BRAVE_APP_BRAVE_COMMAND_LINE_HELPER_H_

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_COMMANDER_TAB_COMMAND_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_COMMANDER_TAB_COMMAND_SOURCE_H_

#include "chrome/browser/ui/commander/command_source.h"

#include "src/chrome/browser/ui/commander/tab_command_source.h"  // IWYU pragma: export

namespace commander {

// Note: This is implemented in chromium_src because we need access to functions
// in the anonymous namespace of TabCommandSource.
class BraveTabCommandSource : public CommandSource {
 public:
  BraveTabCommandSource();
  ~BraveTabCommandSource() override;

  BraveTabCommandSource(const BraveTabCommandSource& other) = delete;
  BraveTabCommandSource& operator=(const BraveTabCommandSource& other) = delete;

  // Command source overrides
  CommandSource::CommandResults GetCommands(const std::u16string& input,
                                            Browser* browser) const override;
};

}  // namespace commander

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_COMMANDER_TAB_COMMAND_SOURCE_H_

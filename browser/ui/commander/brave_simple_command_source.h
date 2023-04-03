// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDER_BRAVE_SIMPLE_COMMAND_SOURCE_H_
#define BRAVE_BROWSER_UI_COMMANDER_BRAVE_SIMPLE_COMMAND_SOURCE_H_

#include "chrome/browser/ui/commander/command_source.h"

namespace commander {

class BraveSimpleCommandSource : public CommandSource {
 public:
  BraveSimpleCommandSource();
  ~BraveSimpleCommandSource() override;

  BraveSimpleCommandSource(const BraveSimpleCommandSource& other) = delete;
  BraveSimpleCommandSource& operator=(const BraveSimpleCommandSource& other) =
      delete;

  // CommandSource:
  CommandSource::CommandResults GetCommands(const std::u16string& input,
                                            Browser* browser) const override;
};

}  // namespace commander

#endif  // BRAVE_BROWSER_UI_COMMANDER_BRAVE_SIMPLE_COMMAND_SOURCE_H_

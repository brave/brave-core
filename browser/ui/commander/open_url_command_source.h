// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_COMMANDER_OPEN_URL_COMMAND_SOURCE_H_
#define BRAVE_BROWSER_UI_COMMANDER_OPEN_URL_COMMAND_SOURCE_H_

#include <utility>
#include <vector>

#include "brave/browser/ui/commander/command_source.h"
#include "url/gurl.h"

namespace commander {

// A command source for basic commands that open a given URL in a new tab.
class OpenURLCommandSource : public CommandSource {
 public:
  OpenURLCommandSource();
  ~OpenURLCommandSource() override;

  // Disallow copy and assign.
  OpenURLCommandSource(const OpenURLCommandSource& other) = delete;
  OpenURLCommandSource& operator=(const OpenURLCommandSource& other) = delete;

  // CommandSource overrides
  CommandSource::CommandResults GetCommands(const std::u16string& input,
                                            Browser* browser) const override;

 private:
  const std::vector<std::pair<std::u16string, GURL>> title_url_map_;
};

}  // namespace commander

#endif  // BRAVE_BROWSER_UI_COMMANDER_OPEN_URL_COMMAND_SOURCE_H_

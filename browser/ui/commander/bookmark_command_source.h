// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_COMMANDER_BOOKMARK_COMMAND_SOURCE_H_
#define BRAVE_BROWSER_UI_COMMANDER_BOOKMARK_COMMAND_SOURCE_H_

#include "brave/browser/ui/commander/command_source.h"

namespace commander {

// Provides an "Open Bookmark..." composite command which lets the user
// search for a bookmark to open. If the user has typed a minimum threshold
// of characters, this will also return matching individual bookmark commands
// directly.
class BookmarkCommandSource : public CommandSource {
 public:
  BookmarkCommandSource();
  ~BookmarkCommandSource() override;

  BookmarkCommandSource(const BookmarkCommandSource& other) = delete;
  BookmarkCommandSource& operator=(const BookmarkCommandSource& other) = delete;

  // Command source overrides
  CommandSource::CommandResults GetCommands(const std::u16string& input,
                                            Browser* browser) const override;
};

}  // namespace commander

#endif  // BRAVE_BROWSER_UI_COMMANDER_BOOKMARK_COMMAND_SOURCE_H_

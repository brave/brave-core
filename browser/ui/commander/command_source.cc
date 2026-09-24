// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/commander/command_source.h"

#include <variant>

namespace commander {

CommandItem::CommandItem() = default;
CommandItem::CommandItem(const std::u16string& title,
                         double score,
                         const std::vector<gfx::Range>& ranges)
    : title(title), score(score), matched_ranges(ranges) {}
CommandItem::~CommandItem() = default;
CommandItem::CommandItem(CommandItem&& other) = default;
CommandItem& CommandItem::operator=(CommandItem&& other) = default;

CommandItem::Type CommandItem::GetType() {
  if (std::get_if<CompositeCommand>(&command)) {
    return kComposite;
  }
  return kOneShot;
}

}  // namespace commander

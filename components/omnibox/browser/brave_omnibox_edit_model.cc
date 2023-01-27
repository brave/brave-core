// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/brave_omnibox_edit_model.h"

#include "base/strings/string_util.h"
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"

BraveOmniboxEditModel::~BraveOmniboxEditModel() = default;

bool BraveOmniboxEditModel::CanPasteAndGo(const std::u16string& text) const {
  if (commander::CommanderEnabled() &&
      base::StartsWith(text, commander::kCommandPrefix)) {
    return false;
  }

  return OmniboxEditModel::CanPasteAndGo(text);
}

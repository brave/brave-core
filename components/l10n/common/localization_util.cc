/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/localization_util.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave_l10n {

std::u16string GetLocalizedResourceUTF16String(const int resource_id) {
  const std::string resource =
      ui::ResourceBundle::GetSharedInstance().LoadLocalizedResourceString(
          resource_id);

  return base::UTF8ToUTF16(resource);
}

std::u16string GetStringFUTF16WithPlaceHolders(
    int resource_id,
    const std::vector<std::u16string>& placeholders,
    std::vector<size_t>& offsets) {
  std::u16string contents_text =
      l10n_util::GetStringFUTF16(resource_id, placeholders, &offsets);
  CHECK(offsets.size() == placeholders.size());

  // Remove placeholder text from the message. They were only inserted
  // to find the offsets of each placeholder.
  for (const auto& placeholder : placeholders) {
    base::RemoveChars(contents_text, placeholder, &contents_text);
  }

  // Need to adjust offests after removing placeholders.
  int offset_diff = 0;
  for (size_t i = 0; i < offsets.size(); i++) {
    offsets[i] -= offset_diff;
    offset_diff += placeholders[i].length();
  }

  return contents_text;
}

}  // namespace brave_l10n

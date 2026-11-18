/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/omnibox/clean_link_utils.h"

#include <string>

#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/omnibox/omnibox_view.h"
#include "ui/gfx/range/range.h"

namespace brave {

std::optional<GURL> GetSelectedURLForCleanLink(const OmniboxView& omnibox_view,
                                               OmniboxEditModel& edit_model) {
  const gfx::Range selection = omnibox_view.GetSelectionBounds();
  std::u16string selected_text =
      omnibox_view.GetText().substr(selection.GetMin(), selection.length());
  GURL url;
  bool write_url = false;
  edit_model.AdjustTextForCopy(selection.GetMin(), &selected_text, &url,
                               &write_url);
  if (!write_url) {
    return std::nullopt;
  }
  return url;
}

}  // namespace brave

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/dialog_footnote_utils.h"

#include <utility>

#include "base/functional/bind.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "ui/base/page_transition_types.h"

namespace views {

std::unique_ptr<StyledLabel> CreateStyledLabelForDialogFootnote(
    Browser* browser,
    const std::u16string& footnote,
    const std::vector<std::u16string>& replacements,
    const std::vector<GURL>& urls) {
  DCHECK_EQ(replacements.size(), urls.size());

  std::vector<size_t> offsets;
  std::u16string footnote_text =
      base::ReplaceStringPlaceholders(footnote, replacements, &offsets);

  auto label = std::make_unique<StyledLabel>();
  label->SetText(footnote_text);
  label->SetDefaultTextStyle(style::STYLE_SECONDARY);

  auto add_link = [&](size_t idx, GURL url) {
    DCHECK(idx < offsets.size());
    DCHECK(idx < replacements.size());

    gfx::Range link_range(offsets[idx],
                          offsets[idx] + replacements[idx].length());

    StyledLabel::RangeStyleInfo link_style =
        StyledLabel::RangeStyleInfo::CreateForLink(base::BindRepeating(
            [](Browser* browser, const GURL& url) {
              chrome::AddSelectedTabWithURL(browser, url,
                                            ui::PAGE_TRANSITION_LINK);
            },
            base::Unretained(browser), std::move(url)));

    label->AddStyleRange(link_range, link_style);
  };

  for (size_t i = 0; i < urls.size(); ++i) {
    add_link(i, urls[i]);
  }

  return label;
}

}  // namespace views

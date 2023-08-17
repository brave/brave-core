/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_DIALOG_FOOTNOTE_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_DIALOG_FOOTNOTE_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include "ui/views/controls/styled_label.h"
#include "url/gurl.h"

class Browser;

namespace views {

// Creates a styled label with clickable links that can be shown in dialog
// footnotes (such as Permission prompt and similar).
std::unique_ptr<StyledLabel> CreateStyledLabelForDialogFootnote(
    Browser* browser,
    const std::u16string& footnote,
    const std::vector<std::u16string>& replacements,
    const std::vector<GURL>& urls);

}  // namespace views

#endif  // BRAVE_BROWSER_UI_VIEWS_DIALOG_FOOTNOTE_UTILS_H_

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_OMNIBOX_CLEAN_LINK_UTILS_H_
#define BRAVE_BROWSER_UI_OMNIBOX_CLEAN_LINK_UTILS_H_

#include <optional>

#include "url/gurl.h"

class OmniboxEditModel;
class OmniboxView;

namespace brave {

// Returns the URL that "Copy Clean Link" would clean and copy from the
// omnibox's selected text, or nullopt when the selection does not copy as a
// URL.
std::optional<GURL> GetSelectedURLForCleanLink(const OmniboxView& omnibox_view,
                                               OmniboxEditModel& edit_model);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_OMNIBOX_CLEAN_LINK_UTILS_H_

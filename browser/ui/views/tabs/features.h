/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_FEATURES_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_FEATURES_H_

#include "base/compiler_specific.h"

namespace base {
struct LOGICALLY_CONST Feature;
}  // namespace base

namespace tabs {
namespace features {

extern const base::Feature kBraveVerticalTabs;

// Returns true when users chose to use vertical tabs
bool ShouldShowVerticalTabs();

}  // namespace features
}  // namespace tabs

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_FEATURES_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_PROFILE_MENU_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_PROFILE_MENU_VIEW_H_

#define BRAVE_PROFILE_MENU_VIEW_H friend class BraveProfileMenuView;

#define OnExitProfileButtonClicked virtual OnExitProfileButtonClicked
#define BuildAutofillButtons virtual BuildAutofillButtons
#define BuildIdentity virtual BuildIdentity
#define BuildSyncInfo virtual BuildSyncInfo
#define BuildFeatureButtons virtual BuildFeatureButtons

#include "src/chrome/browser/ui/views/profiles/profile_menu_view.h"  // IWYU pragma: export
#undef BuildFeatureButtons
#undef BuildSyncInfo
#undef BuildIdentity
#undef BuildAutofillButtons
#undef OnExitProfileButtonClicked
#undef BRAVE_PROFILE_MENU_VIEW_H

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PROFILES_PROFILE_MENU_VIEW_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "chrome/browser/ui/tabs/split_tab_menu_model.h"

namespace gfx {
struct VectorIcon;
}  // namespace gfx

// <memory> and the other includes above serve the plastered declaration of
// CreateBraveSplitTabMenuModel(), which returns std::unique_ptr.
#include <chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_

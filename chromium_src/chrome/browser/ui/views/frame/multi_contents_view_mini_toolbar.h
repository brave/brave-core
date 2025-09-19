/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_

#include <memory>

#include "chrome/browser/ui/tabs/split_tab_menu_model.h"

namespace gfx {
struct VectorIcon;
}  // namespace gfx

// Both static methods are defined at brave_multi_contents_view_mini_toolbar.cc
// to avoid brave dependency here.
#define UpdateState(...)                                                    \
  virtual UpdateState(__VA_ARGS__);                                         \
  static const gfx::VectorIcon& GetMoreVerticalIcon();                      \
  static std::unique_ptr<ui::SimpleMenuModel> CreateBraveSplitTabMenuModel( \
      TabStripModel* tab_strip_model, SplitTabMenuModel::MenuSource source, \
      int split_tab_index);                                                 \
  FRIEND_TEST_ALL_PREFIXES(SideBySideEnabledBrowserTest, SelectTabTest);    \
  friend class BraveMultiContentsViewMiniToolbar

#include <chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h>  // IWYU pragma: export

#undef UpdateState

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_VIEW_MINI_TOOLBAR_H_

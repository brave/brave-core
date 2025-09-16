/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"

#include <memory>
#include <optional>

#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/tabs/split_tab_menu_model.h"
#include "ui/menus/simple_menu_model.h"

// defined at brave_multi_contents_view_mini_toolbar.cc to avoid
// brave/components/vector_icons dependency here.
const gfx::VectorIcon& GetMoreVerticalIcon();

// defined at brave_split_tab_menu_model.cc to avoid BraveSplitTabMenuModel
// dependency here.
std::unique_ptr<ui::SimpleMenuModel> CreateBraveSplitTabMenuModel(
    TabStripModel* tab_strip_model,
    SplitTabMenuModel::MenuSource source,
    std::optional<int> split_tab_index = std::nullopt);

// Replace menu model with BraveSplitTabMenuModel to use our strings and icons.
#define kMiniToolbar                                                  \
  kMiniToolbar, index);                                               \
  menu_model_ = CreateBraveSplitTabMenuModel(browser_view_->browser() \
                    ->tab_strip_model(),                              \
  SplitTabMenuModel::MenuSource::kMiniToolbar

#define kBrowserToolsChromeRefreshIcon GetMoreVerticalIcon()

#include <chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.cc>

#undef kBrowserToolsChromeRefreshIcon
#undef kMiniToolbar

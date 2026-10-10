/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIDEBAR_BROWSER_SIDEBAR_ITEM_H_
#define BRAVE_COMPONENTS_SIDEBAR_BROWSER_SIDEBAR_ITEM_H_

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/sidebar/common/features.h"
#include "url/gurl.h"

namespace sidebar {

struct SidebarItem {
  enum class Type {
    kTypeBuiltIn,
    kTypeWeb,
  };

  // How the item opens when its sidebar icon is clicked. Orthogonal to Type:
  // a built-in item can open either kind of panel.
  enum class PanelType {
    // Opens in a tab.
    kNone,
    // Opens the SidePanelEntry registered for `built_in_item_type`. Only
    // built-in items can have one.
    kSidePanel,
    // Loads `url` in the sidebar web panel.
    kWebPanel,
  };

  // Underlying values are used as id of items. Use explicit values so
  // conditionally compiled items don't shift others.
  enum class BuiltInItemType {
    kNone = 0,
#if BUILDFLAG(ENABLE_BRAVE_TALK)
    kBraveTalk = 1,
#endif
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
    kWallet = 2,
#endif
    kBookmarks = 3,
    kReadingList = 4,
    kHistory = 5,
#if BUILDFLAG(ENABLE_PLAYLIST)
    kPlaylist = 6,
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
    kChatUI = 7,
#endif
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
    kBraveNews = 8,
#endif
  };

  // Count of built-in items based on enabled features.
  static constexpr size_t kBuiltInItemsCount =
      3  // kBookmarks, kReadingList, kHistory
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
      + 1  // kWallet
#endif
#if BUILDFLAG(ENABLE_PLAYLIST)
      + 1  // kPlaylist
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
      + 1  // kChatUI
#endif
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
      + 1  // kBraveNews
#endif
#if BUILDFLAG(ENABLE_BRAVE_TALK)
      + 1  // kBraveTalk
#endif
      ;

  static SidebarItem Create(const std::u16string& title,
                            Type type,
                            BuiltInItemType built_in_item_type,
                            PanelType panel_type);

  static SidebarItem Create(const GURL& url,
                            const std::u16string& title,
                            Type type,
                            BuiltInItemType built_in_item_type,
                            PanelType panel_type);

  SidebarItem();
  SidebarItem(const SidebarItem&);
  SidebarItem& operator=(const SidebarItem&);
  SidebarItem(SidebarItem&&);
  SidebarItem& operator=(SidebarItem&&);
  ~SidebarItem();

  bool is_built_in_type() const {
    return type == SidebarItem::Type::kTypeBuiltIn;
  }
  bool is_web_type() const { return type == SidebarItem::Type::kTypeWeb; }
  bool is_side_panel_type() const {
    return panel_type == PanelType::kSidePanel;
  }
  // Web panel items open in a tab while the feature is off, so the gate has to
  // apply to opens_in_panel() too.
  bool is_web_panel_type() const {
    return panel_type == PanelType::kWebPanel &&
           base::FeatureList::IsEnabled(features::kSidebarWebPanel);
  }
  bool opens_in_panel() const {
    return is_side_panel_type() || is_web_panel_type();
  }
  bool IsValidItem() const;

  bool operator==(const SidebarItem& item) const;

  GURL url;
  Type type = Type::kTypeBuiltIn;
  BuiltInItemType built_in_item_type = BuiltInItemType::kNone;
  std::u16string title;
  // The item's declared open behavior, independent of whether the web panel
  // feature is currently enabled.
  PanelType panel_type = PanelType::kNone;
};

}  // namespace sidebar

#endif  // BRAVE_COMPONENTS_SIDEBAR_BROWSER_SIDEBAR_ITEM_H_

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_ENTRY_KEY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_ENTRY_KEY_H_

#define SidePanelEntryKey SidePanelEntryKey_ChromiumImpl

#include "src/chrome/browser/ui/views/side_panel/side_panel_entry_key.h"  // // IWYU pragma: export

#undef SidePanelEntryKey

#include "brave/components/sidebar/browser/mobile_view_id.h"

// Subclass for handling mobile view panel. As each mobile view panel item
// should have unique key, its key is composed by using common type and unique
// id.
class SidePanelEntryKey : public SidePanelEntryKey_ChromiumImpl {
 public:
  explicit SidePanelEntryKey(SidePanelEntryId id);
  SidePanelEntryKey(SidePanelEntryId id, extensions::ExtensionId extension_id);
  SidePanelEntryKey(SidePanelEntryId id, sidebar::MobileViewId mobile_view_id);
  ~SidePanelEntryKey() override;

  SidePanelEntryKey(const SidePanelEntryKey& other);
  SidePanelEntryKey& operator=(const SidePanelEntryKey& other);

  bool operator==(const SidePanelEntryKey& other) const;
  auto operator<=>(const SidePanelEntryKey& other) const;

  std::optional<sidebar::MobileViewId> mobile_view_id() const {
    return mobile_view_id_;
  }

 private:
  std::optional<sidebar::MobileViewId> mobile_view_id_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_ENTRY_KEY_H_

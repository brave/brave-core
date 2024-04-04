/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/side_panel/side_panel_entry_key.h"

#define SidePanelEntryKey SidePanelEntryKey_Chromium

#include "src/chrome/browser/ui/side_panel/side_panel_entry_key.cc"

#undef SidePanelEntryKey

SidePanelEntryKey::SidePanelEntryKey(SidePanelEntryId id)
    : SidePanelEntryKey_Chromium(id) {}

SidePanelEntryKey::SidePanelEntryKey(SidePanelEntryId id,
                                     extensions::ExtensionId extension_id)
    : SidePanelEntryKey_Chromium(id, extension_id) {}

SidePanelEntryKey::SidePanelEntryKey(SidePanelEntryId id,
                                     sidebar::MobileViewId mobile_view_id)
    : SidePanelEntryKey(id) {
  CHECK_EQ(id, SidePanelEntryId::kMobileView);

  // Assign here as an initializer for a delegating constructor must appear
  // alone.
  mobile_view_id_ = mobile_view_id;
}

SidePanelEntryKey::SidePanelEntryKey(const SidePanelEntryKey& other) = default;
SidePanelEntryKey& SidePanelEntryKey::operator=(
    const SidePanelEntryKey& other) = default;
SidePanelEntryKey::~SidePanelEntryKey() = default;

bool SidePanelEntryKey::operator==(const SidePanelEntryKey& other) const {
  return SidePanelEntryKey_Chromium::operator==(other) &&
         mobile_view_id_ == other.mobile_view_id();
}

bool SidePanelEntryKey::operator<(const SidePanelEntryKey& other) const {
  if (id() == other.id() && id() == SidePanelEntryId::kMobileView) {
    CHECK(mobile_view_id_.has_value() && other.mobile_view_id_.has_value());
    return mobile_view_id_.value() < other.mobile_view_id_.value();
  }

  return SidePanelEntryKey_Chromium::operator<(other);
}

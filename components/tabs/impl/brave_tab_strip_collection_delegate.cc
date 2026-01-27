// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tabs/public/brave_tab_strip_collection_delegate.h"

namespace tabs {

BraveTabStripCollectionDelegate::BraveTabStripCollectionDelegate(
    BraveTabStripCollection& collection)
    : collection_(collection) {}

BraveTabStripCollectionDelegate::~BraveTabStripCollectionDelegate() = default;

base::PassKey<BraveTabStripCollectionDelegate>
BraveTabStripCollectionDelegate::GetPassKey() const {
  return {};
}

}  // namespace tabs

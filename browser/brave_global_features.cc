/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_global_features.h"

#include "chrome/browser/global_features.h"

BraveGlobalFeatures::BraveGlobalFeatures() = default;

BraveGlobalFeatures::~BraveGlobalFeatures() = default;

BraveGlobalFeatures* BraveGlobalFeatures::FromGlobalFeatures(
    GlobalFeatures* global_features) {
  return static_cast<BraveGlobalFeatures*>(global_features);
}

GlobalFeatures* CreateBraveGlobalFeatures() {
  return new BraveGlobalFeatures();
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_GLOBAL_FEATURES_H_
#define BRAVE_BROWSER_BRAVE_GLOBAL_FEATURES_H_

#include "chrome/browser/global_features.h"

// Brave-specific subclass of GlobalFeatures
// This class owns the core controllers for features that are globally
// scoped on desktop and Android. It can be subclassed by tests to perform
// dependency injection.
class BraveGlobalFeatures : public GlobalFeatures {
 public:
  BraveGlobalFeatures();
  ~BraveGlobalFeatures() override;

  BraveGlobalFeatures(const BraveGlobalFeatures&) = delete;
  BraveGlobalFeatures& operator=(const BraveGlobalFeatures&) = delete;

  static BraveGlobalFeatures* FromGlobalFeatures(
      GlobalFeatures* global_features);
};

#endif  // BRAVE_BROWSER_BRAVE_GLOBAL_FEATURES_H_

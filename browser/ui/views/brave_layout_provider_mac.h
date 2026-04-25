/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_LAYOUT_PROVIDER_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_LAYOUT_PROVIDER_MAC_H_

#include "brave/browser/ui/views/brave_layout_provider.h"

class BraveLayoutProviderMac : public BraveLayoutProvider {
 public:
  using BraveLayoutProvider::BraveLayoutProvider;
  ~BraveLayoutProviderMac() override;

  // BraveLayoutProvider
  int GetCornerRadiusMetric(
      views::ShapeContextTokensOverride token) const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_LAYOUT_PROVIDER_MAC_H_

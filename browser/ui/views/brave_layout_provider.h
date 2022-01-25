/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_LAYOUT_PROVIDER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_LAYOUT_PROVIDER_H_

#include "chrome/browser/ui/views/chrome_layout_provider.h"

class BraveLayoutProvider : public ChromeLayoutProvider {
 public:
  BraveLayoutProvider() = default;
  BraveLayoutProvider(const BraveLayoutProvider&) = delete;
  BraveLayoutProvider& operator=(const BraveLayoutProvider&) = delete;
  ~BraveLayoutProvider() override = default;

  int GetCornerRadiusMetric(views::Emphasis emphasis,
                            const gfx::Size& size = gfx::Size()) const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_LAYOUT_PROVIDER_H_

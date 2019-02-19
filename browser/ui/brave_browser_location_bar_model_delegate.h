/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_BROWSER_LOCATION_BAR_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_BROWSER_LOCATION_BAR_MODEL_DELEGATE_H_

#include "chrome/browser/ui/browser_location_bar_model_delegate.h"

class BraveBrowserLocationBarModelDelegate
    : public BrowserLocationBarModelDelegate {
 public:
  using BrowserLocationBarModelDelegate::BrowserLocationBarModelDelegate;
  ~BraveBrowserLocationBarModelDelegate() override = default;

 private:
  const gfx::VectorIcon* GetVectorIconOverride() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserLocationBarModelDelegate);
};

#endif  // BRAVE_BROWSER_UI_BRAVE_BROWSER_LOCATION_BAR_MODEL_DELEGATE_H_

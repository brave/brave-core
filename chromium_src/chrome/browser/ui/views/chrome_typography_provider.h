// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_CHROME_TYPOGRAPHY_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_CHROME_TYPOGRAPHY_PROVIDER_H_

#include "ui/views/style/typography_provider.h"

#define ChromeTypographyProvider ChromeTypographyProvider_ChromiumImpl
#include "src/chrome/browser/ui/views/chrome_typography_provider.h"
#undef ChromeTypographyProvider

class ChromeTypographyProvider : public ChromeTypographyProvider_ChromiumImpl {
 public:
  using ChromeTypographyProvider_ChromiumImpl::
      ChromeTypographyProvider_ChromiumImpl;

  ChromeTypographyProvider(const ChromeTypographyProvider&) = delete;
  ChromeTypographyProvider& operator=(const ChromeTypographyProvider&) = delete;

  // TypographyProvider:
  SkColor GetColor(const views::View& view,
                   int context,
                   int style) const override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_CHROME_TYPOGRAPHY_PROVIDER_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_INFOBAR_DELEGATE_H_

#include "components/infobars/core/infobar_delegate.h"

class PrefService;

class WebDiscoveryInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  explicit WebDiscoveryInfoBarDelegate(PrefService* prefs);
  WebDiscoveryInfoBarDelegate(const WebDiscoveryInfoBarDelegate&) = delete;
  WebDiscoveryInfoBarDelegate& operator=(const WebDiscoveryInfoBarDelegate&) =
      delete;
  ~WebDiscoveryInfoBarDelegate() override;

  void Close(bool dismiss);
  void EnableWebDiscovery();

 private:
  // InfoBarDelegate overrides
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;
  bool IsCloseable() const override;

  raw_ptr<PrefService> prefs_ = nullptr;
};

#endif  // BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_INFOBAR_DELEGATE_H_

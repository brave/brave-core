/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/web_discovery_infobar_delegate.h"

#include "brave/browser/web_discovery/web_discovery_cta_util.h"
#include "brave/components/constants/pref_names.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"

WebDiscoveryInfoBarDelegate::WebDiscoveryInfoBarDelegate(PrefService* prefs)
    : prefs_(prefs) {}

WebDiscoveryInfoBarDelegate::~WebDiscoveryInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
WebDiscoveryInfoBarDelegate::GetIdentifier() const {
  return WEB_DISCOVERY_INFOBAR_DELEGATE;
}

bool WebDiscoveryInfoBarDelegate::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  return delegate->GetIdentifier() == GetIdentifier();
}

bool WebDiscoveryInfoBarDelegate::IsCloseable() const {
  // To hide default close button. This infobar has custom close button.
  return false;
}

void WebDiscoveryInfoBarDelegate::Close(bool dismiss) {
  if (dismiss) {
    WebDiscoveryCTAState state =
        GetWebDiscoveryCTAState(prefs_, GetWebDiscoveryCurrentCTAId());
    state.dismissed = true;
    SetWebDiscoveryCTAStateToPrefs(prefs_, state);
  }

  infobar()->RemoveSelf();
}

void WebDiscoveryInfoBarDelegate::EnableWebDiscovery() {
  prefs_->SetBoolean(kWebDiscoveryEnabled, true);
  infobar()->RemoveSelf();
}

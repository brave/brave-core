// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/public/test/fakes/fake_web_client.h"

#include "ios/components/webui/web_ui_url_constants.h"

#define AddAdditionalSchemes AddAdditionalSchemes_ChromiumImpl

#include <ios/web/public/test/fakes/fake_web_client.mm>

#undef AddAdditionalSchemes

namespace web {

// Registers kChromeUIUntrustedScheme as a standard scheme too, mirroring
// BraveWebClient::AddAdditionalSchemes (brave/ios/browser/web/
// brave_web_client.mm), so tests that build chrome-untrusted:// origins via
// FakeWebFrame get a real (non-opaque) url::Origin instead of one that's
// silently opaque because the scheme was never registered as standard.
void FakeWebClient::AddAdditionalSchemes(Schemes* schemes) const {
  AddAdditionalSchemes_ChromiumImpl(schemes);
  schemes->standard_schemes.push_back(kChromeUIUntrustedScheme);
}

}  // namespace web

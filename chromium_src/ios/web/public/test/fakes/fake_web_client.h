// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_TEST_FAKES_FAKE_WEB_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_TEST_FAKES_FAKE_WEB_CLIENT_H_

// Adds a _ChromiumImpl sibling for AddAdditionalSchemes (renamed via the
// matching #define in fake_web_client.mm) so Brave's override can register
// kChromeUIUntrustedScheme as a standard scheme too, the same way
// BraveWebClient::AddAdditionalSchemes does for the real app. Without this,
// url::Origin::Create() can't build a real (non-opaque) chrome-untrusted://
// origin in ios/web unit tests using FakeWebFrame, since the scheme registry
// is locked right after WebTestSuite::Initialize() registers FakeWebClient's
// own (unextended) scheme list.
#define plugin_not_supported_text_ \
  plugin_not_supported_text_;      \
  void AddAdditionalSchemes_ChromiumImpl(Schemes* schemes) const

#include <ios/web/public/test/fakes/fake_web_client.h>  // IWYU pragma: export

#undef plugin_not_supported_text_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_TEST_FAKES_FAKE_WEB_CLIENT_H_

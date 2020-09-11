// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/webauthn/chrome_authenticator_request_delegate.h"

#if defined(OS_APPLE)
#include "base/mac/foundation_util.h"

#define BRAVE_WEBAUTHN_KEYCHAIN_ACCESS_GROUP                    \
  const std::vector<std::string>& kBraveKeyChainAccessParts = { \
      "KL8N8XSYF4", base::mac::BaseBundleID(), "webauthn"};     \
  return TouchIdAuthenticatorConfig{                            \
      base::JoinString(kBraveKeyChainAccessParts, "."),         \
      TouchIdMetadataSecret(profile)};
#else
#define BRAVE_WEBAUTHN_KEYCHAIN_ACCESS_GROUP void(0)
#endif

#include "../../../../../chrome/browser/webauthn/chrome_authenticator_request_delegate.cc"

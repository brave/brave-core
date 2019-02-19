/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_content_client.h"

#include "content/public/common/url_constants.h"

BraveContentClient::BraveContentClient() {}

BraveContentClient::~BraveContentClient() {}

void BraveContentClient::AddAdditionalSchemes(Schemes* schemes) {
#if !defined(OS_ANDROID)
  ChromeContentClient::AddAdditionalSchemes(schemes);
  schemes->standard_schemes.push_back(content::kBraveUIScheme);
  schemes->secure_schemes.push_back(content::kBraveUIScheme);
  schemes->cors_enabled_schemes.push_back(content::kBraveUIScheme);
  schemes->savable_schemes.push_back(content::kBraveUIScheme);
#endif
}

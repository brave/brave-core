/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/brave_web_client.h"

#include <string>

#include "base/bind.h"
#include "brave/ios/browser/brave_web_main_parts.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveWebClient::BraveWebClient() : web_main_parts_(nullptr) {}

BraveWebClient::~BraveWebClient() {
}

void BraveWebClient::AddAdditionalSchemes(Schemes* schemes) const {
  schemes->standard_schemes.push_back(kChromeUIScheme);
  schemes->secure_schemes.push_back(kChromeUIScheme);
}

bool BraveWebClient::IsAppSpecificURL(const GURL& url) const {
  return url.SchemeIs(kChromeUIScheme);
}

std::unique_ptr<web::WebMainParts> BraveWebClient::CreateWebMainParts() {
  auto web_main_parts = std::make_unique<BraveWebMainParts>();
  web_main_parts_ = web_main_parts.get();
  return web_main_parts;
}

void BraveWebClient::SetUserAgent(const std::string& user_agent) {
  user_agent_ = user_agent;
}

std::string BraveWebClient::GetUserAgent(web::UserAgentType type) const {
  return user_agent_;
}

base::StringPiece BraveWebClient::GetDataResource(
    int resource_id,
    ui::ResourceScaleFactor scale_factor) const {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* BraveWebClient::GetDataResourceBytes(
    int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

void BraveWebClient::GetAdditionalWebUISchemes(
    std::vector<std::string>* additional_schemes) {}

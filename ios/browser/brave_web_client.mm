/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/brave_web_client.h"

#include <string>

#include "base/bind.h"
#include "brave/ios/browser/brave_web_main_parts.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveWebClient::BraveWebClient() : web_main_parts_(nullptr) {}

BraveWebClient::~BraveWebClient() {
}

std::unique_ptr<web::WebMainParts> BraveWebClient::CreateWebMainParts() {
  auto web_main_parts = std::make_unique<BraveWebMainParts>();
  web_main_parts_ = web_main_parts.get();
  return web_main_parts;
}

std::string BraveWebClient::GetUserAgent(web::UserAgentType type) const {
  // TODO(bridiver) - get this from the main brave ios app
  return "";
}

base::StringPiece BraveWebClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* BraveWebClient::GetDataResourceBytes(
    int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

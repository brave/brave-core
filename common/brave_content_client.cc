/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_content_client.h"

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/components_resources.h"
#include "content/public/common/url_constants.h"
#include "ui/base/resource/resource_bundle.h"

BraveContentClient::BraveContentClient() {}

BraveContentClient::~BraveContentClient() {}

base::RefCountedMemory* BraveContentClient::GetDataResourceBytes(
    int resource_id) {
  if (resource_id == IDR_FLAGS_UI_FLAGS_JS) {
    const ui::ResourceBundle& resource_bundle =
        ui::ResourceBundle::GetSharedInstance();
    const std::string flags_js =
        resource_bundle.LoadDataResourceString(resource_id) +
        resource_bundle.LoadDataResourceString(
            IDR_FLAGS_UI_BRAVE_FLAGS_OVERRIDES_JS);
    base::RefCountedString* bytes = new base::RefCountedString();
    bytes->data().assign(flags_js.data(), flags_js.length());
    return bytes;
  }
  return ChromeContentClient::GetDataResourceBytes(resource_id);
}

void BraveContentClient::AddAdditionalSchemes(Schemes* schemes) {
  ChromeContentClient::AddAdditionalSchemes(schemes);
  schemes->standard_schemes.push_back(content::kBraveUIScheme);
  schemes->secure_schemes.push_back(content::kBraveUIScheme);
  schemes->cors_enabled_schemes.push_back(content::kBraveUIScheme);
  schemes->savable_schemes.push_back(content::kBraveUIScheme);
}

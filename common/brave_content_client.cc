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

BraveContentClient::BraveContentClient() {}

BraveContentClient::~BraveContentClient() {}

base::RefCountedMemory* BraveContentClient::GetDataResourceBytes(
    int resource_id) {
  if (resource_id == IDR_FLAGS_UI_FLAGS_JS) {
    auto* chromium_flags_ui_data =
        ChromeContentClient::GetDataResourceBytes(resource_id);
    auto* brave_flags_ui_data = ChromeContentClient::GetDataResourceBytes(
        IDR_FLAGS_UI_BRAVE_FLAGS_OVERRIDES_JS);
    std::string new_flags_js(chromium_flags_ui_data->front_as<const char>(),
                             chromium_flags_ui_data->size());
    new_flags_js.append(brave_flags_ui_data->front_as<const char>(),
                        brave_flags_ui_data->size());
    return new base::RefCountedStaticMemory(new_flags_js.c_str(),
                                            new_flags_js.length());
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

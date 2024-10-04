/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "components/search_engines/template_url_starter_pack_data.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"

#define GetStarterPackEngines GetStarterPackEngines_ChromiumImpl

#include "src/components/search_engines/template_url_starter_pack_data.cc"
#undef GetStarterPackEngines

namespace {

constexpr char kChromeSchema[] = "chrome://";
constexpr char kBraveSchema[] = "brave://";
}  // namespace

namespace TemplateURLStarterPackData {

std::vector<std::unique_ptr<TemplateURLData>> GetStarterPackEngines() {
  auto t_urls = GetStarterPackEngines_ChromiumImpl();

  // It is necessary to correct urls for the brave schema
  for (auto& t_url : t_urls) {
    std::string_view url(t_url->url());
    if (base::StartsWith(url, kChromeSchema,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      t_url->SetURL(base::StrCat(
          {kBraveSchema, url.substr(std::size(kChromeSchema) - 1)}));
    }
  }

  return t_urls;
}

}  // namespace TemplateURLStarterPackData

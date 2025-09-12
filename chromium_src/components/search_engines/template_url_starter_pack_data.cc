/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/search_engines/template_url_starter_pack_data.h"

#include <string_view>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "components/strings/grit/components_strings.h"

namespace template_url_starter_pack_data {

// Update this whenever a change is made to any Brave-defined starter pack data.
const int kCurrentBraveDataVersion = 1;

// The @ask starter pack sends a query to "Ask Brave". It reuses the kGemini
// starter pack and resource IDs in order to simplify patching and overrides.
const StarterPackEngine ask_brave_search = {
    .name_message_id = IDS_SEARCH_ENGINES_STARTER_PACK_GEMINI_NAME,
    .keyword_message_id = IDS_SEARCH_ENGINES_STARTER_PACK_GEMINI_KEYWORD,
    .favicon_url = nullptr,
    .search_url = "https://search.brave.com/tap?q={searchTerms}",
    .destination_url = "https://search.brave.com",
    .id = StarterPackId::kGemini,
    .type = SEARCH_ENGINE_STARTER_PACK_GEMINI,
};

}  // namespace template_url_starter_pack_data

#define GetStarterPackEngines GetStarterPackEngines_ChromiumImpl
#define GetDataVersion GetDataVersion_ChromiumImpl

// Override the list of starter pack engines, replacing the Gemini starter pack
// with the Ask Brave Search starter pack.
#define BRAVE_TEMPLATE_URL_STARTER_PACK_DATA_ENGINES               \
  &bookmarks, &history, &tabs, &ask_brave_search, &page, &ai_mode, \
  }                                                                \
  ;                                                                \
  const StarterPackEngine* chromium_engines[] = {
#include <components/search_engines/template_url_starter_pack_data.cc>

#undef BRAVE_TEMPLATE_URL_STARTER_PACK_DATA_ENGINES
#undef GetDataVersion
#undef GetStarterPackEngines

namespace {

constexpr char kChromeSchema[] = "chrome://";
constexpr char kBraveSchema[] = "brave://";

}  // namespace

namespace template_url_starter_pack_data {

int GetDataVersion() {
  return GetDataVersion_ChromiumImpl() + kCurrentBraveDataVersion;
}

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

}  // namespace template_url_starter_pack_data

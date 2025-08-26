/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "components/search_engines/template_url_starter_pack_data.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"

#define GetStarterPackEngines GetStarterPackEngines_ChromiumImpl

#include <components/search_engines/template_url_starter_pack_data.cc>

#undef GetStarterPackEngines

namespace {

constexpr char kChromeSchema[] = "chrome://";
constexpr char kBraveSchema[] = "brave://";
}  // namespace

namespace template_url_starter_pack_data {

namespace {

const StarterPackEngine ask_brave_search = {
    .name_message_id = IDS_SEARCH_ENGINES_STARTER_PACK_ASK_BRAVE_SEARCH_NAME,
    .keyword_message_id =
        IDS_SEARCH_ENGINES_STARTER_PACK_ASK_BRAVE_SEARCH_KEYWORD,
    .favicon_url = nullptr,
    .search_url = "https://search.brave.com/tap?q={searchTerms}",
    .destination_url = "https://search.brave.com",
    .id = StarterPackId::kAskBraveSearch,
    .type = SEARCH_ENGINE_STARTER_PACK_ASK_BRAVE_SEARCH,
};

}  // namespace

std::vector<std::unique_ptr<TemplateURLData>> GetStarterPackEngines() {
  auto t_urls = GetStarterPackEngines_ChromiumImpl();

  t_urls.push_back(TemplateURLDataFromStarterPackEngine(ask_brave_search));

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

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"

#include <string>

namespace settings {

BraveSearchEnginesHandler::~BraveSearchEnginesHandler() = default;

std::unique_ptr<base::DictionaryValue>
BraveSearchEnginesHandler::GetSearchEnginesList() {
  auto search_engines_info = SearchEnginesHandler::GetSearchEnginesList();
  // Don't show two brave search entries from settings to prevent confusion.
  // Hide brave search for tor entry from settings UI. User doesn't need to
  // select brave search tor entry for normal profile.
  constexpr char kDefaultsKey[] = "defaults";
  auto* defaults = search_engines_info->FindListKey(kDefaultsKey);
  DCHECK(defaults && defaults->is_list());
  defaults->EraseListValueIf([](const auto& val) {
    DCHECK(val.is_dict());
    constexpr char kKeywordKey[] = "keyword";
    constexpr char kBraveSearchForTorKeyword[] =
        ":search.brave4u7jddbv7cyviptqjc7jusxh72uik7zt6adtckl5f4nwy2v72qd."
        "onion";
    const std::string* keyword = val.FindStringKey(kKeywordKey);
    DCHECK(keyword);
    return *keyword == kBraveSearchForTorKeyword;
  });
  return search_engines_info;
}

}  // namespace settings

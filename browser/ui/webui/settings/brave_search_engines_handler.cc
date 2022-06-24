/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_search_engines_handler.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "chrome/browser/ui/search_engines/template_url_table_model.h"
#include "components/search_engines/template_url.h"

namespace settings {

namespace {
constexpr char kBraveSearchForTorKeyword[] =
    ":search.brave4u7jddbv7cyviptqjc7jusxh72uik7zt6adtckl5f4nwy2v72qd.onion";
}  // namespace

BraveSearchEnginesHandler::BraveSearchEnginesHandler(Profile* profile)
    : SearchEnginesHandler(profile) {}

BraveSearchEnginesHandler::~BraveSearchEnginesHandler() = default;

void BraveSearchEnginesHandler::RegisterMessages() {
  SearchEnginesHandler::RegisterMessages();

  web_ui()->RegisterMessageCallback(
      "getPrivateSearchEnginesList",
      base::BindRepeating(
          &BraveSearchEnginesHandler::HandleGetPrivateSearchEnginesList,
          base::Unretained(this)));
}

void BraveSearchEnginesHandler::OnModelChanged() {
  SearchEnginesHandler::OnModelChanged();

  brave::SetDefaultPrivateSearchProvider(profile_);

  // Sync normal profile's search provider list with private profile's.
  FireWebUIListener("private-search-engines-changed",
                    GetPrivateSearchEnginesList());
}

void BraveSearchEnginesHandler::HandleGetPrivateSearchEnginesList(
    const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  const base::Value& callback_id = args[0];
  AllowJavascript();
  ResolveJavascriptCallback(callback_id, GetPrivateSearchEnginesList());
}

base::Value BraveSearchEnginesHandler::GetPrivateSearchEnginesList() {
  // Construct list with normal profile's default list.
  // Normal and Private profile use same default list.
  int last_default_engine_index =
      list_controller_.table_model()->last_search_engine_index();
  base::Value::List defaults;

  for (int i = 0; i < last_default_engine_index; ++i) {
    const TemplateURL* template_url = list_controller_.GetTemplateURL(i);
    // Don't show two brave search entries from settings to prevent confusion.
    // Hide brave search for tor entry from settings UI. User doesn't need to
    // select brave search tor entry for private profile.
    if (base::UTF16ToUTF8(template_url->keyword()) == kBraveSearchForTorKeyword)
      continue;

    base::Value::Dict dict;
    dict.Set("value", template_url->sync_guid());
    dict.Set("name", template_url->short_name());
    defaults.Append(std::move(dict));
  }

  return base::Value(std::move(defaults));
}

base::Value::Dict BraveSearchEnginesHandler::GetSearchEnginesList() {
  auto search_engines_info = SearchEnginesHandler::GetSearchEnginesList();
  // Don't show two brave search entries from settings to prevent confusion.
  // Hide brave search for tor entry from settings UI. User doesn't need to
  // select brave search tor entry for normal profile.
  auto* defaults = search_engines_info.FindList("defaults");
  DCHECK(defaults);
  defaults->EraseIf([](const auto& val) {
    const auto& dict = val.GetDict();
    const std::string* keyword = dict.FindString("keyword");
    DCHECK(keyword);
    return *keyword == kBraveSearchForTorKeyword;
  });
  return search_engines_info;
}

}  // namespace settings

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service_helper.h"

#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/values.h"

using adblock::FilterList;

namespace brave_shields {

std::vector<FilterList>::const_iterator FindAdBlockFilterListByUUID(
    const std::vector<FilterList>& region_lists,
    const std::string& uuid) {
  std::string uuid_uppercase = base::ToUpperASCII(uuid);
  return std::find_if(region_lists.begin(), region_lists.end(),
                      [&uuid_uppercase](const FilterList& filter_list) {
                        return filter_list.uuid == uuid_uppercase;
                      });
}

std::vector<FilterList>::const_iterator FindAdBlockFilterListByLocale(
    const std::vector<FilterList>& region_lists,
    const std::string& locale) {
  std::string adjusted_locale;
  std::string::size_type loc = locale.find("-");
  if (loc == std::string::npos) {
    adjusted_locale = locale;
  } else {
    adjusted_locale = locale.substr(0, loc);
  }
  adjusted_locale = base::ToLowerASCII(adjusted_locale);
  return std::find_if(
      region_lists.begin(), region_lists.end(),
      [&adjusted_locale](const FilterList& filter_list) {
        return std::find_if(filter_list.langs.begin(), filter_list.langs.end(),
                            [adjusted_locale](const std::string& lang) {
                              return lang == adjusted_locale;
                            }) != filter_list.langs.end();
      });
}

std::vector<FilterList> RegionalCatalogFromJSON(
    const std::string& catalog_json) {
  std::vector<adblock::FilterList> catalog = std::vector<adblock::FilterList>();

  absl::optional<base::Value> regional_lists =
      base::JSONReader::Read(catalog_json);
  if (!regional_lists) {
    LOG(ERROR) << "Could not load regional adblock catalog";
    return catalog;
  }

  for (const auto& regional_list : regional_lists->GetList()) {
    const auto* uuid = regional_list.FindKey("uuid");
    if (!uuid || !uuid->is_string()) {
      continue;
    }
    const auto* url = regional_list.FindKey("url");
    if (!url || !url->is_string()) {
      continue;
    }
    const auto* title = regional_list.FindKey("title");
    if (!title || !title->is_string()) {
      continue;
    }
    std::vector<std::string> langs = std::vector<std::string>();
    const auto* langs_key = regional_list.FindKey("langs");
    if (!langs_key || !langs_key->is_list()) {
      continue;
    }
    for (auto lang = langs_key->GetList().begin();
         lang < langs_key->GetList().end(); lang++) {
      if (!lang->is_string()) {
        continue;
      }
      langs.push_back(lang->GetString());
    }
    const auto* support_url = regional_list.FindKey("support_url");
    if (!support_url || !support_url->is_string()) {
      continue;
    }
    const auto* component_id = regional_list.FindKey("component_id");
    if (!component_id || !component_id->is_string()) {
      continue;
    }
    const auto* base64_public_key = regional_list.FindKey("base64_public_key");
    if (!base64_public_key || !base64_public_key->is_string()) {
      continue;
    }
    const auto* desc = regional_list.FindKey("desc");
    if (!desc || !desc->is_string()) {
      continue;
    }

    catalog.push_back(adblock::FilterList(uuid->GetString(),
                                          url->GetString(),
                                          title->GetString(),
                                          langs,
                                          support_url->GetString(),
                                          component_id->GetString(),
                                          base64_public_key->GetString(),
                                          desc->GetString()));
  }

  return catalog;
}

// Merges the first CSP directive into the second one provided, if they exist.
//
// Distinct policies are merged with comma separators, according to
// https://www.w3.org/TR/CSP2/#implementation-considerations
void MergeCspDirectiveInto(absl::optional<std::string> from,
                           absl::optional<std::string>* into) {
  DCHECK(into);

  if (!from) {
    return;
  }

  if (!*into) {
    *into = from;
    return;
  }

  const std::string from_str = *from;
  const std::string into_str = **into;

  *into = absl::optional<std::string>(from_str + ", " + into_str);
}

// Merges the contents of the first UrlCosmeticResources Value into the second
// one provided.
//
// If `force_hide` is true, the contents of `from`'s `hide_selectors` field
// will be moved into a possibly new field of `into` called
// `force_hide_selectors`.
void MergeResourcesInto(base::Value from, base::Value* into, bool force_hide) {
  base::Value* resources_hide_selectors = nullptr;
  if (force_hide) {
    resources_hide_selectors = into->FindKey("force_hide_selectors");
    if (!resources_hide_selectors || !resources_hide_selectors->is_list()) {
        into->SetKey("force_hide_selectors", base::ListValue());
        resources_hide_selectors = into->FindKey("force_hide_selectors");
    }
  } else {
    resources_hide_selectors = into->FindKey("hide_selectors");
  }
  base::Value* from_resources_hide_selectors =
      from.FindKey("hide_selectors");
  if (resources_hide_selectors && from_resources_hide_selectors) {
    for (auto& selector : from_resources_hide_selectors->GetList()) {
      resources_hide_selectors->Append(std::move(selector));
    }
  }

  base::Value* resources_style_selectors = into->FindKey("style_selectors");
  base::Value* from_resources_style_selectors =
      from.FindKey("style_selectors");
  if (resources_style_selectors && from_resources_style_selectors) {
    for (auto i : from_resources_style_selectors->DictItems()) {
      base::Value* resources_entry =
          resources_style_selectors->FindKey(i.first);
      if (resources_entry) {
        for (auto& item : i.second.GetList()) {
          resources_entry->Append(std::move(item));
        }
      } else {
        resources_style_selectors->SetKey(i.first, std::move(i.second));
      }
    }
  }

  base::Value* resources_exceptions = into->FindKey("exceptions");
  base::Value* from_resources_exceptions = from.FindKey("exceptions");
  if (resources_exceptions && from_resources_exceptions) {
    for (auto& exception : from_resources_exceptions->GetList()) {
      resources_exceptions->Append(std::move(exception));
    }
  }

  base::Value* resources_injected_script = into->FindKey("injected_script");
  base::Value* from_resources_injected_script =
      from.FindKey("injected_script");
  if (resources_injected_script && from_resources_injected_script) {
    *resources_injected_script = base::Value(
            resources_injected_script->GetString()
            + '\n'
            + from_resources_injected_script->GetString());
  }

  base::Value* resources_generichide = into->FindKey("generichide");
  base::Value* from_resources_generichide =
      from.FindKey("generichide");
  if (from_resources_generichide) {
    if (from_resources_generichide->GetBool()) {
      *resources_generichide = base::Value(true);
    }
  }
}

}  // namespace brave_shields

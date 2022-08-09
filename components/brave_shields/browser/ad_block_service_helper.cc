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
#include "base/strings/strcat.h"
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

  absl::optional<base::Value> parsed_json =
      base::JSONReader::Read(catalog_json);
  if (!parsed_json || !parsed_json->is_list()) {
    LOG(ERROR) << "Could not load regional adblock catalog";
    return catalog;
  }

  base::Value::List& regional_lists = parsed_json->GetList();
  for (const auto& item : regional_lists) {
    DCHECK(item.is_dict());
    const base::Value::Dict& regional_list = item.GetDict();

    const auto* uuid = regional_list.FindString("uuid");
    if (!uuid) {
      continue;
    }
    const auto* url = regional_list.FindString("url");
    if (!url) {
      continue;
    }
    const auto* title = regional_list.FindString("title");
    if (!title) {
      continue;
    }
    std::vector<std::string> langs = std::vector<std::string>();
    const auto* langs_key = regional_list.FindList("langs");
    if (!langs_key) {
      continue;
    }
    for (const auto& lang : *langs_key) {
      DCHECK(lang.is_string());
      langs.push_back(lang.GetString());
    }
    const auto* support_url = regional_list.FindString("support_url");
    if (!support_url) {
      continue;
    }
    const auto* component_id = regional_list.FindString("component_id");
    if (!component_id) {
      continue;
    }
    const auto* base64_public_key =
        regional_list.FindString("base64_public_key");
    if (!base64_public_key) {
      continue;
    }
    const auto* desc = regional_list.FindString("desc");
    if (!desc) {
      continue;
    }

    catalog.push_back(adblock::FilterList(*uuid, *url, *title, langs,
                                          *support_url, *component_id,
                                          *base64_public_key, *desc));
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
void MergeResourcesInto(base::Value::Dict from,
                        base::Value::Dict* into,
                        bool force_hide) {
  DCHECK(into);
  base::Value::List* resources_hide_selectors = nullptr;
  if (force_hide) {
    resources_hide_selectors = into->FindList("force_hide_selectors");
    if (!resources_hide_selectors) {
      resources_hide_selectors =
          into->Set("force_hide_selectors", base::Value::List())->GetIfList();
    }
  } else {
    resources_hide_selectors = into->FindList("hide_selectors");
  }
  base::Value::List* from_resources_hide_selectors =
      from.FindList("hide_selectors");
  if (resources_hide_selectors && from_resources_hide_selectors) {
    for (auto& selector : *from_resources_hide_selectors) {
      resources_hide_selectors->Append(std::move(selector));
    }
  }

  base::Value::Dict* resources_style_selectors =
      into->FindDict("style_selectors");
  base::Value::Dict* from_resources_style_selectors =
      from.FindDict("style_selectors");
  if (resources_style_selectors && from_resources_style_selectors) {
    for (auto [key, value] : *from_resources_style_selectors) {
      base::Value::List* resources_entry =
          resources_style_selectors->FindList(key);
      if (resources_entry) {
        DCHECK(value.is_list());
        for (auto& item : value.GetList()) {
          resources_entry->Append(std::move(item));
        }
      } else {
        resources_style_selectors->Set(key, std::move(value));
      }
    }
  }

  base::Value::List* resources_exceptions = into->FindList("exceptions");
  base::Value::List* from_resources_exceptions = from.FindList("exceptions");
  if (resources_exceptions && from_resources_exceptions) {
    for (auto& exception : *from_resources_exceptions) {
      resources_exceptions->Append(std::move(exception));
    }
  }

  auto* resources_injected_script = into->FindString("injected_script");
  auto* from_resources_injected_script = from.FindString("injected_script");
  if (resources_injected_script && from_resources_injected_script) {
    *resources_injected_script = base::StrCat(
        {*resources_injected_script, "\n", *from_resources_injected_script});
  }

  auto from_resources_generichide = from.FindBool("generichide");
  if (from_resources_generichide && *from_resources_generichide) {
    into->Set("generichide", true);
  }
}

}  // namespace brave_shields

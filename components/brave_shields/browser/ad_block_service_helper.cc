/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service_helper.h"

#include <algorithm>
#include <utility>

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

// Merges the contents of the second UrlCosmeticResources Value into the first
// one provided.
//
// If `force_hide` is true, the contents of `from`'s `hide_selectors` field
// will be moved into a possibly new field of `into` called
// `force_hide_selectors`.
void MergeResourcesInto(
        base::Value* into,
        base::Value* from,
        bool force_hide) {
  base::Value* resources_hide_selectors = nullptr;
  if (force_hide) {
    resources_hide_selectors = into->FindKey("force_hide_selectors");
    if (!resources_hide_selectors || !resources_hide_selectors->is_list()) {
        into->SetPath("force_hide_selectors", base::ListValue());
        resources_hide_selectors = into->FindKey("force_hide_selectors");
    }
  } else {
    resources_hide_selectors = into->FindKey("hide_selectors");
  }
  base::Value* from_resources_hide_selectors =
      from->FindKey("hide_selectors");
  if (resources_hide_selectors && from_resources_hide_selectors) {
    for (auto i = from_resources_hide_selectors->GetList().begin();
            i < from_resources_hide_selectors->GetList().end();
            i++) {
      resources_hide_selectors->Append(std::move(*i));
    }
  }

  base::Value* resources_style_selectors = into->FindKey("style_selectors");
  base::Value* from_resources_style_selectors =
      from->FindKey("style_selectors");
  if (resources_style_selectors && from_resources_style_selectors) {
    for (auto i : from_resources_style_selectors->DictItems()) {
      base::Value* resources_entry =
          resources_style_selectors->FindKey(i.first);
      if (resources_entry) {
        for (auto j = i.second.GetList().begin();
                j < i.second.GetList().end();
                j++) {
          resources_entry->Append(std::move(*j));
        }
      } else {
        resources_style_selectors->SetPath(i.first, std::move(i.second));
      }
    }
  }

  base::Value* resources_exceptions = into->FindKey("exceptions");
  base::Value* from_resources_exceptions = from->FindKey("exceptions");
  if (resources_exceptions && from_resources_exceptions) {
    for (auto i = from_resources_exceptions->GetList().begin();
            i < from_resources_exceptions->GetList().end();
            i++) {
      resources_exceptions->Append(std::move(*i));
    }
  }

  base::Value* resources_injected_script = into->FindKey("injected_script");
  base::Value* from_resources_injected_script =
      from->FindKey("injected_script");
  if (resources_injected_script && from_resources_injected_script) {
    *resources_injected_script = base::Value(
            resources_injected_script->GetString()
            + '\n'
            + from_resources_injected_script->GetString());
  }

  base::Value* resources_generichide = into->FindKey("generichide");
  base::Value* from_resources_generichide =
      from->FindKey("generichide");
  if (from_resources_generichide) {
    if (from_resources_generichide->GetBool()) {
      *resources_generichide = base::Value(true);
    }
  }
}

}  // namespace brave_shields

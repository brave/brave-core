// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_service_helper.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/values.h"

namespace brave_shields {

// Merges the first CSP directive into the second one provided, if they exist.
//
// Distinct policies are merged with comma separators, according to
// https://www.w3.org/TR/CSP2/#implementation-considerations
void MergeCspDirectiveInto(std::optional<std::string> from,
                           std::optional<std::string>* into) {
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

  *into = std::optional<std::string>(from_str + ", " + into_str);
}

// Merges the contents of the first UrlCosmeticResources Value into the second
// one provided.
//
// If `force_hide` is true, the contents of `from`'s `hide_selectors` field
// will be moved into a possibly new field of `into` called
// `force_hide_selectors`.
void MergeResourcesInto(base::Value::Dict from,
                        base::Value::Dict& into,
                        bool force_hide) {
  base::Value::List* resources_hide_selectors = nullptr;
  if (force_hide) {
    resources_hide_selectors = into.FindList("force_hide_selectors");
    if (!resources_hide_selectors) {
      resources_hide_selectors =
          into.Set("force_hide_selectors", base::Value::List())->GetIfList();
    }
  } else {
    resources_hide_selectors = into.FindList("hide_selectors");
  }
  base::Value::List* from_resources_hide_selectors =
      from.FindList("hide_selectors");
  if (resources_hide_selectors && from_resources_hide_selectors) {
    for (auto& selector : *from_resources_hide_selectors) {
      resources_hide_selectors->Append(std::move(selector));
    }
  }

  constexpr std::string_view kListKeys[] = {"exceptions", "procedural_actions"};
  for (const auto& key_ : kListKeys) {
    base::Value::List* resources = into.FindList(key_);
    base::Value::List* from_resources = from.FindList(key_);
    if (resources && from_resources) {
      for (auto& exception : *from_resources) {
        resources->Append(std::move(exception));
      }
    }
  }

  auto* resources_injected_script = into.FindString("injected_script");
  auto* from_resources_injected_script = from.FindString("injected_script");
  if (resources_injected_script && from_resources_injected_script) {
    *resources_injected_script = base::StrCat(
        {*resources_injected_script, "\n", *from_resources_injected_script});
  }

  auto from_resources_generichide = from.FindBool("generichide");
  if (from_resources_generichide && *from_resources_generichide) {
    into.Set("generichide", true);
  }
}

// Removes any procedural filters from the given UrlCosmeticResources Value.
//
// Procedural filters are filters with at least one selector operator of a type
// that isn't `css-selector`.
//
// These filters are represented as JSON provided by adblock-rust. The format
// is documented at:
// https://docs.rs/adblock/latest/adblock/cosmetic_filter_cache/struct.ProceduralOrActionFilter.html
void StripProceduralFilters(base::Value::Dict& resources) {
  base::Value::List* procedural_actions =
      resources.FindList("procedural_actions");
  if (procedural_actions) {
    base::Value::List::iterator it = procedural_actions->begin();
    while (it < procedural_actions->end()) {
      DCHECK(it->is_string());
      auto* pfilter_str = it->GetIfString();
      if (pfilter_str == nullptr) {
        continue;
      }
      auto val = base::JSONReader::ReadDict(*pfilter_str);
      if (val) {
        auto* list = val->FindList("selector");
        if (list && list->size() != 1) {
          // Non-procedural filters are always a single operator in length.
          it = procedural_actions->erase(it);
          continue;
        }
        // The single operator must also be a `css-selector`.
        auto op_iterator = list->begin();
        auto* dict = op_iterator->GetIfDict();
        if (dict) {
          auto* str = dict->FindString("type");
          if (str && *str != "css-selector") {
            it = procedural_actions->erase(it);
            continue;
          }
        }
      }
      it++;
    }
  }
}

}  // namespace brave_shields

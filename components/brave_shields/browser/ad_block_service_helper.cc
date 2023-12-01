/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service_helper.h"

#include <optional>
#include <utility>

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

  base::Value::Dict* resources_style_selectors =
      into.FindDict("style_selectors");
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

  base::Value::List* resources_exceptions = into.FindList("exceptions");
  base::Value::List* from_resources_exceptions = from.FindList("exceptions");
  if (resources_exceptions && from_resources_exceptions) {
    for (auto& exception : *from_resources_exceptions) {
      resources_exceptions->Append(std::move(exception));
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

}  // namespace brave_shields

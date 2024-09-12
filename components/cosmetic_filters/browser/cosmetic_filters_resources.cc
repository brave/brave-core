/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/browser/cosmetic_filters_resources.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

namespace {

const char kProceduralActionsScript[] =
    R"((function() {
          const CC = window.content_cosmetic;
          let stylesheet = '';
          const takeStyleFilter = filter => {
            if (filter.selector.length === 1 && filter.selector[0].type === 'css-selector' && filter.action && filter.action.type === 'style') {
              stylesheet += filter.selector[0].arg + '{' + filter.action.arg + '}\n';
              return false;
            }
            return $1;
          };
          CC.proceduralActionFilters = JSON.parse(String.raw`$2`).filter(f => takeStyleFilter(f));
          CC.hasProceduralActions = CC.proceduralActionFilters.length > 0;
          return stylesheet;
        })();)";

}  // namespace

namespace cosmetic_filters {

CosmeticFiltersResources::CosmeticFiltersResources(
    brave_shields::AdBlockService* ad_block_service)
    : ad_block_service_(ad_block_service) {}

CosmeticFiltersResources::~CosmeticFiltersResources() = default;

void CosmeticFiltersResources::HiddenClassIdSelectors(
    const std::string& input,
    const std::vector<std::string>& exceptions,
    HiddenClassIdSelectorsCallback callback) {
  DCHECK(ad_block_service_->GetTaskRunner()->RunsTasksInCurrentSequence());
  std::optional<base::Value> input_value = base::JSONReader::Read(input);
  if (!input_value) {
    // Nothing to work with
    std::move(callback).Run(base::Value::Dict());

    return;
  }
  base::Value::Dict* input_dict = input_value->GetIfDict();
  if (!input_dict) {
    std::move(callback).Run(base::Value::Dict());
    return;
  }
  std::vector<std::string> classes;
  base::Value::List* classes_list = input_dict->FindList("classes");
  if (classes_list) {
    for (const auto& class_item : *classes_list) {
      if (!class_item.is_string()) {
        continue;
      }
      classes.push_back(class_item.GetString());
    }
  }
  std::vector<std::string> ids;
  base::Value::List* ids_list = input_dict->FindList("ids");
  if (ids_list) {
    for (const auto& id_item : *ids_list) {
      if (!id_item.is_string()) {
        continue;
      }
      ids.push_back(id_item.GetString());
    }
  }

  auto selectors =
      ad_block_service_->HiddenClassIdSelectors(classes, ids, exceptions);

  std::move(callback).Run(std::move(selectors));
}

void CosmeticFiltersResources::UrlCosmeticResources(
    const std::string& url,
    bool aggressive_blocking,
    UrlCosmeticResourcesCallback callback) {
  DCHECK(ad_block_service_->GetTaskRunner()->RunsTasksInCurrentSequence());
  auto resources =
      ad_block_service_->UrlCosmeticResources(url, aggressive_blocking);

  const auto* procedural_actions_list =
      resources.FindList("procedural_actions");
  if (procedural_actions_list && !procedural_actions_list->empty()) {
    const char* procedural_filtering_feature_enabled =
        base::FeatureList::IsEnabled(
            brave_shields::features::kBraveAdblockProceduralFiltering)
            ? "true"
            : "false";

    // Each element of procedural_actions_list is already formatted as JSON.
    // Combine them into a single JSON list using string concatenation to avoid
    // double-escaping.
    auto procedural_actions_strings = std::vector<std::string>();
    std::transform(procedural_actions_list->cbegin(),
                   procedural_actions_list->cend(),
                   std::back_inserter(procedural_actions_strings),
                   [](const base::Value& action) -> std::string {
                     return action.GetString();
                   });
    std::string procedural_actions_json = base::StrCat(
        {"[", base::JoinString(procedural_actions_strings, ","), "]"});
    std::string procedural_actions_script = base::ReplaceStringPlaceholders(
        kProceduralActionsScript,
        {procedural_filtering_feature_enabled, procedural_actions_json.c_str()},
        nullptr);
    resources.Set("procedural_actions_script", procedural_actions_script);
  }
  resources.Remove("procedural_actions");

  std::move(callback).Run(base::Value(std::move(resources)));
}

}  // namespace cosmetic_filters

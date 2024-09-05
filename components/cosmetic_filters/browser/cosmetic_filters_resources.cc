/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/browser/cosmetic_filters_resources.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
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
            return true;
          };
          CC.proceduralActionFilters = JSON.parse(String.raw`%s`).filter(f => takeStyleFilter(f));
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

  std::string procedural_actions_json;
  const auto* procedural_actions_list =
      resources.FindList("procedural_actions");
  if (procedural_actions_list && !procedural_actions_list->empty()) {
    // Each element of procedural_actions_list is already formatted as JSON.
    // Combine them into a single JSON list using string operations to avoid
    // double-escaping.
    procedural_actions_json = "[";
    for (const auto& procedural_action : *procedural_actions_list) {
      procedural_actions_json += procedural_action.GetString() + ',';
    }
    // Close the list by replacing the trailing comma.
    procedural_actions_json[procedural_actions_json.length() - 1] = ']';
    std::string procedural_actions_script = base::StringPrintf(
        kProceduralActionsScript, procedural_actions_json.c_str());
    resources.Set("procedural_actions_script", procedural_actions_script);
  }
  resources.Remove("procedural_actions");

  std::move(callback).Run(base::Value(std::move(resources)));
}

}  // namespace cosmetic_filters

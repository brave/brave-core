/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/browser/cosmetic_filters_resources.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace cosmetic_filters {

CosmeticFiltersResources::CosmeticFiltersResources(
    brave_shields::AdBlockService* ad_block_service)
    : ad_block_service_(ad_block_service) {}

CosmeticFiltersResources::~CosmeticFiltersResources() {}

void CosmeticFiltersResources::HiddenClassIdSelectors(
    const std::string& input,
    const std::vector<std::string>& exceptions,
    HiddenClassIdSelectorsCallback callback) {
  DCHECK(ad_block_service_->GetTaskRunner()->RunsTasksInCurrentSequence());
  absl::optional<base::Value> input_value = base::JSONReader::Read(input);
  if (!input_value || !input_value->is_dict()) {
    // Nothing to work with
    std::move(callback).Run(base::Value());

    return;
  }
  base::DictionaryValue* input_dict;
  if (!input_value->GetAsDictionary(&input_dict)) {
    std::move(callback).Run(base::Value());

    return;
  }
  std::vector<std::string> classes;
  base::ListValue* classes_list;
  if (input_dict->GetList("classes", &classes_list)) {
    for (const auto& class_item : classes_list->GetList()) {
      if (!class_item.is_string()) {
        continue;
      }
      classes.push_back(class_item.GetString());
    }
  }
  std::vector<std::string> ids;
  base::ListValue* ids_list;
  if (input_dict->GetList("ids", &ids_list)) {
    for (const auto& id_item : ids_list->GetList()) {
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
    UrlCosmeticResourcesCallback callback) {
  DCHECK(ad_block_service_->GetTaskRunner()->RunsTasksInCurrentSequence());
  auto resources = ad_block_service_->UrlCosmeticResources(url);
  std::move(callback).Run(resources ? std::move(resources.value())
                                    : base::Value());
}

}  // namespace cosmetic_filters

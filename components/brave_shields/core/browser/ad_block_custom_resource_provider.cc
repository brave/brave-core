// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"

#include <string>
#include <string_view>

#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_shields {

namespace {

constexpr const char kNameField[] = "name";
constexpr const char kContentField[] = "content";
constexpr const char kKindField[] = "kind";
constexpr const char kMimeField[] = "mime";

bool IsValidResource(const base::Value& resource) {
  if (!resource.is_dict() || !resource.GetDict().FindString(kNameField) ||
      !resource.GetDict().FindString(kContentField)) {
    // Invalid resource structure.
    return false;
  }

  const auto* name = resource.GetDict().FindString(kNameField);
  if (name->empty() || !base::IsStringASCII(*name)) {
    return false;
  }

  auto* kind = resource.GetDict().FindDict(kKindField);
  if (!kind || !kind->FindString(kMimeField)) {
    return false;
  }
  return true;
}

const std::string& GetResourceName(const base::Value& resource) {
  DCHECK(IsValidResource(resource));
  return *resource.GetDict().FindString(kNameField);
}

base::Value::List::iterator FindResource(base::Value::List& resources,
                                         const std::string& name) {
  return base::ranges::find_if(resources, [name](const base::Value& v) {
    if (!IsValidResource(v)) {
      return false;
    }
    return GetResourceName(v) == name;
  });
}

std::string_view JsonListStr(std::string_view json) {
  const auto start = json.find('[');
  const auto end = json.rfind(']');
  if (start == std::string_view::npos || end == std::string_view::npos ||
      start >= end) {
    return std::string_view();
  }
  return json.substr(start + 1, end - start - 1);
}

std::string MergeResources(const std::string& default_resources,
                           const std::string& custom_resources) {
  auto default_resources_str = JsonListStr(default_resources);
  if (default_resources_str.empty()) {
    return custom_resources;
  }
  auto custom_resources_str = JsonListStr(custom_resources);
  if (custom_resources_str.empty()) {
    return default_resources;
  }
  return base::StrCat(
      {"[", default_resources_str, ",", custom_resources_str, "]"});
}

base::Value FixName(const base::Value& resource) {
  base::Value result = resource.Clone();

  std::string* name = result.GetDict().FindString(kNameField);
  *name = base::ToLowerASCII(*name);
  if (!base::EndsWith(*name, ".js")) {
    name->append(".js");
  }

  return result;
}

}  // namespace

AdBlockCustomResourceProvider::AdBlockCustomResourceProvider(
    PrefService* local_state,
    std::unique_ptr<AdBlockResourceProvider> default_resource_provider)
    : local_state_(local_state),
      default_resource_provider_(std::move(default_resource_provider)) {
  CHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kCosmeticFilteringCustomScriptlets));
  CHECK(default_resource_provider_);
  default_resource_provider_->AddObserver(this);
}

AdBlockCustomResourceProvider::~AdBlockCustomResourceProvider() {
  default_resource_provider_->RemoveObserver(this);
}

const base::Value& AdBlockCustomResourceProvider::GetCustomResources() {
  return local_state_->GetValue(prefs::kAdBlockCustomResources);
}

AdBlockCustomResourceProvider::ErrorCode
AdBlockCustomResourceProvider::AddResource(const base::Value& resource) {
  if (!IsValidResource(resource)) {
    return ErrorCode::kInvalid;
  }

  ScopedListPrefUpdate update(local_state_, prefs::kAdBlockCustomResources);
  if (FindResource(update.Get(), GetResourceName(resource)) != update->end()) {
    return ErrorCode::kAlreadyExists;
  }
  update->Append(FixName(resource));
  ReloadResourcesAndNotify();
  return ErrorCode::kOk;
}

AdBlockCustomResourceProvider::ErrorCode
AdBlockCustomResourceProvider::UpdateResource(const std::string& old_name,
                                              const base::Value& resource) {
  if (!IsValidResource(resource)) {
    return ErrorCode::kInvalid;
  }
  ScopedListPrefUpdate update(local_state_, prefs::kAdBlockCustomResources);
  auto updated_resource = FindResource(update.Get(), old_name);
  if (updated_resource == update->end()) {
    return ErrorCode::kNotFound;
  }
  const std::string& new_name = GetResourceName(resource);
  if (old_name != new_name) {
    if (FindResource(update.Get(), new_name) != update->end()) {
      return ErrorCode::kNotFound;
    }
  }

  *updated_resource = FixName(resource);
  ReloadResourcesAndNotify();
  return ErrorCode::kOk;
}

AdBlockCustomResourceProvider::ErrorCode
AdBlockCustomResourceProvider::RemoveResource(
    const std::string& resource_name) {
  ScopedListPrefUpdate update(local_state_, prefs::kAdBlockCustomResources);
  auto updated_resource = FindResource(update.Get(), resource_name);
  update->erase(updated_resource);
  ReloadResourcesAndNotify();
  return ErrorCode::kOk;
}

void AdBlockCustomResourceProvider::LoadResources(
    base::OnceCallback<void(const std::string& resources_json)> on_load) {
  default_resource_provider_->LoadResources(
      base::BindOnce(&AdBlockCustomResourceProvider::OnDefaultResourcesLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_load)));
}

void AdBlockCustomResourceProvider::OnResourcesLoaded(
    const std::string& resources_json) {
  NotifyResourcesLoaded(
      MergeResources(resources_json, GetCustomResourcesJson()));
}

std::string AdBlockCustomResourceProvider::GetCustomResourcesJson() {
  return base::WriteJson(GetCustomResources()).value_or(std::string());
}

void AdBlockCustomResourceProvider::OnDefaultResourcesLoaded(
    base::OnceCallback<void(const std::string& resources_json)> on_load,
    const std::string& resources_json) {
  const auto& custom_resources =
      local_state_->GetList(prefs::kAdBlockCustomResources);
  std::string custom_resources_json =
      base::WriteJson(custom_resources).value_or(std::string());

  if (custom_resources.empty()) {
    std::move(on_load).Run(resources_json);
  } else if (resources_json.empty()) {
    std::move(on_load).Run(custom_resources_json);
  } else {
    // Merge.
    std::move(on_load).Run(
        MergeResources(resources_json, custom_resources_json));
  }
}

void AdBlockCustomResourceProvider::ReloadResourcesAndNotify() {
  LoadResources(
      base::BindOnce(&AdBlockCustomResourceProvider::NotifyResourcesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace brave_shields

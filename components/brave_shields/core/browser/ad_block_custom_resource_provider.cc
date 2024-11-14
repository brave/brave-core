// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_custom_resource_provider.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/value_store/value_store_factory_impl.h"
#include "components/value_store/value_store_frontend.h"
#include "components/value_store/value_store_task_runner.h"

namespace brave_shields {

namespace {
constexpr const char kStorageUMA[] = "AdBlock Custom Resources";
constexpr const base::FilePath::CharType kStorageName[] =
    FILE_PATH_LITERAL("AdBlock Custom Resources");
constexpr const char kStorageScriptletsKey[] = "SCRIPTLETS";

constexpr const char kNameField[] = "name";
constexpr const char kContentField[] = "content";
constexpr const char kMimeField[] = "kind.mime";
constexpr const char kAppJs[] = "application/javascript";

bool HasName(const base::Value& resource) {
  return resource.is_dict() && !!resource.GetDict().FindString(kNameField);
}

bool IsValidResource(const base::Value& resource) {
  if (!HasName(resource)) {
    return false;
  }
  if (!resource.GetDict().FindString(kContentField)) {
    // Invalid resource structure.
    return false;
  }

  const auto* name = resource.GetDict().FindString(kNameField);
  if (name->empty() || !base::IsStringASCII(*name)) {
    return false;
  }

  const auto* mime = resource.GetDict().FindStringByDottedPath(kMimeField);
  if (!mime) {
    return false;
  }

  if (*mime == kAppJs) {
    // Resource is a scriptlet:
    if (!name->starts_with("brave-") || !name->ends_with(".js")) {
      return false;
    }
  } else {
    return false;
  }

  return true;
}

const std::string& GetResourceName(const base::Value& resource) {
  if (!HasName(resource)) {
    return base::EmptyString();
  }
  return *resource.GetDict().FindString(kNameField);
}

base::Value::List::iterator FindResource(base::Value::List& resources,
                                         const std::string& name) {
  return base::ranges::find_if(resources, [name](const base::Value& v) {
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

}  // namespace

AdBlockCustomResourceProvider::AdBlockCustomResourceProvider(
    const base::FilePath& storage_root,
    std::unique_ptr<AdBlockResourceProvider> default_resource_provider)
    : default_resource_provider_(std::move(default_resource_provider)) {
  CHECK(base::FeatureList::IsEnabled(
      brave_shields::features::kCosmeticFilteringCustomScriptlets));
  CHECK(default_resource_provider_);
  auto factory =
      base::MakeRefCounted<value_store::ValueStoreFactoryImpl>(storage_root);
  storage_ = std::make_unique<value_store::ValueStoreFrontend>(
      std::move(factory), base::FilePath(kStorageName), kStorageUMA,
      base::SequencedTaskRunner::GetCurrentDefault(),
      value_store::GetValueStoreTaskRunner());
  default_resource_provider_->AddObserver(this);
}

AdBlockCustomResourceProvider::~AdBlockCustomResourceProvider() {
  default_resource_provider_->RemoveObserver(this);
}

void AdBlockCustomResourceProvider::EnableDeveloperMode(bool enabled) {
  if (developer_mode_enabled_ == enabled) {
    return;
  }
  developer_mode_enabled_ = enabled;
  ReloadResourcesAndNotify();
}

void AdBlockCustomResourceProvider::GetCustomResources(
    base::OnceCallback<void(base::Value)> callback) {
  if (!developer_mode_enabled_) {
    return std::move(callback).Run(base::Value(base::Value::Type::LIST));
  }

  storage_->Get(
      kStorageScriptletsKey,
      base::BindOnce(
          [](base::OnceCallback<void(base::Value)> callback,
             std::optional<base::Value> value) {
            if (value && value->is_list()) {
              std::move(callback).Run(std::move(*value));
            } else {
              std::move(callback).Run(base::Value(base::Value::Type::LIST));
            }
          },
          std::move(callback)));
}

void AdBlockCustomResourceProvider::AddResource(const base::Value& resource,
                                                StatusCallback on_complete) {
  if (!IsValidResource(resource)) {
    return std::move(on_complete).Run(ErrorCode::kInvalid);
  }
  GetCustomResources(
      base::BindOnce(&AdBlockCustomResourceProvider::AddResourceInternal,
                     weak_ptr_factory_.GetWeakPtr(), resource.Clone(),
                     std::move(on_complete)));
}

void AdBlockCustomResourceProvider::UpdateResource(const std::string& old_name,
                                                   const base::Value& resource,
                                                   StatusCallback on_complete) {
  if (!IsValidResource(resource)) {
    return std::move(on_complete).Run(ErrorCode::kInvalid);
  }
  GetCustomResources(
      base::BindOnce(&AdBlockCustomResourceProvider::UpdateResourceInternal,
                     weak_ptr_factory_.GetWeakPtr(), old_name, resource.Clone(),
                     std::move(on_complete)));
}

void AdBlockCustomResourceProvider::RemoveResource(
    const std::string& resource_name,
    StatusCallback on_complete) {
  GetCustomResources(base::BindOnce(
      &AdBlockCustomResourceProvider::RemoveResourceInternal,
      weak_ptr_factory_.GetWeakPtr(), resource_name, std::move(on_complete)));
}

void AdBlockCustomResourceProvider::LoadResources(
    base::OnceCallback<void(const std::string& resources_json)> on_load) {
  default_resource_provider_->LoadResources(
      base::BindOnce(&AdBlockCustomResourceProvider::OnDefaultResourcesLoaded,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_load)));
}

void AdBlockCustomResourceProvider::OnResourcesLoaded(
    const std::string& resources_json) {
  OnDefaultResourcesLoaded(
      base::BindOnce(&AdBlockCustomResourceProvider::NotifyResourcesLoaded,
                     weak_ptr_factory_.GetWeakPtr()),
      resources_json);
}

void AdBlockCustomResourceProvider::AddResourceInternal(
    base::Value resource,
    StatusCallback on_complete,
    base::Value resources) {
  CHECK(resources.is_list());
  auto& list = resources.GetList();
  if (FindResource(list, GetResourceName(resource)) != list.end()) {
    return std::move(on_complete).Run(ErrorCode::kAlreadyExists);
  }
  list.Append(std::move(resource));
  SaveResources(std::move(resources));
  ReloadResourcesAndNotify();
  std::move(on_complete).Run(ErrorCode::kOk);
}

void AdBlockCustomResourceProvider::UpdateResourceInternal(
    const std::string& old_name,
    base::Value resource,
    StatusCallback on_complete,
    base::Value resources) {
  CHECK(resources.is_list());
  auto updated_resource = FindResource(resources.GetList(), old_name);
  if (updated_resource == resources.GetList().end()) {
    return std::move(on_complete).Run(ErrorCode::kNotFound);
  }

  const std::string& new_name = GetResourceName(resource);
  if (old_name != new_name) {
    if (FindResource(resources.GetList(), new_name) !=
        resources.GetList().end()) {
      return std::move(on_complete).Run(ErrorCode::kNotFound);
    }
  }

  *updated_resource = std::move(resource);
  SaveResources(std::move(resources));
  ReloadResourcesAndNotify();
  std::move(on_complete).Run(ErrorCode::kOk);
}

void AdBlockCustomResourceProvider::RemoveResourceInternal(
    const std::string& name,
    StatusCallback on_complete,
    base::Value resources) {
  CHECK(resources.is_list());
  auto updated_resource = FindResource(resources.GetList(), name);
  if (updated_resource != resources.GetList().end()) {
    resources.GetList().erase(updated_resource);
    SaveResources(std::move(resources));
    ReloadResourcesAndNotify();
    std::move(on_complete).Run(ErrorCode::kOk);
  } else {
    std::move(on_complete).Run(ErrorCode::kNotFound);
  }
}

void AdBlockCustomResourceProvider::SaveResources(base::Value resources) {
  storage_->Set(kStorageScriptletsKey, std::move(resources));
}

void AdBlockCustomResourceProvider::OnDefaultResourcesLoaded(
    base::OnceCallback<void(const std::string& resources_json)> on_load,
    const std::string& resources_json) {
  GetCustomResources(base::BindOnce(
      &AdBlockCustomResourceProvider::OnCustomResourcesLoaded,
      weak_ptr_factory_.GetWeakPtr(), std::move(on_load), resources_json));
}

void AdBlockCustomResourceProvider::OnCustomResourcesLoaded(
    base::OnceCallback<void(const std::string& resources_json)> on_load,
    const std::string& default_resources_json,
    base::Value custom_resources) {
  CHECK(custom_resources.is_list());

  if (custom_resources.GetList().empty()) {
    std::move(on_load).Run(default_resources_json);
  } else {
    auto custom_resources_json = base::WriteJson(custom_resources);
    if (!custom_resources_json) {
      std::move(on_load).Run(default_resources_json);
    } else {
      std::move(on_load).Run(
          MergeResources(default_resources_json, *custom_resources_json));
    }
  }
}

void AdBlockCustomResourceProvider::ReloadResourcesAndNotify() {
  LoadResources(
      base::BindOnce(&AdBlockCustomResourceProvider::NotifyResourcesLoaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace brave_shields

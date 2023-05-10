/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/component_updater/resource_component.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_ads/browser/component_updater/component_util.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_ads {

namespace {

constexpr int kCurrentSchemaVersion = 1;
constexpr char kSchemaVersionKey[] = "schemaVersion";
constexpr char kResourcesKey[] = "resources";
constexpr char kResourceIdKey[] = "id";
constexpr char kResourceFilenameKey[] = "filename";
constexpr char kResourceVersionKey[] = "version";

constexpr char kComponentName[] = "Brave Ads Resources (%s)";

constexpr base::FilePath::CharType kManifestFile[] =
    FILE_PATH_LITERAL("resources.json");

std::string GetResourceKey(const std::string& id, int version) {
  return id + base::NumberToString(version);
}

}  // namespace

ResourceComponent::ResourceComponent(Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate) {
  CHECK(delegate);
}

ResourceComponent::~ResourceComponent() = default;

void ResourceComponent::AddObserver(ResourceComponentObserver* observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void ResourceComponent::RemoveObserver(ResourceComponentObserver* observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

void ResourceComponent::RegisterComponentsForLocale(const std::string& locale) {
  RegisterComponentForCountryCode(brave_l10n::GetISOCountryCode(locale));
  RegisterComponentForLanguageCode(brave_l10n::GetISOLanguageCode(locale));
}

void ResourceComponent::NotifyDidUpdateResourceComponent(
    const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnDidUpdateResourceComponent(id);
  }
}

absl::optional<base::FilePath> ResourceComponent::GetPath(const std::string& id,
                                                          const int version) {
  const std::string index = GetResourceKey(id, version);
  const auto iter = resources_.find(index);
  if (iter == resources_.cend()) {
    return absl::nullopt;
  }

  const ResourceInfo resource = iter->second;
  return resource.path;
}

//////////////////////////////////////////////////////////////////////////////

void ResourceComponent::RegisterComponentForCountryCode(
    const std::string& country_code) {
  CHECK(!country_code.empty());

  const absl::optional<ComponentInfo> component =
      GetComponentInfo(country_code);
  if (!component) {
    VLOG(1) << "Ads resource not supported for " << country_code;
    return;
  }

  const std::string component_name =
      base::StringPrintf(kComponentName, country_code.c_str());

  VLOG(1) << "Registering " << component_name << " with id " << component->id;

  Register(component_name, component->id.data(), component->public_key.data());
}

void ResourceComponent::RegisterComponentForLanguageCode(
    const std::string& language_code) {
  CHECK(!language_code.empty());

  const absl::optional<ComponentInfo> component =
      GetComponentInfo(language_code);
  if (!component) {
    VLOG(1) << "Ads resource not supported for " << language_code;
    return;
  }

  const std::string component_name =
      base::StringPrintf(kComponentName, language_code.c_str());

  VLOG(1) << "Registering " << component_name << " with id " << component->id;

  Register(component_name, component->id.data(), component->public_key.data());
}

std::string GetManifest(const base::FilePath& path) {
  std::string json;

  const bool success = base::ReadFileToString(path, &json);
  if (!success || json.empty()) {
    VLOG(1) << "Failed to read resource manifest file: " << path;
    return json;
  }

  return json;
}

void ResourceComponent::OnComponentReady(const std::string& component_id,
                                         const base::FilePath& install_dir,
                                         const std::string& /*manifest*/) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&GetManifest, install_dir.Append(kManifestFile)),
      base::BindOnce(&ResourceComponent::OnGetManifest,
                     weak_factory_.GetWeakPtr(), component_id, install_dir));
}

void ResourceComponent::OnGetManifest(const std::string& component_id,
                                      const base::FilePath& install_dir,
                                      const std::string& json) {
  VLOG(8) << "Resource manifest JSON: " << json;

  const absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    VLOG(1) << "Failed to parse resource json";
    return;
  }
  const base::Value::Dict& dict = root->GetDict();

  const absl::optional<int> schema_version = dict.FindInt(kSchemaVersionKey);
  if (!schema_version) {
    VLOG(1) << "Resource schema version is missing";
    return;
  }

  if (*schema_version != kCurrentSchemaVersion) {
    VLOG(1) << "Resource schema version mismatch";
    return;
  }

  const auto* const resources_list = dict.FindList(kResourcesKey);
  if (!resources_list) {
    VLOG(1) << "Resource is missing";
    return;
  }

  for (const auto& item : *resources_list) {
    const auto* item_dict = item.GetIfDict();
    if (!item_dict) {
      VLOG(1) << "Failed to parse resource manifest";
      return;
    }

    const std::string* const resource_id =
        item_dict->FindString(kResourceIdKey);
    if (!resource_id) {
      VLOG(1) << "Resource id is missing";
      continue;
    }

    const absl::optional<int> version = item_dict->FindInt(kResourceVersionKey);
    if (!version) {
      VLOG(1) << *resource_id << " resource version is missing";
      continue;
    }

    const std::string* const filename =
        item_dict->FindString(kResourceFilenameKey);
    if (!filename) {
      VLOG(1) << *resource_id << " resource filename is missing";
      continue;
    }

    ResourceInfo resource;
    resource.id = *resource_id;
    resource.version = *version;
    resource.path = install_dir.AppendASCII(*filename);

    const std::string resource_key =
        GetResourceKey(resource.id, resource.version);
    auto iter = resources_.find(resource_key);
    if (iter != resources_.cend()) {
      VLOG(1) << "Updating " << resource.id << " resource to version "
              << resource.version;
      iter->second = resource;
    } else {
      VLOG(1) << "Adding " << resource.id << " resource version "
              << resource.version;
      resources_.insert({resource_key, resource});
    }
  }

  VLOG(1) << "Notifying resource component observers";
  NotifyDidUpdateResourceComponent(component_id);
}

}  // namespace brave_ads

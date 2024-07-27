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
#include "base/task/thread_pool.h"

namespace brave_ads {

namespace {

constexpr int kSchemaVersion = 1;
constexpr char kSchemaVersionKey[] = "schemaVersion";

constexpr char kManifestVersionKey[] = "version";

constexpr char kResourcesKey[] = "resources";
constexpr char kResourceIdKey[] = "id";
constexpr char kResourceFilenameKey[] = "filename";
constexpr char kResourceVersionKey[] = "version";

constexpr base::FilePath::CharType kManifestJsonFile[] =
    FILE_PATH_LITERAL("manifest.json");

constexpr base::FilePath::CharType kResourcesJsonFile[] =
    FILE_PATH_LITERAL("resources.json");

std::string GetResourceKey(const std::string& id, int version) {
  return id + base::NumberToString(version);
}

}  // namespace

ResourceComponent::ResourceComponent(
    brave_component_updater::BraveComponent::Delegate*
        component_updater_delegate)
    : country_resource_component_registrar_(component_updater_delegate, *this),
      language_resource_component_registrar_(component_updater_delegate,
                                             *this) {}

ResourceComponent::~ResourceComponent() = default;

void ResourceComponent::AddObserver(ResourceComponentObserver* observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void ResourceComponent::RemoveObserver(ResourceComponentObserver* observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

void ResourceComponent::RegisterComponentForCountryCode(
    const std::string& country_code) {
  country_resource_component_registrar_.RegisterResourceComponent(country_code);
}

void ResourceComponent::RegisterComponentForLanguageCode(
    const std::string& language_code) {
  language_resource_component_registrar_.RegisterResourceComponent(
      language_code);
}

std::optional<base::FilePath> ResourceComponent::MaybeGetPath(
    const std::string& id,
    const int version) {
  const std::string index = GetResourceKey(id, version);
  const auto iter = resources_.find(index);
  if (iter == resources_.cend()) {
    return std::nullopt;
  }

  const auto& [_, resource] = *iter;
  return resource.path;
}

///////////////////////////////////////////////////////////////////////////////

std::string LoadFile(const base::FilePath& path) {
  std::string json;

  const bool success = base::ReadFileToString(path, &json);
  if (!success || json.empty()) {
    VLOG(1) << "Failed to load file: " << path;
    return json;
  }

  return json;
}

void ResourceComponent::OnResourceComponentRegistered(
    const std::string& component_id,
    const base::FilePath& install_dir) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&LoadFile, install_dir.Append(kManifestJsonFile)),
      base::BindOnce(&ResourceComponent::LoadManifestCallback,
                     weak_factory_.GetWeakPtr(), component_id, install_dir));
}

void ResourceComponent::OnResourceComponentUnregistered(
    const std::string& component_id) {
  NotifyDidUnregisterResourceComponent(component_id);
}

void ResourceComponent::LoadManifestCallback(const std::string& component_id,
                                             const base::FilePath& install_dir,
                                             const std::string& json) {
  VLOG(8) << "Manifest JSON: " << json;

  const std::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    return VLOG(1) << "Failed to parse manifest";
  }
  const base::Value::Dict& dict = root->GetDict();

  const std::string* const manifest_version =
      dict.FindString(kManifestVersionKey);
  if (!manifest_version) {
    return VLOG(1) << "Manifest version is missing";
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&LoadFile, install_dir.Append(kResourcesJsonFile)),
      base::BindOnce(&ResourceComponent::LoadResourceCallback,
                     weak_factory_.GetWeakPtr(), *manifest_version,
                     component_id, install_dir));
}

void ResourceComponent::LoadResourceCallback(
    const std::string& manifest_version,
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& json) {
  VLOG(8) << "Resource JSON: " << json;

  const std::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    return VLOG(1) << "Failed to parse resource";
  }
  const base::Value::Dict& dict = root->GetDict();

  const std::optional<int> schema_version = dict.FindInt(kSchemaVersionKey);
  if (!schema_version) {
    return VLOG(1) << "Resource schema version is missing";
  }

  if (*schema_version != kSchemaVersion) {
    return VLOG(1) << "Resource schema version mismatch";
  }

  const auto* const resources_list = dict.FindList(kResourcesKey);
  if (!resources_list) {
    return VLOG(1) << "Resource is missing";
  }

  for (const auto& item : *resources_list) {
    const auto* item_dict = item.GetIfDict();
    if (!item_dict) {
      return VLOG(1) << "Failed to parse resource";
    }

    const std::string* const resource_id =
        item_dict->FindString(kResourceIdKey);
    if (!resource_id) {
      VLOG(1) << "Resource id is missing";
      continue;
    }

    const std::optional<int> version = item_dict->FindInt(kResourceVersionKey);
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
  NotifyResourceComponentDidChange(manifest_version, component_id);
}

void ResourceComponent::NotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnResourceComponentDidChange(manifest_version, id);
  }
}

void ResourceComponent::NotifyDidUnregisterResourceComponent(
    const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnDidUnregisterResourceComponent(id);
  }
}

}  // namespace brave_ads

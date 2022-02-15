/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/component_updater/resource_component.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_ads/browser/component_updater/component_util.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_ads {

namespace {

const uint16_t kCurrentSchemaVersion = 1;
const char kSchemaVersionPath[] = "schemaVersion";
const char kResourcePath[] = "resources";
const char kResourceIdPath[] = "id";
const char kResourceFilenamePath[] = "filename";
const char kResourceVersionPath[] = "version";

const char kComponentName[] = "Brave Ads Resources (%s)";

const base::FilePath::CharType kManifestFile[] =
    FILE_PATH_LITERAL("resources.json");

std::string GetIndex(std::string id, int version) {
  return id + std::to_string(version);
}

}  // namespace

ResourceComponent::ResourceComponent(Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate) {
  DCHECK(delegate);
}

ResourceComponent::~ResourceComponent() = default;

void ResourceComponent::RegisterComponentsForLocale(const std::string& locale) {
  const std::string country_code = brave_l10n::GetCountryCode(locale);
  RegisterComponentForCountryCode(country_code);

  const std::string language_code = brave_l10n::GetLanguageCode(locale);
  RegisterComponentForLanguageCode(language_code);
}

void ResourceComponent::AddObserver(Observer* observer) {
  DCHECK(observer);

  observers_.AddObserver(observer);
}

void ResourceComponent::RemoveObserver(Observer* observer) {
  DCHECK(observer);

  observers_.RemoveObserver(observer);
}

void ResourceComponent::NotifyObservers(const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnResourceComponentUpdated(id);
  }
}

absl::optional<base::FilePath> ResourceComponent::GetPath(const std::string& id,
                                                          const int version) {
  const std::string index = GetIndex(id, version);
  const auto iter = resources_.find(index);
  if (iter == resources_.end()) {
    return absl::nullopt;
  }

  const ResourceInfo resource = iter->second;
  return resource.path;
}

//////////////////////////////////////////////////////////////////////////////

void ResourceComponent::RegisterComponentForCountryCode(
    const std::string& country_code) {
  DCHECK(!country_code.empty());

  const absl::optional<ComponentInfo> component =
      GetComponentInfo(country_code);
  if (!component) {
    VLOG(1) << "Ads resource not supported for " << country_code;
    return;
  }

  const std::string component_name =
      base::StringPrintf(kComponentName, country_code.c_str());

  VLOG(1) << "Registering " << component_name << " with id " << component->id;

  Register(component_name, component->id, component->public_key);
}

void ResourceComponent::RegisterComponentForLanguageCode(
    const std::string& language_code) {
  DCHECK(!language_code.empty());

  const absl::optional<ComponentInfo> component =
      GetComponentInfo(language_code);
  if (!component) {
    VLOG(1) << "Ads resource not supported for " << language_code;
    return;
  }

  const std::string component_name =
      base::StringPrintf(kComponentName, language_code.c_str());

  VLOG(1) << "Registering " << component_name << " with id " << component->id;

  Register(component_name, component->id, component->public_key);
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
                                         const std::string& manifest) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&GetManifest, install_dir.Append(kManifestFile)),
      base::BindOnce(&ResourceComponent::OnGetManifest,
                     weak_factory_.GetWeakPtr(), component_id, install_dir));
}

void ResourceComponent::OnGetManifest(const std::string& component_id,
                                      const base::FilePath& install_dir,
                                      const std::string& json) {
  VLOG(8) << "resource manifest: " << json;

  absl::optional<base::Value> manifest = base::JSONReader::Read(json);
  if (!manifest) {
    VLOG(1) << "Failed to parse resource manifest";
    return;
  }

  const absl::optional<int> schemaVersion =
      manifest->FindIntPath(kSchemaVersionPath);
  if (!schemaVersion) {
    VLOG(1) << "Resource schema version is missing";
    return;
  }

  if (*schemaVersion != kCurrentSchemaVersion) {
    VLOG(1) << "Resource schema version mismatch";
    return;
  }

  const base::Value* resource_values = manifest->FindListPath(kResourcePath);
  if (!resource_values) {
    VLOG(1) << "No resources found";
    return;
  }

  for (const auto& resource_value : resource_values->GetListDeprecated()) {
    ResourceInfo resource;

    const std::string* id = resource_value.FindStringPath(kResourceIdPath);
    if (!id) {
      VLOG(1) << *id << " resource id is missing";
      continue;
    }
    resource.id = *id;

    const absl::optional<int> version =
        resource_value.FindIntPath(kResourceVersionPath);
    if (!version) {
      VLOG(1) << *id << " resource version is missing";
      continue;
    }
    resource.version = *version;

    const std::string* path =
        resource_value.FindStringPath(kResourceFilenamePath);
    if (!path) {
      VLOG(1) << *id << " resource path is missing";
      continue;
    }
    resource.path = install_dir.AppendASCII(*path);

    const std::string index = GetIndex(resource.id, resource.version);
    auto iter = resources_.find(index);
    if (iter != resources_.end()) {
      VLOG(1) << "Updating resource " << resource.id << " version "
              << resource.version;
      iter->second = resource;
    } else {
      VLOG(1) << "Adding resource " << resource.id << " version "
              << resource.version;
      resources_.insert({index, resource});
    }
  }

  VLOG(1) << "Notifying resource observers";
  NotifyObservers(component_id);
}

}  // namespace brave_ads

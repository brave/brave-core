/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_user_model/browser/user_model_file_service.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "brave/components/brave_user_model/browser/component_util.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_user_model {

namespace {

const uint16_t kCurrentSchemaVersion = 1;
const char kSchemaVersionPath[] = "schemaVersion";

const char kModelsPath[] = "models";
const char kModelIdPath[] = "id";
const char kModelFilenamePath[] = "filename";
const char kModelVersionPath[] = "version";

const char kComponentName[] = "Brave User Model Installer (%s)";

const base::FilePath::CharType kManifestFile[] =
    FILE_PATH_LITERAL("models.json");

}  // namespace

UserModelFileService::UserModelFileService(
    Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate) {
  DCHECK(delegate);
}

UserModelFileService::~UserModelFileService() = default;

void UserModelFileService::RegisterComponentsForLocale(
    const std::string& locale) {
  const std::string country_code = brave_l10n::GetCountryCode(locale);
  RegisterComponentForCountryCode(country_code);

  const std::string language_code = brave_l10n::GetLanguageCode(locale);
  RegisterComponentForLanguageCode(language_code);
}

void UserModelFileService::AddObserver(
    Observer* observer) {
  DCHECK(observer);

  observers_.AddObserver(observer);
}

void UserModelFileService::RemoveObserver(
    Observer* observer) {
  DCHECK(observer);

  observers_.RemoveObserver(observer);
}

void UserModelFileService::NotifyObservers(
    const std::string& id) {
  for (auto& observer : observers_) {
    observer.OnUserModelUpdated(id);
  }
}

base::Optional<base::FilePath> UserModelFileService::GetPathForId(
    const std::string& id) {
  const auto iter = user_models_.find(id);
  if (iter == user_models_.end()) {
    return base::nullopt;
  }

  const UserModelInfo user_model = iter->second;
  return user_model.path;
}

//////////////////////////////////////////////////////////////////////////////

void UserModelFileService::RegisterComponentForCountryCode(
    const std::string& country_code) {
  DCHECK(!country_code.empty());

  const base::Optional<ComponentInfo> component =
      GetComponentInfo(country_code);
  if (!component) {
    VLOG(1) << country_code << " not supported for user model installer";
    return;
  }

  const std::string component_name =
      base::StringPrintf(kComponentName, country_code.c_str());

  VLOG(1) << "Registering " << component_name << " with id " << component->id;

  Register(component_name, component->id, component->public_key);
}

void UserModelFileService::RegisterComponentForLanguageCode(
    const std::string& language_code) {
  DCHECK(!language_code.empty());

  const base::Optional<ComponentInfo> component =
      GetComponentInfo(language_code);
  if (!component) {
    VLOG(1) << language_code << " not supported for user model installer";
    return;
  }

  const std::string component_name =
      base::StringPrintf(kComponentName, language_code.c_str());

  VLOG(1) << "Registering " << component_name << " with id " << component->id;

  Register(component_name, component->id, component->public_key);
}

std::string GetManifest(
    const base::FilePath& path) {
  std::string json;

  const bool success = base::ReadFileToString(path, &json);
  if (!success || json.empty()) {
    VLOG(1) << "Failed to read user model manifest file: " << path;
    return json;
  }

  return json;
}

void UserModelFileService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  base::PostTaskAndReplyWithResult(FROM_HERE,
      { base::ThreadPool(), base::MayBlock() },
      base::BindOnce(&GetManifest, install_dir.Append(kManifestFile)),
      base::BindOnce(&UserModelFileService::OnGetManifest,
          weak_factory_.GetWeakPtr(), install_dir));
}

void UserModelFileService::OnGetManifest(
    const base::FilePath& install_dir,
    const std::string& json) {
  VLOG(8) << "User model manifest: " << json;

  base::Optional<base::Value> manifest = base::JSONReader::Read(json);
  if (!manifest) {
    VLOG(1) << "Failed to parse user model manifest";
    return;
  }

  const base::Optional<int> version = manifest->FindIntPath(kSchemaVersionPath);
  if (!version) {
    VLOG(1) << "User model schema version is missing";
    return;
  }

  if (*version != kCurrentSchemaVersion) {
    VLOG(1) << "User model schema version mismatch";
    return;
  }

  const base::Value* user_model_values = manifest->FindListPath(kModelsPath);
  if (!user_model_values) {
    VLOG(1) << "No user models found";
    return;
  }

  for (const auto& user_model_value : user_model_values->GetList()) {
    UserModelInfo user_model;

    const std::string* id = user_model_value.FindStringPath(kModelIdPath);
    if (!id) {
      VLOG(1) << *id << " user model id is missing";
      continue;
    }
    user_model.id = *id;

    const base::Optional<int> version =
        user_model_value.FindIntPath(kModelVersionPath);
    if (!version) {
      VLOG(1) << *id << " user model version is missing";
      continue;
    }
    user_model.version = *version;

    const std::string* path =
        user_model_value.FindStringPath(kModelFilenamePath);
    if (!path) {
      VLOG(1) << *id << " user model path is missing";
      continue;
    }
    user_model.path = install_dir.AppendASCII(*path);

    auto iter = user_models_.find(user_model.id);
    if (iter != user_models_.end()) {
      VLOG(1) << "Updating " << user_model.id << " user model";
      iter->second = user_model;
    } else {
      VLOG(1) << "Adding " << user_model.id << " user model";
      user_models_.insert({user_model.id, user_model});
    }

    VLOG(1) << "Notifying user model observers";
    NotifyObservers(user_model.id);
  }
}

}  // namespace brave_user_model

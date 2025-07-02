/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/local_data_files_service.h"

#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"

using brave_component_updater::BraveComponent;

namespace brave_component_updater {

LocalDataFilesService::LocalDataFilesService(BraveComponent::Delegate* delegate)
  : BraveComponent(delegate),
    initialized_(false) {}

LocalDataFilesService::~LocalDataFilesService() {
  for (auto& observer : observers_)
    observer.OnLocalDataFilesServiceDestroyed();
}

bool LocalDataFilesService::Start() {
  if (initialized_)
    return true;
  Register(kLocalDataFilesComponentName, kLocalDataFilesComponentId,
           kLocalDataFilesComponentBase64PublicKey);
  initialized_ = true;
  return true;
}

void LocalDataFilesService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  for (auto& observer : observers_)
    observer.OnComponentReady(component_id, install_dir, manifest);
}

void LocalDataFilesService::AddObserver(LocalDataFilesObserver* observer) {
  observers_.AddObserver(observer);
}

void LocalDataFilesService::RemoveObserver(LocalDataFilesObserver* observer) {
  observers_.RemoveObserver(observer);
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<LocalDataFilesService>
LocalDataFilesServiceFactory(BraveComponent::Delegate* delegate) {
  return std::make_unique<LocalDataFilesService>(delegate);
}

}  // namespace brave_component_updater

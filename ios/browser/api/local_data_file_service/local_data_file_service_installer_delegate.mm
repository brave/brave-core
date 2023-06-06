/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/local_data_file_service/local_data_file_service_installer_delegate.h"
#include "brave/ios/browser/api/local_data_file_service/local_data_file_service_installer_policy.h"

#include <sys/qos.h>
#include <string>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/task/thread_pool.h"

#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"

using brave_component_updater::BraveComponent;
using brave_component_updater::BraveOnDemandUpdater;
using ComponentObserver = update_client::UpdateClient::Observer;

namespace local_data_file_service {
LocalDataFileServiceDelegate::LocalDataFileServiceDelegate()
    : task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {}

LocalDataFileServiceDelegate::~LocalDataFileServiceDelegate() = default;

void LocalDataFileServiceDelegate::Register(
    const std::string& component_id,
    const std::string& component_name,
    const std::string& component_base64_public_key,
    base::OnceClosure registered_callback,
    BraveComponent::ReadyCallback ready_callback) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<LocalDataFilesComponentInstallerPolicy>(
          component_base64_public_key, component_id, component_name,
          ready_callback));

  installer->Register(GetApplicationContext()->GetComponentUpdateService(),
                      std::move(registered_callback));
}

bool LocalDataFileServiceDelegate::Unregister(const std::string& component_id) {
  return GetApplicationContext()
      ->GetComponentUpdateService()
      ->UnregisterComponent(component_id);
}

void LocalDataFileServiceDelegate::OnDemandUpdate(
    const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(component_id);
}

void LocalDataFileServiceDelegate::AddObserver(ComponentObserver* observer) {
  GetApplicationContext()->GetComponentUpdateService()->AddObserver(observer);
}

void LocalDataFileServiceDelegate::RemoveObserver(ComponentObserver* observer) {
  GetApplicationContext()->GetComponentUpdateService()->RemoveObserver(
      observer);
}

scoped_refptr<base::SequencedTaskRunner>
LocalDataFileServiceDelegate::GetTaskRunner() {
  return task_runner_;
}

const std::string LocalDataFileServiceDelegate::locale() const {
  return GetApplicationContext()->GetApplicationLocale();
}

PrefService* LocalDataFileServiceDelegate::local_state() {
  return GetApplicationContext()->GetLocalState();
}
}  // namespace local_data_file_service

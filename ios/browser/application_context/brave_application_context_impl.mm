// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/application_context/brave_application_context_impl.h"

#include <string>

#import "base/command_line.h"
#import "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/brave_component_updater_delegate.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "brave/ios/browser/brave_wallet/wallet_data_files_installer_delegate_impl.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"

BraveApplicationContextImpl::BraveApplicationContextImpl(
    base::SequencedTaskRunner* local_state_task_runner,
    const base::CommandLine& command_line,
    const std::string& locale,
    const std::string& country)
    : ApplicationContextImpl(local_state_task_runner,
                             command_line,
                             locale,
                             country) {}

inline BraveApplicationContextImpl::~BraveApplicationContextImpl() = default;

// MARK: - ApplicationContextImpl

ukm::UkmRecorder* BraveApplicationContextImpl::GetUkmRecorder() {
  return nullptr;
}

gcm::GCMDriver* BraveApplicationContextImpl::GetGCMDriver() {
  return nullptr;
}

// MARK: - BraveApplicationContextImpl

brave_component_updater::BraveComponent::Delegate*
BraveApplicationContextImpl::brave_component_updater_delegate() {
  if (!brave_component_updater_delegate_) {
    brave_component_updater_delegate_ =
        std::make_unique<brave::BraveComponentUpdaterDelegate>(
            GetComponentUpdateService(), GetLocalState(),
            GetApplicationLocale());
  }

  return brave_component_updater_delegate_.get();
}

brave_component_updater::LocalDataFilesService*
BraveApplicationContextImpl::local_data_files_service() {
  if (!local_data_files_service_) {
    local_data_files_service_ =
        brave_component_updater::LocalDataFilesServiceFactory(
            brave_component_updater_delegate());
  }
  return local_data_files_service_.get();
}

brave::URLSanitizerComponentInstaller*
BraveApplicationContextImpl::url_sanitizer_component_installer() {
  if (!url_sanitizer_component_installer_) {
    url_sanitizer_component_installer_ =
        std::make_unique<brave::URLSanitizerComponentInstaller>(
            local_data_files_service());
  }
  return url_sanitizer_component_installer_.get();
}

void BraveApplicationContextImpl::StartBraveServices() {
  // We need to Initialize the component installer
  // before calling Start on the local_data_files_service
  url_sanitizer_component_installer();

  // Start the local data file service
  local_data_files_service()->Start();

  brave_wallet::WalletDataFilesInstaller::GetInstance().SetDelegate(
      std::make_unique<brave_wallet::WalletDataFilesInstallerDelegateImpl>());
}

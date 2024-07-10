// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_BRAVE_APPLICATION_CONTEXT_IMPL_H_
#define BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_BRAVE_APPLICATION_CONTEXT_IMPL_H_

#include <memory>
#include <string>

#include "brave/components/ai_chat/core/browser/leo_local_models_updater.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/debounce/core/browser/debounce_component_installer.h"
#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "ios/chrome/browser/application_context/model/application_context_impl.h"

namespace base {
class CommandLine;
class SequencedTaskRunner;
}  // namespace base

/// This extends the behaviors of the ApplicationContext
class BraveApplicationContextImpl : public ApplicationContextImpl {
 public:
  // Out-of-line constructor declaration
  BraveApplicationContextImpl(
      base::SequencedTaskRunner* local_state_task_runner,
      const base::CommandLine& command_line,
      const std::string& locale,
      const std::string& country);

  BraveApplicationContextImpl(const BraveApplicationContextImpl&) = delete;
  BraveApplicationContextImpl& operator=(const BraveApplicationContextImpl&) =
      delete;

  ~BraveApplicationContextImpl() override;

  // ApplicationContextImpl
  ukm::UkmRecorder* GetUkmRecorder() override;
  gcm::GCMDriver* GetGCMDriver() override;

  // BraveApplicationContextImpl
  brave::URLSanitizerComponentInstaller* url_sanitizer_component_installer();
  debounce::DebounceComponentInstaller* debounce_component_installer();
  https_upgrade_exceptions::HttpsUpgradeExceptionsService*
  https_upgrade_exceptions_service();
  ai_chat::LeoLocalModelsUpdater* leo_local_models_updater();

  // Start any services that we may need later
  void StartBraveServices();

 private:
  brave_component_updater::BraveComponent::Delegate*
  brave_component_updater_delegate();
  brave_component_updater::LocalDataFilesService* local_data_files_service();

  std::unique_ptr<brave_component_updater::BraveComponent::Delegate>
      brave_component_updater_delegate_;
  std::unique_ptr<brave_component_updater::LocalDataFilesService>
      local_data_files_service_;
  std::unique_ptr<ai_chat::LeoLocalModelsUpdater> leo_local_models_updater_;
  std::unique_ptr<brave::URLSanitizerComponentInstaller>
      url_sanitizer_component_installer_;
  std::unique_ptr<debounce::DebounceComponentInstaller>
      debounce_component_installer_;
  std::unique_ptr<https_upgrade_exceptions::HttpsUpgradeExceptionsService>
      https_upgrade_exceptions_service_;
};

#endif  // BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_BRAVE_APPLICATION_CONTEXT_IMPL_H_

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_BRAVE_APPLICATION_CONTEXT_IMPL_H_
#define BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_BRAVE_APPLICATION_CONTEXT_IMPL_H_

#include <memory>
#include <string>
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "ios/chrome/browser/application_context/application_context_impl.h"

namespace base {
class CommandLine;
class SequencedTaskRunner;
}  // namespace base

/// This extends the behaviors or the ApplicationContext
class BraveApplicationContextImpl : public ApplicationContextImpl {
 public:
  // Out-of-line constructor declaration
  BraveApplicationContextImpl(
      base::SequencedTaskRunner* local_state_task_runner,
      const base::CommandLine& command_line,
      const std::string& locale,
      const std::string& country);

  brave::URLSanitizerComponentInstaller* url_sanitizer_component_installer();

  // Disable copy constructor and assignment operator
  BraveApplicationContextImpl(const BraveApplicationContextImpl&) = delete;
  BraveApplicationContextImpl& operator=(const BraveApplicationContextImpl&) =
      delete;

  // Out-of-line destructor declaration
  ~BraveApplicationContextImpl() override;

 private:
  brave_component_updater::BraveComponent::Delegate*
  brave_component_updater_delegate();
  brave_component_updater::LocalDataFilesService* local_data_files_service();

  std::unique_ptr<brave_component_updater::BraveComponent::Delegate>
      brave_component_updater_delegate_;
  std::unique_ptr<brave_component_updater::LocalDataFilesService>
      local_data_files_service_;
  std::unique_ptr<brave::URLSanitizerComponentInstaller>
      url_sanitizer_component_installer_;
};

#endif  // BRAVE_IOS_BROWSER_APPLICATION_CONTEXT_BRAVE_APPLICATION_CONTEXT_IMPL_H_

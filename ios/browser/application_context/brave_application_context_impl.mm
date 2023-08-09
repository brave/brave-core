/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "brave/ios/browser/local_data_file_service/local_data_file_service_installer_delegate.h"

#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"

#import "base/command_line.h"
#import "base/task/sequenced_task_runner.h"

BraveApplicationContextImpl::BraveApplicationContextImpl(
    base::SequencedTaskRunner* local_state_task_runner,
    const base::CommandLine& command_line,
    const std::string& locale,
    const std::string& country)
    : ApplicationContextImpl(local_state_task_runner,
                             command_line,
                             locale,
                             country) {}

BraveApplicationContextImpl::~BraveApplicationContextImpl() {
  DCHECK_EQ(this, GetApplicationContext());
  SetApplicationContext(nullptr);
}

brave_component_updater::BraveComponent::Delegate*
BraveApplicationContextImpl::brave_component_updater_delegate() {
  if (!brave_component_updater_delegate_) {
    brave_component_updater_delegate_ = std::make_unique<
        local_data_file_service::LocalDataFileServiceDelegate>();
  }

  return brave_component_updater_delegate_.get();
}

brave_component_updater::LocalDataFilesService*
BraveApplicationContextImpl::local_data_files_service() {
  if (!local_data_files_service_) {
    local_data_files_service_ =
        brave_component_updater::LocalDataFilesServiceFactory(
            brave_component_updater_delegate());
    local_data_files_service_.get()->Start();
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

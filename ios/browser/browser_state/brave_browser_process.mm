/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/browser_state/brave_browser_process.h"

#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"

#include "brave/ios/browser/local_data_file_service/local_data_file_service_installer_delegate.h"

BraveBrowserProcess& BraveBrowserProcess::GetInstance() {
  static BraveBrowserProcess instance;  // Guaranteed to be initialized once.
  return instance;
}

BraveBrowserProcess::BraveBrowserProcess() {}

// The delegate used by the iOS
brave_component_updater::BraveComponent::Delegate*
BraveBrowserProcess::brave_component_updater_delegate() {
  if (!brave_component_updater_delegate_) {
    brave_component_updater_delegate_ = std::make_unique<
        local_data_file_service::LocalDataFileServiceDelegate>();
  }

  return brave_component_updater_delegate_.get();
}

brave_component_updater::LocalDataFilesService*
BraveBrowserProcess::local_data_files_service() {
  if (!local_data_files_service_) {
    local_data_files_service_ =
        brave_component_updater::LocalDataFilesServiceFactory(
            brave_component_updater_delegate());
    local_data_files_service_.get()->Start();
  }
  return local_data_files_service_.get();
}

brave::URLSanitizerComponentInstaller*
BraveBrowserProcess::url_sanitizer_component_installer() {
  if (!url_sanitizer_component_installer_) {
    url_sanitizer_component_installer_ =
        std::make_unique<brave::URLSanitizerComponentInstaller>(
            local_data_files_service());
  }
  return url_sanitizer_component_installer_.get();
}

BraveBrowserProcess::~BraveBrowserProcess() {
  // Put any necessary cleanup code here (if needed)
}

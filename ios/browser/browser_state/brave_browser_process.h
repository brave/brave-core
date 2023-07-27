/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BROWSER_STATE_BRAVE_BROWSER_PROCESS_H_
#define BRAVE_IOS_BROWSER_BROWSER_STATE_BRAVE_BROWSER_PROCESS_H_

#include <memory>

#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"

namespace brave_component_updater {
class LocalDataFilesService;
}  // namespace brave_component_updater

class BraveBrowserProcess {
 public:
  static BraveBrowserProcess& GetInstance();

  brave::URLSanitizerComponentInstaller* url_sanitizer_component_installer();

  // Disable copy constructor and assignment operator
  BraveBrowserProcess(const BraveBrowserProcess&) = delete;
  BraveBrowserProcess& operator=(const BraveBrowserProcess&) = delete;

  // Out-of-line destructor declaration
  ~BraveBrowserProcess();

 private:
  // Private constructor to prevent instantiation from outside
  BraveBrowserProcess();

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

#endif  // BRAVE_IOS_BROWSER_BROWSER_STATE_BRAVE_BROWSER_PROCESS_H_

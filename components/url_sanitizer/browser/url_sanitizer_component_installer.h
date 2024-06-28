/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_URL_SANITIZER_BROWSER_URL_SANITIZER_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_URL_SANITIZER_BROWSER_URL_SANITIZER_COMPONENT_INSTALLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/json/json_value_converter.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"

namespace brave {

class URLSanitizerComponentInstaller
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit URLSanitizerComponentInstaller(
      brave_component_updater::LocalDataFilesService* local_data_files_service);
  URLSanitizerComponentInstaller(const URLSanitizerComponentInstaller&) =
      delete;
  URLSanitizerComponentInstaller& operator=(
      const URLSanitizerComponentInstaller&) = delete;
  ~URLSanitizerComponentInstaller() override;

  struct RawConfig {
    RawConfig();
    RawConfig(const RawConfig&);
    RawConfig(RawConfig&&);
    ~RawConfig();

    std::string matchers;
    std::string permissions;
  };

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnConfigReady(const RawConfig& config) = 0;
  };

  // Implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  void OnRawConfigReady(const RawConfig& config);
  void LoadDirectlyFromResourcePath();

  base::ObserverList<Observer> observers_;
  base::FilePath resource_dir_;

  base::WeakPtrFactory<URLSanitizerComponentInstaller> weak_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_URL_SANITIZER_BROWSER_URL_SANITIZER_COMPONENT_INSTALLER_H_

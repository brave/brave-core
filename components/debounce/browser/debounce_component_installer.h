/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_COMPONENT_INSTALLER_H_

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
#include "brave/components/debounce/browser/debounce_rule.h"
#include "brave/components/debounce/browser/debounce_service.h"

namespace debounce {

class DebounceBrowserTest;

extern const char kDebounceConfigFile[];
extern const char kDebounceConfigFileVersion[];

// The debounce download service is in charge
// of loading and parsing the debounce configuration file
class DebounceComponentInstaller
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit DebounceComponentInstaller(
      brave_component_updater::LocalDataFilesService* local_data_files_service);
  DebounceComponentInstaller(const DebounceComponentInstaller&) = delete;
  DebounceComponentInstaller& operator=(const DebounceComponentInstaller&) =
      delete;
  ~DebounceComponentInstaller() override;

  const std::vector<std::unique_ptr<DebounceRule>>& rules() const {
    return rules_;
  }
  const base::flat_set<std::string>& host_cache() const { return host_cache_; }

  // implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  // implementation of our own observers
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnRulesReady(
        DebounceComponentInstaller* component_installer) = 0;
  };
  void AddObserver(Observer* observer) { observers_.AddObserver(observer); }
  void RemoveObserver(Observer* observer) {
    observers_.RemoveObserver(observer);
  }

 private:
  friend class DebounceBrowserTest;

  void OnDATFileDataReady(const std::string& contents);
  void LoadOnTaskRunner();
  void LoadDirectlyFromResourcePath();

  base::ObserverList<Observer> observers_;
  std::vector<std::unique_ptr<DebounceRule>> rules_;
  base::flat_set<std::string> host_cache_;
  base::FilePath resource_dir_;

  base::WeakPtrFactory<DebounceComponentInstaller> weak_factory_{this};
};

}  // namespace debounce

#endif  // BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_COMPONENT_INSTALLER_H_

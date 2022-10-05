/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_COMPONENT_INSTALLER_H_

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
#include "brave/components/request_otr/browser/request_otr_rule.h"
#include "brave/components/request_otr/browser/request_otr_service.h"

namespace request_otr {

class RequestOTRBrowserTest;

extern const char kRequestOTRConfigFile[];
extern const char kRequestOTRConfigFileVersion[];

// The request_otr download service is in charge
// of loading and parsing the request_otr configuration file
class RequestOTRComponentInstaller
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit RequestOTRComponentInstaller(
      brave_component_updater::LocalDataFilesService* local_data_files_service);
  RequestOTRComponentInstaller(const RequestOTRComponentInstaller&) = delete;
  RequestOTRComponentInstaller& operator=(const RequestOTRComponentInstaller&) =
      delete;
  ~RequestOTRComponentInstaller() override;

  const std::vector<std::unique_ptr<RequestOTRRule>>& rules() const {
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
        RequestOTRComponentInstaller* component_installer) = 0;
  };
  void AddObserver(Observer* observer) { observers_.AddObserver(observer); }
  void RemoveObserver(Observer* observer) {
    observers_.RemoveObserver(observer);
  }

 private:
  friend class RequestOTRBrowserTest;

  void OnDATFileDataReady(const std::string& contents);
  void LoadOnTaskRunner();
  void LoadDirectlyFromResourcePath();

  base::ObserverList<Observer> observers_;
  std::vector<std::unique_ptr<RequestOTRRule>> rules_;
  base::flat_set<std::string> host_cache_;
  base::FilePath resource_dir_;

  base::WeakPtrFactory<RequestOTRComponentInstaller> weak_factory_{this};
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_COMPONENT_INSTALLER_H_

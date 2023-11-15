/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_COMPONENT_INSTALLER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"

namespace request_otr {

inline constexpr char kRequestOTRConfigFile[] = "request-otr.json";
inline constexpr char kRequestOTRConfigFileVersion[] = "1";

// The request_otr download service is in charge
// of loading and parsing the request_otr configuration file
class RequestOTRComponentInstallerPolicy
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  explicit RequestOTRComponentInstallerPolicy(
      brave_component_updater::LocalDataFilesService* local_data_files_service);
  RequestOTRComponentInstallerPolicy(
      const RequestOTRComponentInstallerPolicy&) = delete;
  RequestOTRComponentInstallerPolicy& operator=(
      const RequestOTRComponentInstallerPolicy&) = delete;
  ~RequestOTRComponentInstallerPolicy() override;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnRulesReady(const std::string& json_content) = 0;
  };

  // implementation of brave_component_updater::LocalDataFilesObserver
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  void OnDATFileDataReady(const std::string& contents);
  void LoadOnTaskRunner();
  void LoadDirectlyFromResourcePath();

  base::ObserverList<Observer> observers_;
  base::FilePath resource_dir_;

  base::WeakPtrFactory<RequestOTRComponentInstallerPolicy> weak_factory_{this};
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_COMPONENT_INSTALLER_H_

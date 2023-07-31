/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_INSTALLER_DELEGATE_H_
#define BRAVE_IOS_BROWSER_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_INSTALLER_DELEGATE_H_

#include <sys/qos.h>
#include <string>

#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"

using brave_component_updater::BraveComponent;
using ComponentObserver = update_client::UpdateClient::Observer;

namespace base {
class SequencedTaskRunner;
}

class PrefService;

namespace local_data_file_service {
/// A delegate class that is used for the local data file service
class LocalDataFileServiceDelegate final : public BraveComponent::Delegate {
 public:
  LocalDataFileServiceDelegate();
  LocalDataFileServiceDelegate(const LocalDataFileServiceDelegate&) = delete;
  LocalDataFileServiceDelegate& operator=(const LocalDataFileServiceDelegate&) =
      delete;
  ~LocalDataFileServiceDelegate() override;

  // brave_component_updater::BraveComponent::Delegate implementation
  void Register(const std::string& component_name,
                const std::string& component_base64_public_key,
                base::OnceClosure registered_callback,
                BraveComponent::ReadyCallback ready_callback) override;
  bool Unregister(const std::string& component_id) override;
  void OnDemandUpdate(const std::string& component_id) override;

  void AddObserver(ComponentObserver* observer) override;
  void RemoveObserver(ComponentObserver* observer) override;

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() override;

  const std::string locale() const override;
  PrefService* local_state() override;

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};
}  // namespace local_data_file_service

#endif  // BRAVE_IOS_BROWSER_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_INSTALLER_DELEGATE_H_

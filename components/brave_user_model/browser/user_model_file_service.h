/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_USER_MODEL_BROWSER_USER_MODEL_FILE_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_USER_MODEL_BROWSER_USER_MODEL_FILE_SERVICE_H_

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_user_model/browser/user_model_info.h"
#include "brave/components/brave_user_model/browser/user_model_observer.h"

using brave_component_updater::BraveComponent;

namespace brave_user_model {

class UserModelFileService : public BraveComponent {
 public:
  explicit UserModelFileService(
      Delegate* delegate);
  ~UserModelFileService() override;

  UserModelFileService(const UserModelFileService&) = delete;
  UserModelFileService& operator=(const UserModelFileService&) = delete;

  void RegisterComponentsForLocale(
      const std::string& locale);

  void AddObserver(
      Observer* observer);
  void RemoveObserver(
      Observer* observer);
  void NotifyObservers(
      const std::string& id);

  base::Optional<base::FilePath> GetPathForId(
      const std::string& id);

 private:
  void RegisterComponentForCountryCode(
      const std::string& country_code);

  void RegisterComponentForLanguageCode(
      const std::string& language_code);

  void OnComponentReady(
      const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;

  void OnGetManifest(
      const base::FilePath& install_dir,
      const std::string& json);

  std::map<std::string, UserModelInfo> user_models_;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<UserModelFileService> weak_factory_{this};
};

}  // namespace brave_user_model

#endif  // BRAVE_COMPONENTS_BRAVE_USER_MODEL_BROWSER_USER_MODEL_FILE_SERVICE_H_

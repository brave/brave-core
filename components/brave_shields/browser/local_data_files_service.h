/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_LOCAL_DATA_FILES_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_LOCAL_DATA_FILES_SERVICE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

class AdBlockServiceTest;
class AutoplayWhitelistServiceTest;
class ReferrerWhitelistServiceTest;
class TrackingProtectionServiceTest;

using brave_component_updater::BraveComponent;

namespace brave_shields {

class BaseLocalDataFilesObserver;

const char kLocalDataFilesComponentName[] = "Brave Local Data Updater";
const char kLocalDataFilesComponentId[] = "afalakplffnnnlkncjhbmahjfjhmlkal";
const char kLocalDataFilesComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAs4TIQXRCftLpGmQZxmm6"
    "AU8pqGKLoDyi537HGQyRKcK7j/CSXCf3vwJr7xkV72p7bayutuzyNZ3740QxBPie"
    "sfBOp8bBb8d2VgTHP3b+SuNmK/rsSRsMRhT05x8AAr/7ab6U3rW0Gsalm2653xnn"
    "QS8vt0s62xQTmC+UMXowaSLUZ0Be/TOu6lHZhOeo0NBMKc6PkOu0R1EEfP7dJR6S"
    "M/v4dBUBZ1HXcuziVbCXVyU51opZCMjlxyUlQR9pTGk+Zh5sDn1Vw1MwLnWiEfQ4"
    "EGL1V7GeI4vgLoOLgq7tmhEratHGCfC1IHm9luMACRr/ybMI6DQJOvgBvecb292F"
    "xQIDAQAB";

// The component in charge of delegating access to different DAT files
// such as tracking protection and video autoplay whitelist
class LocalDataFilesService : public BraveComponent {
 public:
  LocalDataFilesService(BraveComponent::Delegate* delegate);
  ~LocalDataFilesService() override;
  bool Start();
  bool IsInitialized() const { return initialized_; }
  void AddObserver(BaseLocalDataFilesObserver* observer);

 protected:
  void OnComponentReady(const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;

 private:
  friend class ::AdBlockServiceTest;
  friend class ::AutoplayWhitelistServiceTest;
  friend class ::ReferrerWhitelistServiceTest;
  friend class ::TrackingProtectionServiceTest;
  static std::string g_local_data_files_component_id_;
  static std::string g_local_data_files_component_base64_public_key_;
  static void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key);

  std::vector<BaseLocalDataFilesObserver*> observers_;

  SEQUENCE_CHECKER(sequence_checker_);
  bool initialized_;
  bool observers_already_called_;
  base::WeakPtrFactory<LocalDataFilesService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(LocalDataFilesService);
};

// Creates the LocalDataFilesService
std::unique_ptr<LocalDataFilesService>
LocalDataFilesServiceFactory(BraveComponent::Delegate* delegate);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_LOCAL_DATA_FILES_SERVICE_H_

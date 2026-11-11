/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_LOCAL_DATA_FILES_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_LOCAL_DATA_FILES_SERVICE_H_

#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "crypto/sha2.h"

namespace brave_component_updater {

class LocalDataFilesObserver;

inline constexpr char kLocalDataFilesComponentName[] =
    "Brave Local Data Updater";
inline constexpr uint8_t kLocalDataFilesComponentPublicKeySHA256[32] = {
    0x05, 0x0b, 0x0a, 0xfb, 0x55, 0xdd, 0xdb, 0xad, 0x29, 0x71, 0xc0,
    0x79, 0x59, 0x7c, 0xba, 0x0b, 0xd5, 0xac, 0x2b, 0x73, 0xca, 0xfc,
    0xb5, 0x14, 0xfa, 0xb3, 0x3d, 0x16, 0x60, 0xa7, 0x9a, 0xaa};
static_assert(std::size(kLocalDataFilesComponentPublicKeySHA256) ==
              crypto::kSHA256Length);

// The component in charge of delegating access to different DAT files
// such as tracking protection.
class COMPONENT_EXPORT(BRAVE_COMPONENT_UPDATER) LocalDataFilesService
    : public BraveComponent {
 public:
  explicit LocalDataFilesService(BraveComponent::Delegate* delegate);
  LocalDataFilesService(const LocalDataFilesService&) = delete;
  LocalDataFilesService& operator=(const LocalDataFilesService&) = delete;
  ~LocalDataFilesService() override;
  bool Start();
  bool IsInitialized() const { return initialized_; }
  void AddObserver(LocalDataFilesObserver* observer);
  void RemoveObserver(LocalDataFilesObserver* observer);

 protected:
  void OnComponentReady(const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;

 private:
  bool initialized_;
  base::ObserverList<LocalDataFilesObserver>::Unchecked observers_;
};

// Creates the LocalDataFilesService
COMPONENT_EXPORT(BRAVE_COMPONENT_UPDATER)
std::unique_ptr<LocalDataFilesService>
LocalDataFilesServiceFactory(BraveComponent::Delegate* delegate);

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_LOCAL_DATA_FILES_SERVICE_H_

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_MENU_COORDINATOR_MAC_H_
#define BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_MENU_COORDINATOR_MAC_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/commands/accelerator_service.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "chrome/browser/ui/browser_window/public/browser_collection_observer.h"

class BrowserCollection;
class BrowserWindowInterface;
class Profile;

namespace commands {

// Keeps the main menu ([NSApp mainMenu]) key equivalents in sync with the
// last-active profile's customized shortcuts, so that removing or changing a
// menu-backed default shortcut in brave://settings/system/shortcuts takes
// effect (menu-backed shortcuts are dispatched by the OS menu, not by the
// browser's FocusManager - see AcceleratorService).
//
// A menu item shows its default key equivalent while the default accelerator
// remains assigned to its command, and no key equivalent otherwise. Custom
// accelerators are never written into the menu: they are dispatched by the
// FocusManager, and displaying them on a menu item would double-dispatch.
class AcceleratorMenuCoordinatorMac : public BrowserCollectionObserver,
                                      public AcceleratorService::Observer,
                                      public ProfileObserver {
 public:
  AcceleratorMenuCoordinatorMac();
  AcceleratorMenuCoordinatorMac(const AcceleratorMenuCoordinatorMac&) = delete;
  AcceleratorMenuCoordinatorMac& operator=(
      const AcceleratorMenuCoordinatorMac&) = delete;
  ~AcceleratorMenuCoordinatorMac() override;

  // BrowserCollectionObserver:
  void OnBrowserActivated(BrowserWindowInterface* browser) override;

  // AcceleratorService::Observer:
  void OnAcceleratorsChanged(
      const AcceleratorPrefManager::Accelerators& changed) override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

 private:
  // Holds the Objective-C state (menu items and their pristine key
  // equivalents), so this header stays includable from C++.
  struct ObjCStorage;

  // Updates the menu items mapped to |command_id| to reflect the current
  // accelerators in |service_|.
  void SyncCommand(int command_id);

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<AcceleratorService> service_ = nullptr;
  base::ScopedObservation<Profile, ProfileObserver> profile_observation_{this};
  base::ScopedObservation<AcceleratorService, AcceleratorService::Observer>
      service_observation_{this};
  base::ScopedObservation<BrowserCollection, BrowserCollectionObserver>
      browser_collection_observation_{this};
  std::unique_ptr<ObjCStorage> objc_storage_;
};

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_MENU_COORDINATOR_MAC_H_

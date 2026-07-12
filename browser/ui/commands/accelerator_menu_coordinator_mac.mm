// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commands/accelerator_menu_coordinator_mac.h"

#import <Cocoa/Cocoa.h>

#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "brave/browser/ui/views/commands/default_accelerators_mac.h"
#include "brave/components/commands/common/accelerator_parsing.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"

namespace commands {

namespace {

// Commands whose menu item key equivalents are managed elsewhere and must not
// be touched:
// - Unmodifiable commands (IDC_CLOSE_TAB / IDC_CLOSE_WINDOW): hard-coded and
//   dynamically swapped by upstream's app_controller_mac.mm.
// - IDC_CONTENT_CONTEXT_COPY / IDC_COPY_CLEAN_LINK: dynamically swapped by
//   BraveAppController when the Edit menu opens (see
//   brave_app_controller_mac.mm).
bool IsKeyEquivalentManagedElsewhere(int command_id) {
  return IsUnmodifiableCommand(command_id) ||
         command_id == IDC_CONTENT_CONTEXT_COPY ||
         command_id == IDC_COPY_CLEAN_LINK;
}

}  // namespace

struct AcceleratorMenuCoordinatorMac::ObjCStorage {
  struct MenuItemState {
    NSMenuItem* item = nil;
    // The item's default key equivalent, captured before any mutation.
    NSString* key_equivalent = nil;
    NSUInteger modifier_mask = 0;
    BOOL allows_localization = NO;
    BOOL allows_mirroring = NO;
    // Codes string (see accelerator_parsing.h) of the default accelerator the
    // key equivalent maps to.
    std::string codes;
  };

  // Built once, before any mutation, so the captured state is pristine. The
  // main menu's command items are static after startup.
  bool indexed = false;
  base::flat_map<int, std::vector<MenuItemState>> items_by_command;

  void EnsureIndexed() {
    if (indexed) {
      return;
    }
    indexed = true;

    NSMenu* menu = [NSApp mainMenu];
    if (!menu) {
      return;
    }
    ForEachMenuItem(menu, [this](NSMenuItem* item) {
      const int command_id = static_cast<int>(item.tag);
      if (IsKeyEquivalentManagedElsewhere(command_id)) {
        return;
      }
      auto accelerator = GetAcceleratorFromMenuItem(item);
      if (!accelerator) {
        return;
      }
      MenuItemState state;
      state.item = item;
      state.key_equivalent = [item.keyEquivalent copy];
      state.modifier_mask = item.keyEquivalentModifierMask;
      if (@available(macos 12.0, *)) {
        state.allows_localization =
            item.allowsAutomaticKeyEquivalentLocalization;
        state.allows_mirroring = item.allowsAutomaticKeyEquivalentMirroring;
      }
      state.codes = ToCodesString(*accelerator);
      items_by_command[command_id].push_back(std::move(state));
    });
  }

  // Restores every indexed item to its pristine key equivalent.
  void RestoreAll() {
    for (auto& [command_id, states] : items_by_command) {
      for (auto& state : states) {
        RestorePristine(state);
      }
    }
  }

  static void RestorePristine(const MenuItemState& state) {
    NSMenuItem* item = state.item;
    item.keyEquivalent = state.key_equivalent;
    item.keyEquivalentModifierMask = state.modifier_mask;
    if (@available(macos 12.0, *)) {
      item.allowsAutomaticKeyEquivalentLocalization = state.allows_localization;
      item.allowsAutomaticKeyEquivalentMirroring = state.allows_mirroring;
    }
  }
};

AcceleratorMenuCoordinatorMac::AcceleratorMenuCoordinatorMac()
    : objc_storage_(std::make_unique<ObjCStorage>()) {
  browser_collection_observation_.Observe(
      GlobalBrowserCollection::GetInstance());
}

AcceleratorMenuCoordinatorMac::~AcceleratorMenuCoordinatorMac() = default;

void AcceleratorMenuCoordinatorMac::OnBrowserActivated(
    BrowserWindowInterface* browser) {
  Profile* profile = browser->GetProfile()->GetOriginalProfile();
  if (profile == profile_) {
    return;
  }

  profile_observation_.Reset();
  service_observation_.Reset();
  profile_ = profile;
  profile_observation_.Observe(profile);

  // The service can be null for profiles the factory doesn't cover (e.g.
  // Guest). Those windows fall back to the default shortcuts (see
  // BraveBrowserView::LoadAccelerators), so restore the pristine menu.
  service_ = AcceleratorServiceFactory::GetForContext(profile);
  if (!service_) {
    objc_storage_->RestoreAll();
    return;
  }

  // Adding the observer replays the current state of every command, which
  // drives a full resync of the menu to this profile's customizations.
  service_observation_.Observe(service_);
}

void AcceleratorMenuCoordinatorMac::OnAcceleratorsChanged(
    const AcceleratorPrefManager::Accelerators& changed) {
  for (const auto& [command_id, accelerators] : changed) {
    SyncCommand(command_id);
  }
}

void AcceleratorMenuCoordinatorMac::OnProfileWillBeDestroyed(Profile* profile) {
  CHECK(profile == profile_);
  profile_observation_.Reset();
  service_observation_.Reset();
  service_ = nullptr;
  profile_ = nullptr;
}

void AcceleratorMenuCoordinatorMac::SyncCommand(int command_id) {
  if (IsKeyEquivalentManagedElsewhere(command_id)) {
    return;
  }

  objc_storage_->EnsureIndexed();
  auto it = objc_storage_->items_by_command.find(command_id);
  if (it == objc_storage_->items_by_command.end()) {
    return;
  }

  CHECK(service_);
  base::flat_set<std::string> current_codes;
  for (const auto& accelerator :
       service_->GetAcceleratorsForCommand(command_id)) {
    current_codes.insert(ToCodesString(accelerator));
  }

  for (auto& state : it->second) {
    NSMenuItem* item = state.item;
    if (current_codes.contains(state.codes)) {
      // The default accelerator is (still) assigned to this command: the item
      // shows and dispatches it.
      ObjCStorage::RestorePristine(state);
    } else {
      // The default accelerator was removed or moved to another command.
      // Clear the key equivalent so the menu no longer dispatches it. Note
      // that custom accelerators are deliberately not written into the menu:
      // they are registered with the browser's FocusManager instead (see
      // AcceleratorService::IsOsDispatched).
      if (@available(macos 12.0, *)) {
        item.allowsAutomaticKeyEquivalentLocalization = NO;
      }
      item.keyEquivalent = @"";
      item.keyEquivalentModifierMask = 0;
    }
  }
}

}  // namespace commands

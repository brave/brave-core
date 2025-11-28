// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commands/accelerator_service.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/containers/map_util.h"
#include "brave/app/brave_command_ids.h"
#include "brave/app/command_utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/commands/browser/accelerator_pref_manager.h"
#include "brave/components/commands/common/accelerator_parsing.h"
#include "brave/components/commands/common/commands.mojom-forward.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/accelerators/accelerator.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/pref_names.h"
#endif

namespace commands {

namespace {

mojom::CommandPtr ToMojoCommand(
    int command_id,
    const std::vector<ui::Accelerator>& accelerators,
    const std::vector<ui::Accelerator>& default_accelerators,
    const base::flat_set<ui::Accelerator>& unmodifiable) {
  auto command = mojom::Command::New();
  command->id = command_id;
  command->name = std::string(commands::GetCommandName(command_id));

  // Note: Default accelerators sometimes contains duplicate accelerators, so we
  // create a set before we check to see if something has been modified.
  auto default_accelerator_codes =
      base::MakeFlatSet<std::string>(default_accelerators, {}, &ToCodesString);

  command->modified =
      accelerators.size() != default_accelerator_codes.size() ||
      std::ranges::find_if(
          accelerators, [&default_accelerator_codes](const auto& a) {
            return !default_accelerator_codes.contains(ToCodesString(a));
          }) != accelerators.end();

  for (const auto& accelerator : accelerators) {
    auto a = mojom::Accelerator::New();
    a->codes = commands::ToCodesString(accelerator);
    a->keys = commands::ToKeysString(accelerator);
    a->unmodifiable = unmodifiable.contains(accelerator);
    command->accelerators.push_back(std::move(a));
  }
  return command;
}

base::flat_map<int, mojom::CommandPtr> ToMojoCommands(
    const Accelerators& commands,
    const Accelerators& default_commands,
    const base::flat_set<ui::Accelerator>& unmodifiable) {
  base::flat_map<int, mojom::CommandPtr> result;
  for (const auto& [command_id, accelerators] : commands) {
    auto* command_accelerators = base::FindOrNull(default_commands, command_id);
    result[command_id] =
        ToMojoCommand(command_id, accelerators,
                      command_accelerators ? *command_accelerators
                                           : std::vector<ui::Accelerator>(),
                      unmodifiable);
  }
  return result;
}

}  // namespace

AcceleratorService::AcceleratorService(
    PrefService* pref_service,
    Accelerators default_accelerators,
    base::flat_set<ui::Accelerator> system_managed)
    : pref_service_(pref_service),
      pref_manager_(pref_service, commands::GetCommands()),
      default_accelerators_(std::move(default_accelerators)),
      system_managed_(std::move(system_managed)) {
  Initialize();
}

AcceleratorService::~AcceleratorService() = default;

void AcceleratorService::Initialize() {
  accelerators_ = pref_manager_.GetAccelerators();
  UpdateDefaultAccelerators();

  // Include commands in the table which don't have any accelerators.
  auto commands = GetCommands();
  for (const auto& command_id : commands) {
    if (!accelerators_.contains(command_id)) {
      accelerators_[command_id] = {};
    }
  }
}

void AcceleratorService::UpdateDefaultAccelerators() {
  const auto& system_managed = system_managed_;
  auto old_defaults = pref_manager_.GetDefaultAccelerators();
  Accelerators added;
  Accelerators removed;

  // Handle new accelerators, and removed accelerators.
  for (const auto& [command_id, new_accelerators] : default_accelerators_) {
    const auto& old_accelerators = old_defaults[command_id];

    // Note all the added accelerators.
    std::ranges::copy_if(
        new_accelerators, std::back_inserter(added[command_id]),
        [&old_accelerators, &system_managed](const auto& accelerator) {
          return !base::Contains(old_accelerators, accelerator) ||
                 // If the accelerator is marked as a system command, be sure to
                 // reset it.
                 system_managed.contains(accelerator);
        });

    // Note all the removed accelerators.
    std::ranges::copy_if(
        old_accelerators, std::back_inserter(removed[command_id]),
        [&new_accelerators](const auto& accelerator) {
          return !base::Contains(new_accelerators, accelerator);
        });
  }

  // We also need to handle the case where a command was removed from the list
  // of default accelerators:
  for (const auto& [command_id, accelerators] : old_defaults) {
    // This is handled above.
    if (default_accelerators_.contains(command_id)) {
      continue;
    }

    // We used to have accelerators for this command, now we have none.
    std::ranges::copy(accelerators, std::back_inserter(removed[command_id]));
  }

  // Remove deleted accelerators
  for (const auto& [command_id, accelerators] : removed) {
    for (const auto& accelerator : accelerators) {
      UnassignAccelerator(command_id, accelerator);
    }
  }

  // Add new accelerators
  for (const auto& [command_id, accelerators] : added) {
    for (const auto& accelerator : accelerators) {
      AssignAccelerator(command_id, accelerator);
    }
  }

  // If anything changed, update the set of default prefs.
  if (!removed.empty() || !added.empty()) {
    pref_manager_.SetDefaultAccelerators(default_accelerators_);
    accelerators_ = pref_manager_.GetAccelerators();
  }
}

void AcceleratorService::BindInterface(
    mojo::PendingReceiver<CommandsService> pending_receiver) {
  receivers_.Add(this, std::move(pending_receiver));
}

void AcceleratorService::AssignAcceleratorToCommand(
    int command_id,
    const std::string& accelerator) {
  if (accelerator.empty()) {
    return;
  }
  NotifyCommandsChanged(
      AssignAccelerator(command_id, FromCodesString(accelerator)));
}

void AcceleratorService::UnassignAcceleratorFromCommand(
    int command_id,
    const std::string& accelerator) {
  if (accelerator.empty()) {
    return;
  }
  UnassignAccelerator(command_id, FromCodesString(accelerator));
  NotifyCommandsChanged({command_id});
}

void AcceleratorService::ResetAcceleratorsForCommand(int command_id) {
  std::vector<int> modified_commands = {command_id};

  // First, clear our shortcuts list.
  accelerators_[command_id].clear();
  pref_manager_.ClearAccelerators(command_id);

  // For each default shortcut for this command, assign it.
  const auto& default_accelerators = default_accelerators_[command_id];
  for (const auto& default_accelerator : default_accelerators) {
    auto additionally_modified =
        AssignAccelerator(command_id, default_accelerator);
    modified_commands.insert(modified_commands.end(),
                             additionally_modified.begin(),
                             additionally_modified.end());
  }

  NotifyCommandsChanged(modified_commands);
}

void AcceleratorService::ResetAccelerators() {
  std::vector<int> commands;
  for (const auto command : GetCommands()) {
    pref_manager_.ClearAccelerators(command);
    commands.push_back(command);

    // Make sure we add all the accelerators back.
    for (const auto& accelerator : default_accelerators_[command]) {
      pref_manager_.AddAccelerator(command, accelerator);
    }
  }

  // Load the default accelerators back.
  accelerators_ = pref_manager_.GetAccelerators();

  NotifyCommandsChanged(commands);
}

void AcceleratorService::GetKeyFromCode(const std::string& code,
                                        GetKeyFromCodeCallback callback) {
  std::move(callback).Run(CodeStringToKeyString(code));
}

void AcceleratorService::AddCommandsListener(
    mojo::PendingRemote<mojom::CommandsListener> listener) {
  auto id = mojo_listeners_.Add(std::move(listener));
  auto event = mojom::CommandsEvent::New();

  // Filter out commands that are disabled by policy
  auto filtered_accelerators = FilterCommandsByPolicy(accelerators_);
  auto filtered_default_accelerators =
      FilterCommandsByPolicy(default_accelerators_);

  event->addedOrUpdated = ToMojoCommands(
      filtered_accelerators, filtered_default_accelerators, system_managed_);
  mojo_listeners_.Get(id)->Changed(std::move(event));
}

void AcceleratorService::AddObserver(Observer* observer) {
  Accelerators changed;
  observers_.AddObserver(observer);
  const auto& system_managed = system_managed_;
  for (const auto& [command_id, accelerators] : accelerators_) {
    // Skip commands that are disabled by policy
    if (IsCommandDisabledByPolicy(command_id)) {
      continue;
    }

    std::ranges::copy_if(accelerators, std::back_inserter(changed[command_id]),
                         [&system_managed](const ui::Accelerator& accelerator) {
                           return !system_managed.contains(accelerator);
                         });
  }
  observer->OnAcceleratorsChanged(changed);
}

void AcceleratorService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

const Accelerators& AcceleratorService::GetAcceleratorsForTesting() {
  return accelerators_;
}

mojom::CommandPtr AcceleratorService::GetCommandForTesting(int command_id) {
  return ToMojoCommand(command_id, accelerators_[command_id],
                       default_accelerators_[command_id], system_managed_);
}

void AcceleratorService::Shutdown() {
  observers_.Clear();
  mojo_listeners_.Clear();
  receivers_.Clear();
}

std::vector<int> AcceleratorService::AssignAccelerator(
    int command_id,
    const ui::Accelerator& accelerator) {
  std::vector<int> modified_commands = {command_id};
  Accelerators& default_accelerators = default_accelerators_;
  auto& system_managed = system_managed_;

  // Find any other commands with this accelerator and remove it from them.
  for (auto& [other_command_id, accelerators] : accelerators_) {
    if (std::erase_if(
            accelerators, [&accelerator, &system_managed, &default_accelerators,
                           other_command_id](const auto& other) {
              bool is_default_accelerator =
                  base::Contains(default_accelerators[other_command_id], other);
              // Note: We don't erase system managed default accelerators, as
              // the system can register the same accelerator for multiple
              // commands, and we don't want resetting one to reset the other.
              return !(system_managed.contains(other) &&
                       is_default_accelerator) &&
                     ToCodesString(accelerator) == ToCodesString(other);
            })) {
      pref_manager_.RemoveAccelerator(other_command_id, accelerator);
      modified_commands.push_back(other_command_id);
    }
  }

  accelerators_[command_id].push_back(accelerator);
  pref_manager_.AddAccelerator(command_id, accelerator);
  return modified_commands;
}

void AcceleratorService::UnassignAccelerator(
    int command_id,
    const ui::Accelerator& accelerator) {
  std::erase(accelerators_[command_id], accelerator);
  pref_manager_.RemoveAccelerator(command_id, accelerator);
}

void AcceleratorService::NotifyCommandsChanged(
    const std::vector<int>& modified_ids) {
  Accelerators changed;
  auto event = mojom::CommandsEvent::New();
  const auto& system_managed = system_managed_;

  for (const auto& command_id : modified_ids) {
    // Skip commands that are disabled by policy
    if (IsCommandDisabledByPolicy(command_id)) {
      continue;
    }

    const auto& changed_command = accelerators_[command_id];
    auto* command_accelerators =
        base::FindOrNull(default_accelerators_, command_id);
    event->addedOrUpdated[command_id] =
        ToMojoCommand(command_id, changed_command,
                      command_accelerators ? *command_accelerators
                                           : std::vector<ui::Accelerator>(),
                      system_managed_);

    // Make sure system managed commands aren't registered with the Browser - as
    // that might break these commands being triggered from the system.
    std::ranges::copy_if(changed_command,
                         std::back_inserter(changed[command_id]),
                         [&system_managed](const ui::Accelerator& accelerator) {
                           return !system_managed.contains(accelerator);
                         });
  }

  for (const auto& listener : mojo_listeners_) {
    listener->Changed(event->Clone());
  }

  for (auto& listener : observers_) {
    listener.OnAcceleratorsChanged(changed);
  }
}

bool AcceleratorService::IsCommandDisabledByPolicy(int command_id) const {
  switch (command_id) {
    case IDC_CONFIGURE_BRAVE_NEWS:
      return pref_service_->GetBoolean(
          brave_news::prefs::kBraveNewsDisabledByPolicy);
    case IDC_SHOW_BRAVE_TALK:
      return pref_service_->GetBoolean(kBraveTalkDisabledByPolicy);
    case IDC_SHOW_BRAVE_VPN_PANEL:
    case IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON:
    case IDC_TOGGLE_BRAVE_VPN_TRAY_ICON:
    case IDC_SEND_BRAVE_VPN_FEEDBACK:
    case IDC_ABOUT_BRAVE_VPN:
    case IDC_MANAGE_BRAVE_VPN_PLAN:
    case IDC_TOGGLE_BRAVE_VPN:
#if BUILDFLAG(ENABLE_BRAVE_VPN)
      return pref_service_->GetBoolean(
          brave_vpn::prefs::kManagedBraveVPNDisabled);
#else
      return true;  // VPN not compiled in, always disabled
#endif
    case IDC_SHOW_BRAVE_WALLET:
    case IDC_SHOW_BRAVE_WALLET_PANEL:
    case IDC_CLOSE_BRAVE_WALLET_PANEL:
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
      return pref_service_->GetBoolean(
          brave_wallet::kBraveWalletDisabledByPolicy);
#else
      return true;  // Wallet not compiled in, always disabled
#endif
    case IDC_SHOW_BRAVE_REWARDS:
    case IDC_OFFERS_AND_REWARDS_FOR_PAGE:
      return pref_service_->GetBoolean(brave_rewards::prefs::kDisabledByPolicy);
#if BUILDFLAG(ENABLE_AI_CHAT)
    case IDC_TOGGLE_AI_CHAT:
    case IDC_OPEN_FULL_PAGE_CHAT:
      return !pref_service_->GetBoolean(ai_chat::prefs::kEnabledByPolicy);
#endif
    case IDC_NEW_OFFTHERECORD_WINDOW_TOR:
    case IDC_NEW_TOR_CONNECTION_FOR_SITE:
#if BUILDFLAG(ENABLE_TOR)
      return g_browser_process->local_state()->GetBoolean(
          tor::prefs::kTorDisabled);
#else
      return true;  // Tor not compiled in, always disabled
#endif
    case IDC_SPEEDREADER_ICON_ONCLICK:
#if BUILDFLAG(ENABLE_SPEEDREADER)
      return !pref_service_->GetBoolean(speedreader::kSpeedreaderEnabled);
#else
      return true;  // Speedreader not compiled in, always disabled
#endif
    case IDC_SHOW_WAYBACK_MACHINE_BUBBLE:
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
      return !pref_service_->GetBoolean(kBraveWaybackMachineEnabled);
#else
      return true;  // Wayback Machine not compiled in, always disabled
#endif
    default:
      return false;  // Unknown command - Not subject to policy filtering
  }
}

Accelerators AcceleratorService::FilterCommandsByPolicy(
    const Accelerators& commands) const {
  Accelerators filtered_commands;
  for (const auto& [command_id, accelerators] : commands) {
    if (!IsCommandDisabledByPolicy(command_id)) {
      filtered_commands[command_id] = accelerators;
    }
  }
  return filtered_commands;
}

}  // namespace commands

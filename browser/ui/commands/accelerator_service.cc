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
#include "base/containers/cxx20_erase_vector.h"
#include "base/containers/flat_map.h"
#include "base/ranges/algorithm.h"
#include "brave/app/command_utils.h"
#include "brave/components/commands/browser/accelerator_pref_manager.h"
#include "brave/components/commands/common/accelerator_parsing.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {

namespace {

mojom::CommandPtr ToMojoCommand(
    int command_id,
    const std::vector<ui::Accelerator>& accelerators,
    const std::vector<ui::Accelerator>& default_accelerators) {
  auto command = mojom::Command::New();
  command->id = command_id;
  command->name = std::string(commands::GetCommandName(command_id));
  command->modified = accelerators.size() != default_accelerators.size() ||
                      base::ranges::find_if(
                          accelerators, [default_accelerators](const auto& a) {
                            return !base::Contains(default_accelerators, a);
                          }) != accelerators.end();

  for (const auto& accelerator : accelerators) {
    auto a = mojom::Accelerator::New();
    a->codes = commands::ToCodesString(accelerator);
    a->keys = commands::ToKeysString(accelerator);
    command->accelerators.push_back(std::move(a));
  }
  return command;
}

base::flat_map<int, mojom::CommandPtr> ToMojoCommands(
    const Accelerators& commands,
    const Accelerators& default_commands) {
  base::flat_map<int, mojom::CommandPtr> result;
  for (const auto& [command_id, accelerators] : commands) {
    auto it = default_commands.find(command_id);
    result[command_id] = ToMojoCommand(command_id, accelerators,
                                       it == default_commands.end()
                                           ? std::vector<ui::Accelerator>()
                                           : it->second);
  }
  return result;
}

}  // namespace

AcceleratorService::AcceleratorService(PrefService* pref_service,
                                       Accelerators default_accelerators)
    : pref_manager_(pref_service),
      default_accelerators_(std::move(default_accelerators)) {
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
  auto old_defaults = pref_manager_.GetDefaultAccelerators();

  Accelerators added;
  Accelerators removed;

  // Handle new accelerators, and removed accelerators.
  for (const auto& [command_id, new_accelerators] : default_accelerators_) {
    const auto& old_accelerators = old_defaults[command_id];

    // Note all the added accelerators.
    base::ranges::copy_if(
        new_accelerators, std::back_inserter(added[command_id]),
        [&old_accelerators](const auto& accelerator) {
          return !base::Contains(old_accelerators, accelerator);
        });

    // Note all the removed accelerators.
    base::ranges::copy_if(
        old_accelerators, std::back_inserter(removed[command_id]),
        [&new_accelerators](const auto& accelerator) {
          return !base::Contains(new_accelerators, accelerator);
        });
  }

  // We also need to handle the case where a command was removed from the list
  // of default accelerators:
  for (const auto& [command_id, accelerators] : old_defaults) {
    // This is handled above.
    if (base::Contains(default_accelerators_, command_id)) {
      continue;
    }

    // We used to have accelerators for this command, now we have none.
    base::ranges::copy(accelerators, std::back_inserter(removed[command_id]));
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
  accelerators_ = default_accelerators_;

  std::vector<int> commands;
  for (const auto command : GetCommands()) {
    pref_manager_.ClearAccelerators(command);
    commands.push_back(command);

    // Make sure we add all the accelerators back.
    for (const auto& accelerator : accelerators_[command]) {
      pref_manager_.AddAccelerator(command, accelerator);
    }
  }
  NotifyCommandsChanged(commands);
}

void AcceleratorService::AddCommandsListener(
    mojo::PendingRemote<mojom::CommandsListener> listener) {
  auto id = mojo_listeners_.Add(std::move(listener));
  auto event = mojom::CommandsEvent::New();
  event->addedOrUpdated = ToMojoCommands(accelerators_, default_accelerators_);
  mojo_listeners_.Get(id)->Changed(std::move(event));
}

void AcceleratorService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
  observer->OnAcceleratorsChanged(accelerators_);
}

void AcceleratorService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

const Accelerators& AcceleratorService::GetAcceleratorsForTesting() {
  return accelerators_;
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

  // Find any other commands with this accelerator and remove it from them.
  for (auto& [other_command_id, accelerators] : accelerators_) {
    if (base::Contains(accelerators, accelerator)) {
      base::Erase(accelerators, accelerator);
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
  base::Erase(accelerators_[command_id], accelerator);
  pref_manager_.RemoveAccelerator(command_id, accelerator);
}

void AcceleratorService::NotifyCommandsChanged(
    const std::vector<int>& modified_ids) {
  Accelerators changed;
  auto event = mojom::CommandsEvent::New();

  for (const auto& command_id : modified_ids) {
    const auto& changed_command = accelerators_[command_id];
    auto it = default_accelerators_.find(command_id);
    event->addedOrUpdated[command_id] = ToMojoCommand(
        command_id, changed_command,
        it == default_accelerators_.end() ? std::vector<ui::Accelerator>()
                                          : it->second);
    changed[command_id] = changed_command;
  }

  for (const auto& listener : mojo_listeners_) {
    listener->Changed(event->Clone());
  }

  for (auto& listener : observers_) {
    listener.OnAcceleratorsChanged(changed);
  }
}

}  // namespace commands

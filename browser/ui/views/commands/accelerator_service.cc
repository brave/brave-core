// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/commands/accelerator_service.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/cxx20_erase_vector.h"
#include "base/containers/flat_map.h"
#include "brave/app/command_utils.h"
#include "brave/components/commands/common/accelerator_parsing.h"
#include "brave/components/commands/common/accelerator_pref_manager.h"
#include "brave/components/commands/common/commands.mojom-forward.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/accelerators/accelerator.h"

namespace commands {

namespace {

mojom::CommandPtr ToMojoCommand(
    int command_id,
    const std::vector<ui::Accelerator>& accelerators) {
  auto command = mojom::Command::New();
  command->id = command_id;
  command->name = commands::GetCommandName(command_id);
  for (const auto& accelerator : accelerators) {
    auto a = mojom::Accelerator::New();
    a->codes = commands::ToCodesString(accelerator);
    a->keys = commands::ToKeysString(accelerator);
    command->accelerators.push_back(std::move(a));
  }
  return command;
}

base::flat_map<int, mojom::CommandPtr> ToMojoCommands(
    const base::flat_map<int, std::vector<ui::Accelerator>>& commands) {
  base::flat_map<int, mojom::CommandPtr> result;
  for (const auto& [command_id, accelerators] : commands) {
    result[command_id] = ToMojoCommand(command_id, accelerators);
  }
  return result;
}

}  // namespace

AcceleratorService::AcceleratorService(PrefService* pref_service,
                                       const Accelerators& default_accelerators)
    : pref_manager_(pref_service), default_accelerators_(default_accelerators) {
  Initialize();
}

AcceleratorService::~AcceleratorService() = default;

void AcceleratorService::Initialize() {
  auto accelerators = pref_manager_.GetAccelerators();
  if (accelerators.size() == 0) {
    for (const auto& [command_id, default_accelerators] :
         default_accelerators_) {
      for (const auto& default_accelerator : default_accelerators) {
        pref_manager_.AddAccelerator(command_id, default_accelerator);
      }
    }
    accelerators = pref_manager_.GetAccelerators();
  }

  auto commands = GetCommands();
  for (const auto& command : commands) {
    accelerators_.insert({command, {}});

    auto it = accelerators.find(command);
    if (it == accelerators.end()) {
      continue;
    }

    // Initialize the default table.
    for (const auto& accelerator : it->second) {
      accelerators_[command].push_back(accelerator);
    }
  }
}

void AcceleratorService::BindInterface(
    mojo::PendingReceiver<CommandsService> pending_receiver) {
  receivers_.Add(this, std::move(pending_receiver));
}

void AcceleratorService::AssignAcceleratorToCommand(
    int command_id,
    const std::string& accelerator) {
  NotifyCommandsChanged(
      AssignAcceleratorToCommand(command_id, FromCodesString(accelerator)));
}

void AcceleratorService::UnassignAcceleratorFromCommand(
    int command_id,
    const std::string& accelerator) {
  auto a = FromCodesString(accelerator);
  base::Erase(accelerators_[command_id], a);
  pref_manager_.RemoveAccelerator(command_id, a);
  NotifyCommandsChanged({command_id});
}

void AcceleratorService::ResetAcceleratorsForCommand(int command_id) {
  std::vector<int> modified_commands = {command_id};

  // First, clear our shortcuts list.
  pref_manager_.ClearAccelerators(command_id);

  // For each default shortcut for this command, assign it.
  const auto& default_accelerators = default_accelerators_[command_id];
  for (const auto& default_accelerator : default_accelerators) {
    auto additionally_modified =
        AssignAcceleratorToCommand(command_id, default_accelerator);
    modified_commands.insert(modified_commands.end(),
                             additionally_modified.begin(),
                             additionally_modified.end());
  }

  NotifyCommandsChanged(modified_commands);
}

void AcceleratorService::AddCommandsListener(
    mojo::PendingRemote<mojom::CommandsListener> listener) {
  auto id = mojo_listeners_.Add(std::move(listener));
  auto event = mojom::CommandsEvent::New();
  event->addedOrUpdated = ToMojoCommands(accelerators_);
  mojo_listeners_.Get(id)->Changed(std::move(event));
}

void AcceleratorService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
  observer->OnAcceleratorsChanged(accelerators_);
}

void AcceleratorService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

std::vector<int> AcceleratorService::AssignAcceleratorToCommand(
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

void AcceleratorService::NotifyCommandsChanged(
    const std::vector<int>& modified_ids) {
  Accelerators changed;
  auto event = mojom::CommandsEvent::New();

  for (const auto& command_id : modified_ids) {
    const auto& changed_command = accelerators_[command_id];
    event->addedOrUpdated[command_id] =
        ToMojoCommand(command_id, changed_command);
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

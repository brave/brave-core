// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_SERVICE_H_
#define BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_SERVICE_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/commands/browser/accelerator_pref_manager.h"
#include "brave/components/commands/common/commands.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "ui/base/accelerators/accelerator.h"

class PrefService;

namespace commands {

using Accelerators = AcceleratorPrefManager::Accelerators;

class AcceleratorService : public mojom::CommandsService, public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnAcceleratorsChanged(const Accelerators& changed) = 0;
  };

  AcceleratorService(PrefService* pref_service,
                     Accelerators default_accelerators);
  AcceleratorService(const AcceleratorService&) = delete;
  AcceleratorService& operator=(const AcceleratorService&) = delete;
  ~AcceleratorService() override;

  void Initialize();
  void UpdateDefaultAccelerators();
  void BindInterface(mojo::PendingReceiver<CommandsService> pending_receiver);

  void AssignAcceleratorToCommand(int command_id,
                                  const std::string& accelerator) override;
  void UnassignAcceleratorFromCommand(int command_id,
                                      const std::string& accelerator) override;
  void ResetAcceleratorsForCommand(int command_id) override;
  void ResetAccelerators() override;

  void AddCommandsListener(
      mojo::PendingRemote<mojom::CommandsListener> listener) override;
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  const Accelerators& GetAcceleratorsForTesting();

  // KeyedService:
  void Shutdown() override;

 private:
  // Returns all the command_ids whose accelerators were affected by the set and
  // does not notify observers.
  std::vector<int> AssignAccelerator(int command_id,
                                     const ui::Accelerator& accelerator);
  // Unassigns an accelerator and does not notify observers.
  void UnassignAccelerator(int command_id, const ui::Accelerator& accelerator);
  void NotifyCommandsChanged(const std::vector<int>& modified_ids);

  AcceleratorPrefManager pref_manager_;
  Accelerators accelerators_;
  Accelerators default_accelerators_;

  mojo::ReceiverSet<CommandsService> receivers_;
  mojo::RemoteSet<mojom::CommandsListener> mojo_listeners_;
  base::ObserverList<Observer> observers_;
};
}  // namespace commands

#endif  // BRAVE_BROWSER_UI_COMMANDS_ACCELERATOR_SERVICE_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_

#include <string>

#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "components/component_updater/component_updater_service.h"

using brave_component_updater::BraveComponent;

namespace base {
class SequencedTaskRunner;
}

class PrefService;

namespace brave {

class BraveComponentUpdaterDelegate : public BraveComponent::Delegate {
 public:
  BraveComponentUpdaterDelegate();
  BraveComponentUpdaterDelegate(const BraveComponentUpdaterDelegate&) = delete;
  BraveComponentUpdaterDelegate& operator=(
      const BraveComponentUpdaterDelegate&) = delete;
  ~BraveComponentUpdaterDelegate() override;

  using ComponentObserver = update_client::UpdateClient::Observer;
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

}  // namespace brave

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_

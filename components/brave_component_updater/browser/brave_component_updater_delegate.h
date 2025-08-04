/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_

#include <string>

#include "base/component_export.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

namespace base {
class SequencedTaskRunner;
}

namespace component_updater {
class ComponentUpdateService;
}

class PrefService;

namespace brave_component_updater {

class COMPONENT_EXPORT(BRAVE_COMPONENT_UPDATER) BraveComponentUpdaterDelegate
    : public BraveComponent::Delegate {
 public:
  BraveComponentUpdaterDelegate(
      component_updater::ComponentUpdateService* updater,
      PrefService* local_state,
      const std::string& locale);
  BraveComponentUpdaterDelegate(const BraveComponentUpdaterDelegate&) = delete;
  BraveComponentUpdaterDelegate& operator=(
      const BraveComponentUpdaterDelegate&) = delete;
  ~BraveComponentUpdaterDelegate() override;

  using ComponentObserver = update_client::UpdateClient::Observer;
  // brave_component_updater::BraveComponent::Delegate implementation
  void Register(const std::string& component_name,
                const std::string& component_base64_public_key,
                base::OnceClosure registered_callback,
                brave_component_updater::BraveComponent::ReadyCallback
                    ready_callback) override;
  bool Unregister(const std::string& component_id) override;
  void EnsureInstalled(const std::string& component_id) override;

  void AddObserver(ComponentObserver* observer) override;
  void RemoveObserver(ComponentObserver* observer) override;

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() override;

  const std::string& locale() const override;
  PrefService* local_state() override;

 private:
  const raw_ref<component_updater::ComponentUpdateService> component_updater_;
  const raw_ref<PrefService> local_state_;
  std::string locale_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace brave_component_updater

#endif  // BRAVE_COMPONENTS_BRAVE_COMPONENT_UPDATER_BROWSER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_

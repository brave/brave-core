/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

using brave_component_updater::BraveComponent;

namespace brave {

class BraveComponentUpdaterDelegate : public BraveComponent::Delegate {
 public:
  BraveComponentUpdaterDelegate();
  ~BraveComponentUpdaterDelegate() override;

  // brave_component_updater::BraveComponent::Delegate implementation
  void Register(const std::string& component_name,
                const std::string& component_base64_public_key,
                base::OnceClosure registered_callback,
                BraveComponent::ReadyCallback ready_callback) override;
  bool Unregister(const std::string& component_id) override;
  void OnDemandUpdate(const std::string& component_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveComponentUpdaterDelegate);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_COMPONENT_UPDATER_DELEGATE_H_

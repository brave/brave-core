/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_COMPONENT_UPDATER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_COMPONENT_UPDATER_DELEGATE_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"

class PrefService;

namespace brave_ads::test {

// Minimal no-op `BraveComponent::Delegate` for use in unit tests that need a
// `ResourceComponent` without a real component updater.
class FakeComponentUpdaterDelegate final
    : public brave_component_updater::BraveComponent::Delegate {
 public:
  FakeComponentUpdaterDelegate();

  FakeComponentUpdaterDelegate(const FakeComponentUpdaterDelegate&) = delete;
  FakeComponentUpdaterDelegate& operator=(const FakeComponentUpdaterDelegate&) =
      delete;

  ~FakeComponentUpdaterDelegate() override;

  void Register(const std::string& /*component_name*/,
                const std::string& /*component_base64_public_key*/,
                base::OnceClosure registered_callback,
                brave_component_updater::BraveComponent::ReadyCallback
                /*ready_callback*/) override;

  bool Unregister(const std::string& /*component_id*/) override;

  void EnsureInstalled(const std::string& /*component_id*/) override {}

  void AddObserver(brave_component_updater::BraveComponent::ComponentObserver*
                   /*observer*/) override {}

  void RemoveObserver(
      brave_component_updater::BraveComponent::ComponentObserver*
      /*observer*/) override {}

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() override;

  const std::string& locale() const override;

  PrefService* local_state() override;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_COMPONENT_UPDATER_DELEGATE_H_

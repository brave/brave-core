/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_component_updater_delegate.h"

#include "base/strings/string_util.h"

namespace brave_ads::test {

FakeComponentUpdaterDelegate::FakeComponentUpdaterDelegate() = default;

FakeComponentUpdaterDelegate::~FakeComponentUpdaterDelegate() = default;

void FakeComponentUpdaterDelegate::Register(
    const std::string& /*component_name*/,
    const std::string& /*component_base64_public_key*/,
    base::OnceClosure registered_callback,
    brave_component_updater::BraveComponent::ReadyCallback /*ready_callback*/) {
  if (registered_callback) {
    std::move(registered_callback).Run();
  }
}

bool FakeComponentUpdaterDelegate::Unregister(
    const std::string& /*component_id*/) {
  return true;
}

scoped_refptr<base::SequencedTaskRunner>
FakeComponentUpdaterDelegate::GetTaskRunner() {
  return nullptr;
}

const std::string& FakeComponentUpdaterDelegate::locale() const {
  return base::EmptyString();
}

PrefService* FakeComponentUpdaterDelegate::local_state() {
  return nullptr;
}

}  // namespace brave_ads::test

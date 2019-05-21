/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_component.h"

#include "base/bind.h"
#include "base/sequenced_task_runner.h"

namespace brave_component_updater {

BraveComponent::BraveComponent(Delegate* delegate)
    : delegate_(delegate),
      weak_factory_(this) {}

BraveComponent::~BraveComponent() {
}

void BraveComponent::Register(const std::string& component_name,
                              const std::string& component_id,
                              const std::string& component_base64_public_key) {
  component_name_ = component_name;
  component_id_ = component_id;
  component_base64_public_key_ = component_base64_public_key;

  auto registered_callback =
      base::BindOnce(&BraveComponent::OnComponentRegistered,
                     delegate_,
                     component_id);
  auto ready_callback =
      base::BindOnce(&BraveComponent::OnComponentReady,
                     weak_factory_.GetWeakPtr(),
                     component_id);

  delegate_->Register(component_name_,
                      component_base64_public_key_,
                      std::move(registered_callback),
                      std::move(ready_callback));
}

bool BraveComponent::Unregister() {
  return delegate_->Unregister(component_id_);
}

scoped_refptr<base::SequencedTaskRunner> BraveComponent::GetTaskRunner() {
  return delegate_->GetTaskRunner();
}

void BraveComponent::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {}

// static
void BraveComponent::OnComponentRegistered(
    Delegate* delegate,
    const std::string& component_id) {
  delegate->OnDemandUpdate(component_id);
}

}  // namespace brave_component_updater

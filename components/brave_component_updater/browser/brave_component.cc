/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_component.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_component_updater {

BraveComponent::BraveComponent(Delegate* delegate)
    : delegate_(delegate),
      weak_factory_(this) {}

BraveComponent::~BraveComponent() = default;

void BraveComponent::Register(const std::string& component_name,
                              const std::string& component_id,
                              const std::string& component_base64_public_key) {
  VLOG(2) << "register component: " << component_id;
  component_name_ = component_name;
  component_id_ = component_id;
  component_base64_public_key_ = component_base64_public_key;

  auto registered_callback =
      base::BindOnce(&BraveComponent::OnComponentRegistered,
                     delegate_,
                     component_id);
  auto ready_callback =
      base::BindRepeating(&BraveComponent::OnComponentReadyInternal,
                          weak_factory_.GetWeakPtr(),
                          component_id);

  delegate_->Register(component_name_,
                      component_base64_public_key_,
                      std::move(registered_callback),
                      ready_callback);
}

bool BraveComponent::Unregister() {
  VLOG(2) << "unregister component: " << component_id_;
  return delegate_->Unregister(component_id_);
}

scoped_refptr<base::SequencedTaskRunner> BraveComponent::GetTaskRunner() {
  return delegate_->GetTaskRunner();
}

void BraveComponent::AddObserver(ComponentObserver* observer) {
  DCHECK(delegate_);
  delegate_->AddObserver(observer);
}

void BraveComponent::RemoveObserver(ComponentObserver* observer) {
  DCHECK(delegate_);
  delegate_->RemoveObserver(observer);
}

void BraveComponent::OnComponentReadyInternal(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  VLOG(2) << "component ready: " << manifest;
  OnComponentReady(component_id, install_dir, manifest);
}

void BraveComponent::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {}

// static
void BraveComponent::OnComponentRegistered(
    Delegate* delegate,
    const std::string& component_id) {
  VLOG(2) << "component registered: " << component_id;
  delegate->OnDemandInstall(component_id);
}

BraveComponent::Delegate* BraveComponent::delegate() {
  return delegate_;
}

}  // namespace brave_component_updater

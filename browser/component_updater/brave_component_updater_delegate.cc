/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/component_updater/brave_component_updater_delegate.h"

#include <utility>

#include "base/sequenced_task_runner.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/browser/component_updater/brave_component_installer.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "chrome/browser/browser_process.h"
#include "components/component_updater/component_updater_service.h"

using brave_component_updater::BraveComponent;
using brave_component_updater::BraveOnDemandUpdater;

namespace brave {

BraveComponentUpdaterDelegate::BraveComponentUpdaterDelegate()
    : task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {}

BraveComponentUpdaterDelegate::~BraveComponentUpdaterDelegate() {}

void BraveComponentUpdaterDelegate::Register(
    const std::string& component_name,
    const std::string& component_base64_public_key,
    base::OnceClosure registered_callback,
    BraveComponent::ReadyCallback ready_callback) {
  brave::RegisterComponent(g_browser_process->component_updater(),
                           component_name,
                           component_base64_public_key,
                           std::move(registered_callback),
                           std::move(ready_callback));
}

bool BraveComponentUpdaterDelegate::Unregister(
    const std::string& component_id) {
  return g_browser_process->component_updater()->UnregisterComponent(
      component_id);
}

void BraveComponentUpdaterDelegate::OnDemandUpdate(
    const std::string& component_id) {
  BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(component_id);
}

void BraveComponentUpdaterDelegate::AddObserver(ComponentObserver* observer) {
  g_browser_process->component_updater()->AddObserver(observer);
}

void BraveComponentUpdaterDelegate::RemoveObserver(
    ComponentObserver* observer) {
  g_browser_process->component_updater()->RemoveObserver(observer);
}

scoped_refptr<base::SequencedTaskRunner>
BraveComponentUpdaterDelegate::GetTaskRunner() {
  return task_runner_;
}

}  // namespace brave

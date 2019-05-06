/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/component_updater/brave_crx_update_service.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/component_updater/update_scheduler.h"
#include "components/update_client/configurator.h"
#include "components/update_client/task_update.h"
#include "components/update_client/update_client_internal.h"
#include "components/update_client/update_engine.h"
#include "components/update_client/utils.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/brave_extension_provider.h"
#endif

namespace component_updater {

BraveCrxUpdateService::BraveCrxUpdateService(
    scoped_refptr<Configurator> config,
    std::unique_ptr<UpdateScheduler> scheduler,
    scoped_refptr<UpdateClient> update_client)
    : CrxUpdateService(config, std::move(scheduler), update_client) {}

void BraveCrxUpdateService::Start() {
  DCHECK(thread_checker_.CalledOnValidThread());
  scheduler_->Schedule(
      base::TimeDelta::FromSeconds(config_->InitialDelay()),
      base::TimeDelta::FromSeconds(config_->NextCheckDelay()),
      base::Bind(base::IgnoreResult(&BraveCrxUpdateService::CheckForUpdates),
                 base::Unretained(this)),
      base::DoNothing());
}

bool BraveCrxUpdateService::RegisterComponent(const CrxComponent& component) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (component.pk_hash.empty() || !component.version.IsValid() ||
      !component.installer) {
    return false;
  }

  // Update the registration data if the component has been registered before.
  const std::string id(GetCrxComponentID(component));
  auto it = components_.find(id);
  if (it != components_.end()) {
    it->second = component;
    return true;
  }

  components_.insert(std::make_pair(id, component));
  components_order_.push_back(id);
  for (const auto& mime_type : component.handled_mime_types)
    component_ids_by_mime_type_[mime_type] = id;

  // Create an initial state for this component. The state is mutated in
  // response to events from the UpdateClient instance.
  CrxUpdateItem item;
  item.id = id;
  item.component = component;
  const auto inserted = component_states_.insert(std::make_pair(id, item));
  DCHECK(inserted.second);

  // Start the timer if this is the first component registered. The first timer
  // event occurs after an interval defined by the component update
  // configurator. The subsequent timer events are repeated with a period
  // defined by the same configurator.
  if (components_.size() == 1)
    Start();

  return true;
}

bool BraveCrxUpdateService::CheckForUpdates(
    UpdateScheduler::OnFinishedCallback on_finished) {
  DCHECK(thread_checker_.CalledOnValidThread());

  std::vector<std::string> secure_ids;    // Requires HTTPS for update checks.
  std::vector<std::string> unsecure_ids;  // Can fallback to HTTP.
  for (const auto id : components_order_) {
    DCHECK(components_.find(id) != components_.end());
#if BUILDFLAG(ENABLE_EXTENSIONS)
    if (!extensions::BraveExtensionProvider::IsVetted(id)) {
      continue;
    }
#endif
    const auto component = GetComponent(id);
    if (!component || component->requires_network_encryption)
      secure_ids.push_back(id);
    else
      unsecure_ids.push_back(id);
  }

  if (unsecure_ids.empty() && secure_ids.empty()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  std::move(on_finished));
    return true;
  }

  Callback on_finished_callback = base::BindOnce(
      [](UpdateScheduler::OnFinishedCallback on_finished,
         update_client::Error error) { std::move(on_finished).Run(); },
      std::move(on_finished));

  if (!unsecure_ids.empty()) {
    for (auto id : unsecure_ids) {
      update_client_->Update(
          {id},
          base::BindOnce(&CrxUpdateService::GetCrxComponents,
                         base::Unretained(this)),
          false,
          base::BindOnce(&CrxUpdateService::OnUpdateComplete,
                         base::Unretained(this),
                         secure_ids.empty() && (id == unsecure_ids.back())
                             ? std::move(on_finished_callback)
                             : Callback(),
                         base::TimeTicks::Now()));
    }
  }

  if (!secure_ids.empty()) {
    for (auto id : secure_ids) {
      update_client_->Update(
          {id},
          base::BindOnce(&CrxUpdateService::GetCrxComponents,
                         base::Unretained(this)),
          false,
          base::BindOnce(
              &CrxUpdateService::OnUpdateComplete, base::Unretained(this),
              (id == secure_ids.back()) ? std::move(on_finished_callback)
                                        : Callback(),
              base::TimeTicks::Now()));
    }
  }

  return true;
}

}  // namespace component_updater

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/component_updater/component_updater_service.h"

#include "base/notreached.h"
#include "components/component_updater/component_updater_service_internal.h"

#include "src/components/component_updater/component_updater_service.cc"

namespace component_updater {

void CrxUpdateService::OnDemandUpdate(const std::vector<std::string>& ids,
                                      Priority priority,
                                      Callback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  for (const auto& id : ids) {
    if (!GetComponent(id)) {
      if (callback) {
        base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE, base::BindOnce(std::move(callback),
                                      update_client::Error::INVALID_ARGUMENT));
      }
      return;
    }
  }

  auto crx_data_callback = base::BindOnce(&CrxUpdateService::GetCrxComponents,
                                          base::Unretained(this));
  auto update_complete_callback = base::BindOnce(
      &CrxUpdateService::OnUpdateComplete, base::Unretained(this),
      std::move(callback), base::TimeTicks::Now());

  update_client_->Update(ids, std::move(crx_data_callback), {},
                         priority == Priority::FOREGROUND,
                         std::move(update_complete_callback));
}

void OnDemandUpdater::OnDemandUpdate(const std::vector<std::string>& ids,
                                     Priority priority,
                                     Callback callback) {
  NOTREACHED_NORETURN();
}

}  // namespace component_updater

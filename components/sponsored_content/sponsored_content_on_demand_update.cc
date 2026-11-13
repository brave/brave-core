/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sponsored_content/sponsored_content_on_demand_update.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_updater_service.h"

namespace sponsored_content {

namespace {

void SponsoredContentComponentUpdateCompleteCallback(
    const std::string& component_id,
    const update_client::Error error) {
  switch (error) {
    case update_client::Error::NONE: {
      VLOG(6) << "Checked for updates to Sponsored Content component with ID "
              << component_id;
      break;
    }
    case update_client::Error::UPDATE_IN_PROGRESS: {
      VLOG(6) << "Sponsored Content component update in progress";
      break;
    }
    case update_client::Error::UPDATE_CANCELED: {
      VLOG(6) << "Sponsored Content component update canceled";
      break;
    }
    case update_client::Error::RETRY_LATER: {
      VLOG(6) << "Sponsored Content component update failed, retry later";
      break;
    }
    case update_client::Error::SERVICE_ERROR: {
      VLOG(6) << "Sponsored Content component update failed due to service "
                 "error";
      break;
    }
    case update_client::Error::UPDATE_CHECK_ERROR: {
      VLOG(6) << "Sponsored Content component update failed due to update "
                 "check error";
      break;
    }
    case update_client::Error::CRX_NOT_FOUND: {
      VLOG(6) << "Sponsored Content component update failed due to CRX not "
                 "found";
      break;
    }
    case update_client::Error::INVALID_ARGUMENT: {
      VLOG(6) << "Sponsored Content component update failed due to invalid "
                 "argument";
      break;
    }
    case update_client::Error::BAD_CRX_DATA_CALLBACK: {
      VLOG(6) << "Sponsored Content component update failed due to bad CRX "
                 "data callback";
      break;
    }
    case update_client::Error::UPDATE_DISABLED: {
      VLOG(6) << "Sponsored Content component update disabled";
      break;
    }
  }
}

}  // namespace

void QueueSponsoredContentComponentUpdate(const std::string& component_id) {
  VLOG(6) << "Queueing update check for Sponsored Content component with ID "
          << component_id;

  if (brave_component_updater::BraveOnDemandUpdater::GetInstance()
          ->is_component_update_disabled()) {
    SponsoredContentComponentUpdateCompleteCallback(
        component_id, update_client::Error::UPDATE_CHECK_ERROR);
    return;
  }

  brave_component_updater::BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(
      component_id, component_updater::OnDemandUpdater::Priority::FOREGROUND,
      base::BindOnce(&SponsoredContentComponentUpdateCompleteCallback,
                     component_id));
}

}  // namespace sponsored_content

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_update_util.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_updater_service.h"

namespace ntp_background_images {

namespace {

void CheckAndUpdateSponsoredImagesComponentCallback(
    const std::string& component_id,
    const update_client::Error error) {
  switch (error) {
    case update_client::Error::NONE: {
      VLOG(6)
          << "Checked for updates to NTP Sponsored Images component with ID "
          << component_id;
      break;
    }
    case update_client::Error::UPDATE_IN_PROGRESS: {
      VLOG(6) << "NTP Sponsored Images component update in progress";
      break;
    }
    case update_client::Error::UPDATE_CANCELED: {
      VLOG(6) << "NTP Sponsored Images component update canceled";
      break;
    }
    case update_client::Error::RETRY_LATER: {
      VLOG(6) << "NTP Sponsored Images component update failed, retry later";
      break;
    }
    case update_client::Error::SERVICE_ERROR: {
      VLOG(6) << "NTP Sponsored Images component update failed due to service "
                 "error";
      break;
    }
    case update_client::Error::UPDATE_CHECK_ERROR: {
      VLOG(6) << "NTP Sponsored Images component update failed due to update "
                 "check error";
      break;
    }
    case update_client::Error::CRX_NOT_FOUND: {
      VLOG(6) << "NTP Sponsored Images component update failed due to CRX not "
                 "found";
      break;
    }
    case update_client::Error::INVALID_ARGUMENT: {
      VLOG(6) << "NTP Sponsored Images component update failed due to invalid "
                 "argument";
      break;
    }
    case update_client::Error::BAD_CRX_DATA_CALLBACK: {
      VLOG(6) << "NTP Sponsored Images component update failed due to bad CRX "
                 "data callback";
      break;
    }
    case update_client::Error::MAX_VALUE: {
      VLOG(6) << "NTP Sponsored Images component update failed due to unknown "
                 "error";
      break;
    }
  }
}

}  // namespace

void CheckAndUpdateSponsoredImagesComponent(const std::string& component_id) {
  VLOG(6) << "Checking for updates to NTP Sponsored Images component with ID "
          << component_id;

  brave_component_updater::BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(
      component_id, component_updater::OnDemandUpdater::Priority::FOREGROUND,
      base::BindOnce(&CheckAndUpdateSponsoredImagesComponentCallback,
                     component_id));
}

}  // namespace ntp_background_images

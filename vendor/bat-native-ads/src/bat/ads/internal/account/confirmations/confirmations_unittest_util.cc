/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmations_unittest_util.h"

#include "base/guid.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/unittest_time_util.h"

namespace ads {

ConfirmationInfo BuildConfirmation(const std::string& id,
                                   const std::string& creative_instance_id,
                                   const ConfirmationType& type) {
  ConfirmationInfo confirmation;
  confirmation.id = id;
  confirmation.creative_instance_id = creative_instance_id;
  confirmation.type = type;
  confirmation.created_at = Now();

  return confirmation;
}

ConfirmationInfo BuildConfirmation(const std::string& creative_instance_id,
                                   const ConfirmationType& type) {
  const std::string id = base::GenerateGUID();
  return BuildConfirmation(id, creative_instance_id, type);
}

}  // namespace ads

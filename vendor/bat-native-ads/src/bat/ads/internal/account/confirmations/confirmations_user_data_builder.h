/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_USER_DATA_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_USER_DATA_BUILDER_H_

#include <string>

#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/user_data/user_data_builder_interface.h"

namespace ads {

class ConfirmationsUserDataBuilder final : public UserDataBuilderInterface {
 public:
  ConfirmationsUserDataBuilder(const base::Time time,
                               const std::string& creative_instance_id,
                               const ConfirmationType& confirmation_type);

  ~ConfirmationsUserDataBuilder() override;

  void Build(UserDataBuilderCallback callback) const override;

 private:
  ConfirmationsUserDataBuilder(const ConfirmationsUserDataBuilder&) = delete;
  ConfirmationsUserDataBuilder& operator=(const ConfirmationsUserDataBuilder&) =
      delete;

  base::Time time_;
  std::string creative_instance_id_;
  ConfirmationType confirmation_type_ = ConfirmationType::kUndefined;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_USER_DATA_BUILDER_H_

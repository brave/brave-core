// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_consent_data.h"

namespace psst {

PsstConsentData::PsstConsentData(const std::string& user_id,
                                 const url::Origin& origin,
                                 base::Value::List request_infos,
                                 const int script_version,
                                 ConsentCallback apply_changes_callback)
    : user_id(user_id),
      origin(origin),
      request_infos(std::move(request_infos)),
      script_version(script_version),
      apply_changes_callback(std::move(apply_changes_callback)) {}
PsstConsentData::~PsstConsentData() = default;

PsstConsentData::PsstConsentData(PsstConsentData&&) noexcept = default;
PsstConsentData& PsstConsentData::operator=(PsstConsentData&&) noexcept =
    default;

}  // namespace psst

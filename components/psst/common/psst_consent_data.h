// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_COMMON_PSST_CONSENT_DATA_H_
#define BRAVE_COMPONENTS_PSST_COMMON_PSST_CONSENT_DATA_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/values.h"

namespace psst {

using ConsentCallback =
    base::OnceCallback<void(const std::vector<std::string>& disabled_checks)>;

struct PsstConsentData {
  PsstConsentData(const std::string& user_id,
                  const std::string& site_name,
                  base::Value::List request_infos,
                  const int script_version,
                  ConsentCallback apply_changes_callback);
  ~PsstConsentData();

  PsstConsentData(PsstConsentData&&) noexcept;
  PsstConsentData& operator=(PsstConsentData&&) noexcept;

  PsstConsentData(const PsstConsentData&) = delete;
  PsstConsentData& operator=(const PsstConsentData&) = delete;

  std::string user_id;
  std::string site_name;
  base::Value::List request_infos;
  int script_version;
  ConsentCallback apply_changes_callback;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_COMMON_PSST_CONSENT_DATA_H_

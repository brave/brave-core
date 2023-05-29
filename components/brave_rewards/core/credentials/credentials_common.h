/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_COMMON_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_COMMON_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/credentials/credentials.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal::credential {

class CredentialsCommon {
 public:
  void GetBlindedCreds(const CredentialsTrigger& trigger,
                       ResultCallback callback);

  void SaveUnblindedCreds(
      uint64_t expires_at,
      double token_value,
      const mojom::CredsBatch& creds,
      const std::vector<std::string>& unblinded_encoded_creds,
      const CredentialsTrigger& trigger,
      ResultCallback callback);

 private:
  void BlindedCredsSaved(ResultCallback callback, mojom::Result result);

  void OnSaveUnblindedCreds(ResultCallback callback,
                            const CredentialsTrigger& trigger,
                            mojom::Result result);
};

}  // namespace brave_rewards::internal::credential

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_COMMON_H_

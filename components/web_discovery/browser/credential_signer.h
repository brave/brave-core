/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_SIGNER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_SIGNER_H_

#include <optional>
#include <vector>

#include "base/functional/callback.h"

namespace web_discovery {

class CredentialSigner {
 public:
  using SignCallback =
      base::OnceCallback<void(std::optional<std::vector<const uint8_t>>)>;
  virtual ~CredentialSigner();

  virtual bool CredentialExistsForToday() = 0;
  virtual bool Sign(std::vector<const uint8_t> msg,
                    std::vector<const uint8_t> basename,
                    SignCallback callback) = 0;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_SIGNER_H_

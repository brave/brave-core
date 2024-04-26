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

  // Returns true is a credential is available for the current date.
  // The caller can expect future calls to `Sign` to succeed, if made today.
  virtual bool CredentialExistsForToday() = 0;

  // Signs a message for a given basename. The server has the ability
  // to check whether two messages with the same basename were signed
  // with the same credential without revealing the credential used,
  // preventing Sybil attacks.
  // See signature_basename.h/cc for more information on how the basename
  // should be generated.
  virtual bool Sign(std::vector<const uint8_t> msg,
                    std::vector<const uint8_t> basename,
                    SignCallback callback) = 0;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_CREDENTIAL_SIGNER_H_

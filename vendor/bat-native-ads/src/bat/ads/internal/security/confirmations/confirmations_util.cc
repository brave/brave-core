/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/security/confirmations/confirmations_util.h"

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"

namespace ads {
namespace security {

using challenge_bypass_ristretto::UnblindedToken;
using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;

bool Verify(const ConfirmationInfo& confirmation) {
  std::string credential;
  base::Base64Decode(confirmation.credential, &credential);

  absl::optional<base::Value> value = base::JSONReader::Read(credential);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  const std::string* signature = dictionary->FindStringKey("signature");
  if (!signature) {
    return false;
  }

  VerificationSignature verification_signature =
      VerificationSignature::decode_base64(*signature);
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return false;
  }

  UnblindedToken unblinded_token = confirmation.unblinded_token.value;
  VerificationKey verification_key = unblinded_token.derive_verification_key();
  if (privacy::ExceptionOccurred()) {
    NOTREACHED();
    return false;
  }

  const std::string payload = CreateConfirmationRequestDTO(confirmation);

  return verification_key.verify(verification_signature, payload);
}

}  // namespace security
}  // namespace ads

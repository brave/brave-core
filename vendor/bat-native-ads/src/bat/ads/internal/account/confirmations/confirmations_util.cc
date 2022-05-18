/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmations_util.h"

#include <string>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/values.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace security {

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

  const privacy::cbr::VerificationSignature verification_signature =
      privacy::cbr::VerificationSignature(*signature);
  if (!verification_signature.has_value()) {
    NOTREACHED();
    return false;
  }

  const privacy::cbr::UnblindedToken& unblinded_token =
      confirmation.unblinded_token.value;
  const absl::optional<privacy::cbr::VerificationKey>
      verification_key_optional = unblinded_token.DeriveVerificationKey();
  if (!verification_key_optional) {
    NOTREACHED();
    return false;
  }
  privacy::cbr::VerificationKey verification_key =
      verification_key_optional.value();

  const std::string payload = CreateConfirmationRequestDTO(confirmation);

  return verification_key.Verify(verification_signature, payload);
}

}  // namespace security
}  // namespace ads

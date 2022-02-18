/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<ledger::mojom::VgBodyTokenDataView,
                  sync_pb::VgBodySpecifics::Token>::
    Read(ledger::mojom::VgBodyTokenDataView data,
         sync_pb::VgBodySpecifics::Token* out) {
  std::string token_value;
  if (!data.ReadTokenValue(&token_value)) {
    return false;
  }

  out->set_token_id(data.token_id());
  out->set_token_value(std::move(token_value));
  out->set_value(data.value());
  out->set_expires_at(data.expires_at());

  return true;
}

// static
bool StructTraits<ledger::mojom::VgBodyDataView, sync_pb::VgBodySpecifics>::
    Read(ledger::mojom::VgBodyDataView data, sync_pb::VgBodySpecifics* out) {
  std::string creds_id;
  std::string creds;
  std::string blinded_creds;
  std::string signed_creds;
  std::string public_key;
  std::string batch_proof;

  if (!data.ReadCredsId(&creds_id) || !data.ReadCreds(&creds) ||
      !data.ReadBlindedCreds(&blinded_creds) ||
      !data.ReadSignedCreds(&signed_creds) ||
      !data.ReadPublicKey(&public_key) || !data.ReadBatchProof(&batch_proof) ||
      !data.ReadTokens(out->mutable_tokens())) {
    return false;
  }

  out->set_creds_id(std::move(creds_id));
  out->set_trigger_type(static_cast<std::int32_t>(data.trigger_type()));
  out->set_creds(std::move(creds));
  out->set_blinded_creds(std::move(blinded_creds));
  out->set_signed_creds(std::move(signed_creds));
  out->set_public_key(std::move(public_key));
  out->set_batch_proof(std::move(batch_proof));
  out->set_status(static_cast<std::int32_t>(data.status()));

  return true;
}

// static
bool StructTraits<ledger::mojom::VgSpendStatusDataView,
                  sync_pb::VgSpendStatusSpecifics>::
    Read(ledger::mojom::VgSpendStatusDataView data,
         sync_pb::VgSpendStatusSpecifics* out) {
  out->set_token_id(data.token_id());
  out->set_redeemed_at(data.redeemed_at());
  out->set_redeem_type(static_cast<std::int32_t>(data.redeem_type()));

  return true;
}

}  // namespace mojo

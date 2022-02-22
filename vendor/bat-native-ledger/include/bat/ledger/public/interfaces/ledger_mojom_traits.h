/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_MOJOM_TRAITS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_MOJOM_TRAITS_H_

#include "brave/components/sync/protocol/vg_specifics.pb.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom-shared.h"
#include "mojo/public/cpp/bindings/array_traits_protobuf.h"

namespace mojo {

template <>
struct StructTraits<ledger::mojom::VgBodyTokenDataView,
                    sync_pb::VgBodySpecifics::Token> {
  static std::uint64_t token_id(const sync_pb::VgBodySpecifics::Token& token) {
    return token.token_id();
  }

  static const std::string& token_value(
      const sync_pb::VgBodySpecifics::Token& token) {
    return token.token_value();
  }

  static double value(const sync_pb::VgBodySpecifics::Token& token) {
    return token.value();
  }

  static std::uint64_t expires_at(
      const sync_pb::VgBodySpecifics::Token& token) {
    return token.expires_at();
  }

  static bool Read(ledger::mojom::VgBodyTokenDataView data,
                   sync_pb::VgBodySpecifics::Token* out);
};

template <>
struct StructTraits<ledger::mojom::VgBodyDataView, sync_pb::VgBodySpecifics> {
  static const std::string& creds_id(const sync_pb::VgBodySpecifics& vg_body) {
    return vg_body.creds_id();
  }

  static ledger::mojom::CredsBatchType trigger_type(
      const sync_pb::VgBodySpecifics& vg_body) {
    return static_cast<ledger::mojom::CredsBatchType>(vg_body.trigger_type());
  }

  static const std::string& creds(const sync_pb::VgBodySpecifics& vg_body) {
    return vg_body.creds();
  }

  static const std::string& blinded_creds(
      const sync_pb::VgBodySpecifics& vg_body) {
    return vg_body.blinded_creds();
  }

  static const std::string& signed_creds(
      const sync_pb::VgBodySpecifics& vg_body) {
    return vg_body.signed_creds();
  }

  static const std::string& public_key(
      const sync_pb::VgBodySpecifics& vg_body) {
    return vg_body.public_key();
  }

  static const std::string& batch_proof(
      const sync_pb::VgBodySpecifics& vg_body) {
    return vg_body.batch_proof();
  }

  static ledger::mojom::CredsBatchStatus status(
      const sync_pb::VgBodySpecifics& vg_body) {
    return static_cast<ledger::mojom::CredsBatchStatus>(vg_body.status());
  }

  static const ::google::protobuf::RepeatedPtrField<
      sync_pb::VgBodySpecifics::Token>&
  tokens(const sync_pb::VgBodySpecifics& vg_body) {
    return vg_body.tokens();
  }

  static bool Read(ledger::mojom::VgBodyDataView data,
                   sync_pb::VgBodySpecifics* out);
};

template <>
struct StructTraits<ledger::mojom::VgSpendStatusDataView,
                    sync_pb::VgSpendStatusSpecifics> {
  static std::uint64_t token_id(
      const sync_pb::VgSpendStatusSpecifics& vg_spend_status) {
    return vg_spend_status.token_id();
  }

  static std::uint64_t redeemed_at(
      const sync_pb::VgSpendStatusSpecifics& vg_spend_status) {
    return vg_spend_status.redeemed_at();
  }

  static ledger::mojom::RewardsType redeem_type(
      const sync_pb::VgSpendStatusSpecifics& vg_spend_status) {
    return static_cast<ledger::mojom::RewardsType>(
        vg_spend_status.redeem_type());
  }

  static bool Read(ledger::mojom::VgSpendStatusDataView data,
                   sync_pb::VgSpendStatusSpecifics* out);
};

}  // namespace mojo

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_MOJOM_TRAITS_H_

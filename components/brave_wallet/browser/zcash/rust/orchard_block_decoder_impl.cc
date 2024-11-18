/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_block_decoder_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

OrchardBlockDecoderImpl::OrchardBlockDecoderImpl(const OrchardFullViewKey& fvk)
    : full_view_key_(fvk) {}

OrchardBlockDecoderImpl::~OrchardBlockDecoderImpl() = default;

std::optional<std::vector<::brave_wallet::OrchardNote>>
OrchardBlockDecoderImpl::ScanBlock(
    const ::brave_wallet::zcash::mojom::CompactBlockPtr& block) {
  std::vector<OrchardNote> result;
  for (const auto& tx : block->vtx) {
    ::rust::Vec<orchard::OrchardCompactAction> orchard_actions;
    for (const auto& orchard_action : tx->orchard_actions) {
      orchard::OrchardCompactAction orchard_compact_action;

      if (orchard_action->nullifier.size() != kOrchardNullifierSize ||
          orchard_action->cmx.size() != kOrchardCmxSize ||
          orchard_action->ephemeral_key.size() != kOrchardEphemeralKeySize ||
          orchard_action->ciphertext.size() != kOrchardCipherTextSize) {
        return std::nullopt;
      }

      base::ranges::copy(orchard_action->nullifier,
                         orchard_compact_action.nullifier.begin());
      base::ranges::copy(orchard_action->cmx,
                         orchard_compact_action.cmx.begin());
      base::ranges::copy(orchard_action->ephemeral_key,
                         orchard_compact_action.ephemeral_key.begin());
      base::ranges::copy(orchard_action->ciphertext,
                         orchard_compact_action.enc_cipher_text.begin());

      orchard_actions.emplace_back(std::move(orchard_compact_action));
    }

    ::rust::Box<::brave_wallet::orchard::BatchOrchardDecodeBundleResult>
        decode_result = ::brave_wallet::orchard::batch_decode(
            full_view_key_, std::move(orchard_actions));

    if (decode_result->is_ok()) {
      ::rust::Box<::brave_wallet::orchard::BatchOrchardDecodeBundle>
          result_bundle = decode_result->unwrap();
      for (size_t i = 0; i < result_bundle->size(); i++) {
        result.emplace_back(
            OrchardNote({{},
                         block->height,
                         result_bundle->note_nullifier(full_view_key_, i),
                         result_bundle->note_value(i),
                         0,
                         {},
                         {}}));
      }
    } else {
      return std::nullopt;
    }
  }
  return result;
}

// static
std::unique_ptr<OrchardBlockDecoder> OrchardBlockDecoder::FromFullViewKey(
    const OrchardFullViewKey& fvk) {
  return base::WrapUnique<OrchardBlockDecoder>(
      new OrchardBlockDecoderImpl(fvk));
}

}  // namespace brave_wallet::orchard

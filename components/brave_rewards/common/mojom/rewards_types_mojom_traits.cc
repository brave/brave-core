/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/mojom/rewards_types_mojom_traits.h"

#include <utility>

#include "brave/components/brave_rewards/common/mojom/rewards_types.mojom.h"

namespace mojo {

// static
brave_rewards::mojom::ConnectExternalWalletValuePtr UnionTraits<
    brave_rewards::mojom::ConnectExternalWalletResultDataView,
    base::expected<void, brave_rewards::mojom::ConnectExternalWalletError>>::
    value(
        const base::expected<void,
                             brave_rewards::mojom::ConnectExternalWalletError>&
            result) {
  DCHECK(result.has_value());
  return brave_rewards::mojom::ConnectExternalWalletValue::New();
}

// static
brave_rewards::mojom::ConnectExternalWalletError UnionTraits<
    brave_rewards::mojom::ConnectExternalWalletResultDataView,
    base::expected<void, brave_rewards::mojom::ConnectExternalWalletError>>::
    error(
        const base::expected<void,
                             brave_rewards::mojom::ConnectExternalWalletError>&
            result) {
  DCHECK(!result.has_value());
  return result.error();
}

// static
brave_rewards::mojom::ConnectExternalWalletResultDataView::Tag UnionTraits<
    brave_rewards::mojom::ConnectExternalWalletResultDataView,
    base::expected<void, brave_rewards::mojom::ConnectExternalWalletError>>::
    GetTag(
        const base::expected<void,
                             brave_rewards::mojom::ConnectExternalWalletError>&
            result) {
  return result.has_value()
             ? brave_rewards::mojom::ConnectExternalWalletResultDataView::Tag::
                   kValue
             : brave_rewards::mojom::ConnectExternalWalletResultDataView::Tag::
                   kError;
}

// static
bool UnionTraits<
    brave_rewards::mojom::ConnectExternalWalletResultDataView,
    base::expected<void, brave_rewards::mojom::ConnectExternalWalletError>>::
    Read(brave_rewards::mojom::ConnectExternalWalletResultDataView data,
         base::expected<void, brave_rewards::mojom::ConnectExternalWalletError>*
             out) {
  switch (data.tag()) {
    case brave_rewards::mojom::ConnectExternalWalletResultDataView::Tag::kValue:
      *out = {};
      return true;
    case brave_rewards::mojom::ConnectExternalWalletResultDataView::Tag::kError:
      brave_rewards::mojom::ConnectExternalWalletError error;
      if (data.ReadError(&error)) {
        *out = base::unexpected(error);
        return true;
      }

      break;
  }

  return false;
}

// static
brave_rewards::mojom::FetchBalanceValuePtr
UnionTraits<brave_rewards::mojom::FetchBalanceResultDataView,
            base::expected<brave_rewards::mojom::BalancePtr,
                           brave_rewards::mojom::FetchBalanceError>>::
    value(
        const base::expected<brave_rewards::mojom::BalancePtr,
                             brave_rewards::mojom::FetchBalanceError>& result) {
  DCHECK(result.has_value());
  return brave_rewards::mojom::FetchBalanceValue::New(result.value()->Clone());
}

// static
brave_rewards::mojom::FetchBalanceError
UnionTraits<brave_rewards::mojom::FetchBalanceResultDataView,
            base::expected<brave_rewards::mojom::BalancePtr,
                           brave_rewards::mojom::FetchBalanceError>>::
    error(
        const base::expected<brave_rewards::mojom::BalancePtr,
                             brave_rewards::mojom::FetchBalanceError>& result) {
  DCHECK(!result.has_value());
  return result.error();
}

// static
brave_rewards::mojom::FetchBalanceResultDataView::Tag
UnionTraits<brave_rewards::mojom::FetchBalanceResultDataView,
            base::expected<brave_rewards::mojom::BalancePtr,
                           brave_rewards::mojom::FetchBalanceError>>::
    GetTag(
        const base::expected<brave_rewards::mojom::BalancePtr,
                             brave_rewards::mojom::FetchBalanceError>& result) {
  return result.has_value()
             ? brave_rewards::mojom::FetchBalanceResultDataView::Tag::kValue
             : brave_rewards::mojom::FetchBalanceResultDataView::Tag::kError;
}

// static
bool UnionTraits<brave_rewards::mojom::FetchBalanceResultDataView,
                 base::expected<brave_rewards::mojom::BalancePtr,
                                brave_rewards::mojom::FetchBalanceError>>::
    Read(brave_rewards::mojom::FetchBalanceResultDataView data,
         base::expected<brave_rewards::mojom::BalancePtr,
                        brave_rewards::mojom::FetchBalanceError>* out) {
  switch (data.tag()) {
    case brave_rewards::mojom::FetchBalanceResultDataView::Tag::kValue: {
      brave_rewards::mojom::FetchBalanceValuePtr value;
      if (data.ReadValue(&value)) {
        *out = std::move(value->balance);
        return true;
      }

      break;
    }
    case brave_rewards::mojom::FetchBalanceResultDataView::Tag::kError: {
      brave_rewards::mojom::FetchBalanceError error;
      if (data.ReadError(&error)) {
        *out = base::unexpected(error);
        return true;
      }

      break;
    }
  }

  return false;
}

// static
brave_rewards::mojom::GetExternalWalletValuePtr
UnionTraits<brave_rewards::mojom::GetExternalWalletResultDataView,
            base::expected<brave_rewards::mojom::ExternalWalletPtr,
                           brave_rewards::mojom::GetExternalWalletError>>::
    value(const base::expected<brave_rewards::mojom::ExternalWalletPtr,
                               brave_rewards::mojom::GetExternalWalletError>&
              result) {
  DCHECK(result.has_value());
  return brave_rewards::mojom::GetExternalWalletValue::New(
      result.value()->Clone());
}

// static
brave_rewards::mojom::GetExternalWalletError
UnionTraits<brave_rewards::mojom::GetExternalWalletResultDataView,
            base::expected<brave_rewards::mojom::ExternalWalletPtr,
                           brave_rewards::mojom::GetExternalWalletError>>::
    error(const base::expected<brave_rewards::mojom::ExternalWalletPtr,
                               brave_rewards::mojom::GetExternalWalletError>&
              result) {
  DCHECK(!result.has_value());
  return result.error();
}

// static
brave_rewards::mojom::GetExternalWalletResultDataView::Tag
UnionTraits<brave_rewards::mojom::GetExternalWalletResultDataView,
            base::expected<brave_rewards::mojom::ExternalWalletPtr,
                           brave_rewards::mojom::GetExternalWalletError>>::
    GetTag(const base::expected<brave_rewards::mojom::ExternalWalletPtr,
                                brave_rewards::mojom::GetExternalWalletError>&
               result) {
  return result.has_value() ? brave_rewards::mojom::
                                  GetExternalWalletResultDataView::Tag::kValue
                            : brave_rewards::mojom::
                                  GetExternalWalletResultDataView::Tag::kError;
}

// static
bool UnionTraits<brave_rewards::mojom::GetExternalWalletResultDataView,
                 base::expected<brave_rewards::mojom::ExternalWalletPtr,
                                brave_rewards::mojom::GetExternalWalletError>>::
    Read(brave_rewards::mojom::GetExternalWalletResultDataView data,
         base::expected<brave_rewards::mojom::ExternalWalletPtr,
                        brave_rewards::mojom::GetExternalWalletError>* out) {
  switch (data.tag()) {
    case brave_rewards::mojom::GetExternalWalletResultDataView::Tag::kValue: {
      brave_rewards::mojom::GetExternalWalletValuePtr value;
      if (data.ReadValue(&value)) {
        *out = std::move(value->wallet);
        return true;
      }

      break;
    }
    case brave_rewards::mojom::GetExternalWalletResultDataView::Tag::kError: {
      brave_rewards::mojom::GetExternalWalletError error;
      if (data.ReadError(&error)) {
        *out = base::unexpected(error);
        return true;
      }

      break;
    }
  }

  return false;
}

}  // namespace mojo

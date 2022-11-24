/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types_mojom_traits.h"

#include <utility>

#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom.h"

namespace mojo {

// static
ledger::mojom::GetExternalWalletValuePtr
UnionTraits<ledger::mojom::GetExternalWalletResultDataView,
            base::expected<ledger::mojom::ExternalWalletPtr,
                           ledger::mojom::GetExternalWalletError>>::
    value(const base::expected<ledger::mojom::ExternalWalletPtr,
                               ledger::mojom::GetExternalWalletError>& result) {
  DCHECK(result.has_value());
  return ledger::mojom::GetExternalWalletValue::New(result.value()->Clone());
}

// static
ledger::mojom::GetExternalWalletError
UnionTraits<ledger::mojom::GetExternalWalletResultDataView,
            base::expected<ledger::mojom::ExternalWalletPtr,
                           ledger::mojom::GetExternalWalletError>>::
    error(const base::expected<ledger::mojom::ExternalWalletPtr,
                               ledger::mojom::GetExternalWalletError>& result) {
  DCHECK(!result.has_value());
  return result.error();
}

// static
ledger::mojom::GetExternalWalletResultDataView::Tag
UnionTraits<ledger::mojom::GetExternalWalletResultDataView,
            base::expected<ledger::mojom::ExternalWalletPtr,
                           ledger::mojom::GetExternalWalletError>>::
    GetTag(
        const base::expected<ledger::mojom::ExternalWalletPtr,
                             ledger::mojom::GetExternalWalletError>& result) {
  return result.has_value()
             ? ledger::mojom::GetExternalWalletResultDataView::Tag::kValue
             : ledger::mojom::GetExternalWalletResultDataView::Tag::kError;
}

// static
bool UnionTraits<ledger::mojom::GetExternalWalletResultDataView,
                 base::expected<ledger::mojom::ExternalWalletPtr,
                                ledger::mojom::GetExternalWalletError>>::
    Read(ledger::mojom::GetExternalWalletResultDataView data,
         base::expected<ledger::mojom::ExternalWalletPtr,
                        ledger::mojom::GetExternalWalletError>* out) {
  switch (data.tag()) {
    case ledger::mojom::GetExternalWalletResultDataView::Tag::kValue: {
      ledger::mojom::GetExternalWalletValuePtr value;
      if (data.ReadValue(&value)) {
        *out = std::move(value->wallet);
        return true;
      }

      break;
    }
    case ledger::mojom::GetExternalWalletResultDataView::Tag::kError: {
      ledger::mojom::GetExternalWalletError error;
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
ledger::mojom::ConnectExternalWalletValuePtr
UnionTraits<ledger::mojom::ConnectExternalWalletResultDataView,
            base::expected<void, ledger::mojom::ConnectExternalWalletError>>::
    value(const base::expected<void, ledger::mojom::ConnectExternalWalletError>&
              result) {
  DCHECK(result.has_value());
  return ledger::mojom::ConnectExternalWalletValue::New();
}

// static
ledger::mojom::ConnectExternalWalletError
UnionTraits<ledger::mojom::ConnectExternalWalletResultDataView,
            base::expected<void, ledger::mojom::ConnectExternalWalletError>>::
    error(const base::expected<void, ledger::mojom::ConnectExternalWalletError>&
              result) {
  DCHECK(!result.has_value());
  return result.error();
}

// static
ledger::mojom::ConnectExternalWalletResultDataView::Tag
UnionTraits<ledger::mojom::ConnectExternalWalletResultDataView,
            base::expected<void, ledger::mojom::ConnectExternalWalletError>>::
    GetTag(
        const base::expected<void, ledger::mojom::ConnectExternalWalletError>&
            result) {
  return result.has_value()
             ? ledger::mojom::ConnectExternalWalletResultDataView::Tag::kValue
             : ledger::mojom::ConnectExternalWalletResultDataView::Tag::kError;
}

// static
bool UnionTraits<
    ledger::mojom::ConnectExternalWalletResultDataView,
    base::expected<void, ledger::mojom::ConnectExternalWalletError>>::
    Read(ledger::mojom::ConnectExternalWalletResultDataView data,
         base::expected<void, ledger::mojom::ConnectExternalWalletError>* out) {
  switch (data.tag()) {
    case ledger::mojom::ConnectExternalWalletResultDataView::Tag::kValue:
      *out = {};
      return true;
    case ledger::mojom::ConnectExternalWalletResultDataView::Tag::kError:
      ledger::mojom::ConnectExternalWalletError error;
      if (data.ReadError(&error)) {
        *out = base::unexpected(error);
        return true;
      }

      break;
  }

  return false;
}

}  // namespace mojo

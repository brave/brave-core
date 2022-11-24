/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_TYPES_MOJOM_TRAITS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_TYPES_MOJOM_TRAITS_H_

#include "base/types/expected.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom-forward.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_types.mojom-shared.h"
#include "mojo/public/cpp/bindings/union_traits.h"

namespace mojo {

template <>
struct UnionTraits<ledger::mojom::GetExternalWalletResultDataView,
                   base::expected<ledger::mojom::ExternalWalletPtr,
                                  ledger::mojom::GetExternalWalletError>> {
  static ledger::mojom::GetExternalWalletValuePtr value(
      const base::expected<ledger::mojom::ExternalWalletPtr,
                           ledger::mojom::GetExternalWalletError>& result);

  static ledger::mojom::GetExternalWalletError error(
      const base::expected<ledger::mojom::ExternalWalletPtr,
                           ledger::mojom::GetExternalWalletError>& result);

  static ledger::mojom::GetExternalWalletResultDataView::Tag GetTag(
      const base::expected<ledger::mojom::ExternalWalletPtr,
                           ledger::mojom::GetExternalWalletError>& result);

  static bool Read(ledger::mojom::GetExternalWalletResultDataView data,
                   base::expected<ledger::mojom::ExternalWalletPtr,
                                  ledger::mojom::GetExternalWalletError>* out);
};

template <>
struct UnionTraits<
    ledger::mojom::ConnectExternalWalletResultDataView,
    base::expected<void, ledger::mojom::ConnectExternalWalletError>> {
  static ledger::mojom::ConnectExternalWalletValuePtr value(
      const base::expected<void, ledger::mojom::ConnectExternalWalletError>&
          result);

  static ledger::mojom::ConnectExternalWalletError error(
      const base::expected<void, ledger::mojom::ConnectExternalWalletError>&
          result);

  static ledger::mojom::ConnectExternalWalletResultDataView::Tag GetTag(
      const base::expected<void, ledger::mojom::ConnectExternalWalletError>&
          result);

  static bool Read(
      ledger::mojom::ConnectExternalWalletResultDataView data,
      base::expected<void, ledger::mojom::ConnectExternalWalletError>* out);
};

}  // namespace mojo

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_TYPES_MOJOM_TRAITS_H_

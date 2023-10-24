/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_MOJOM_REWARDS_MOJOM_TRAITS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_MOJOM_REWARDS_MOJOM_TRAITS_H_

#include "base/types/expected.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom-forward.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom-shared.h"
#include "mojo/public/cpp/bindings/union_traits.h"

namespace mojo {

template <>
struct UnionTraits<
    brave_rewards::mojom::ConnectExternalWalletResultDataView,
    base::expected<void, brave_rewards::mojom::ConnectExternalWalletError>> {
  static brave_rewards::mojom::ConnectExternalWalletValuePtr value(
      const base::expected<void,
                           brave_rewards::mojom::ConnectExternalWalletError>&
          result);

  static brave_rewards::mojom::ConnectExternalWalletError error(
      const base::expected<void,
                           brave_rewards::mojom::ConnectExternalWalletError>&
          result);

  static brave_rewards::mojom::ConnectExternalWalletResultDataView::Tag GetTag(
      const base::expected<void,
                           brave_rewards::mojom::ConnectExternalWalletError>&
          result);

  static bool Read(
      brave_rewards::mojom::ConnectExternalWalletResultDataView data,
      base::expected<void, brave_rewards::mojom::ConnectExternalWalletError>*
          out);
};

template <>
struct UnionTraits<brave_rewards::mojom::FetchBalanceResultDataView,
                   base::expected<brave_rewards::mojom::BalancePtr,
                                  brave_rewards::mojom::FetchBalanceError>> {
  static brave_rewards::mojom::FetchBalanceValuePtr value(
      const base::expected<brave_rewards::mojom::BalancePtr,
                           brave_rewards::mojom::FetchBalanceError>& result);

  static brave_rewards::mojom::FetchBalanceError error(
      const base::expected<brave_rewards::mojom::BalancePtr,
                           brave_rewards::mojom::FetchBalanceError>& result);

  static brave_rewards::mojom::FetchBalanceResultDataView::Tag GetTag(
      const base::expected<brave_rewards::mojom::BalancePtr,
                           brave_rewards::mojom::FetchBalanceError>& result);

  static bool Read(
      brave_rewards::mojom::FetchBalanceResultDataView data,
      base::expected<brave_rewards::mojom::BalancePtr,
                     brave_rewards::mojom::FetchBalanceError>* out);
};

template <>
struct UnionTraits<
    brave_rewards::mojom::GetExternalWalletResultDataView,
    base::expected<brave_rewards::mojom::ExternalWalletPtr,
                   brave_rewards::mojom::GetExternalWalletError>> {
  static brave_rewards::mojom::GetExternalWalletValuePtr value(
      const base::expected<brave_rewards::mojom::ExternalWalletPtr,
                           brave_rewards::mojom::GetExternalWalletError>&
          result);

  static brave_rewards::mojom::GetExternalWalletError error(
      const base::expected<brave_rewards::mojom::ExternalWalletPtr,
                           brave_rewards::mojom::GetExternalWalletError>&
          result);

  static brave_rewards::mojom::GetExternalWalletResultDataView::Tag GetTag(
      const base::expected<brave_rewards::mojom::ExternalWalletPtr,
                           brave_rewards::mojom::GetExternalWalletError>&
          result);

  static bool Read(
      brave_rewards::mojom::GetExternalWalletResultDataView data,
      base::expected<brave_rewards::mojom::ExternalWalletPtr,
                     brave_rewards::mojom::GetExternalWalletError>* out);
};

}  // namespace mojo

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_MOJOM_REWARDS_MOJOM_TRAITS_H_

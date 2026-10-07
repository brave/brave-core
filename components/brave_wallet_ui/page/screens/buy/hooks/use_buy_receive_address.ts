// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { skipToken } from '@reduxjs/toolkit/dist/query'

// Types
import { BraveWallet } from '../../../../constants/types'

// Hooks
import {
  useGetPolkadotAddressForNetworkQuery, //
} from '../../../../common/slices/api.slice'
import {
  useReceiveAddressQuery, //
} from '../../../../common/slices/api.slice.extra'

// Resolves the receive address to hand to Meld when funding an account.
// We use this because calls to generateReceiveAddress() should rightfully fail
// for Polkadot, whose addresses are parachain-dependent, so we can't use our
// normal useReceiveAddressQuery.
export const useBuyReceiveAddress = (
  accountId: BraveWallet.AccountId | undefined,
) => {
  const isDotAccount = accountId?.coin === BraveWallet.CoinType.DOT

  const {
    data: polkadotAssetHubAddress,
    isFetching: isFetchingPolkadotAddress,
  } = useGetPolkadotAddressForNetworkQuery(
    isDotAccount && accountId
      ? {
          accountId,
          chainId: BraveWallet.POLKADOT_MAINNET_ASSET_HUB,
        }
      : skipToken,
  )

  const { receiveAddress, isFetchingAddress } = useReceiveAddressQuery(
    isDotAccount ? undefined : accountId,
  )

  if (isDotAccount) {
    return {
      receiveAddress: polkadotAssetHubAddress ?? '',
      isFetchingAddress: isFetchingPolkadotAddress,
    }
  }

  return { receiveAddress, isFetchingAddress }
}

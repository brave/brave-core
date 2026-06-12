// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'

// Utils
import { isPolkadotTransaction } from '../../utils/tx-utils'

// Hooks
import {
  useGetAccountInfosRegistryQuery,
  useGetPolkadotAddressesForNetworkQuery,
} from '../slices/api.slice'
import {
  accountInfoEntityAdaptorInitialState, //
} from '../slices/entities/account-info.entity'

/**
 * Maps each of the user's Polkadot account uniqueKeys to its chain-correct
 * ss58 address for the given transaction's network.
 *
 * A parachain may use a different ss58 prefix than the keyring default that
 * `account.address` is encoded with. Re-encoding each account for the
 * transaction's chain lets the UI associate a recipient with one of the user's
 * own accounts even when the prefixes differ. Returns an empty map for
 * non-Polkadot transactions.
 */
export const usePolkadotTransactionAddresses = (
  transaction:
    | Pick<
        BraveWallet.TransactionInfo | SerializableTransactionInfo,
        'chainId' | 'txDataUnion' | 'fromAccountId'
      >
    | undefined,
): Record<string, string> => {
  const isDot = isPolkadotTransaction(transaction)

  const { data: accountInfosRegistry = accountInfoEntityAdaptorInitialState } =
    useGetAccountInfosRegistryQuery(isDot ? undefined : skipToken)

  // Only re-encode accounts that share the sender's keyring. A chain supports a
  // specific set of keyrings, and `getAddress` errors for an unsupported one —
  // which would reject the whole batched query. Accounts from another keyring
  // belong to a different network and can never be this chain's recipient.
  const polkadotAccountIds = React.useMemo(() => {
    if (!isDot || !transaction) {
      return []
    }
    const keyringId = transaction.fromAccountId.keyringId
    return accountInfosRegistry.ids
      .map((id) => accountInfosRegistry.entities[id])
      .filter(
        (account): account is BraveWallet.AccountInfo =>
          account?.accountId.coin === BraveWallet.CoinType.DOT
          && account.accountId.keyringId === keyringId,
      )
      .map((account) => account.accountId)
  }, [isDot, transaction, accountInfosRegistry])

  const { data: polkadotAddressesByUniqueKey = {} } =
    useGetPolkadotAddressesForNetworkQuery(
      isDot && transaction && polkadotAccountIds.length > 0
        ? { accountIds: polkadotAccountIds, chainId: transaction.chainId }
        : skipToken,
    )

  return polkadotAddressesByUniqueKey
}

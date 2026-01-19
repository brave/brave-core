// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityState } from '@reduxjs/toolkit'

// types
import {
  SerializableTransactionInfo,
  BraveWallet,
  TransactionInfo,
} from '../constants/types'
import { AccountInfoEntity } from '../common/slices/entities/account-info.entity'

// utils
import {
  NetworksRegistry,
  networkEntityAdapter,
} from '../common/slices/entities/network.entity'
import { makeNetworkAsset } from '../options/asset-options'
import { getAddressLabel, getAccountLabel } from './account-utils'
import Amount from './amount'
import { getCoinFromTxDataUnion } from './network-utils'
import {
  findTransactionToken,
  getTransactionApprovalTargetAddress,
  getTransactionToAddress,
  getTransactionIntent,
  parseSwapInfo,
} from './tx-utils'

export type SearchableTransaction = TransactionInfo & {
  approvalTarget: string
  approvalTargetLabel: string
  destinationToken?: BraveWallet.BlockchainToken
  erc721BlockchainToken?: BraveWallet.BlockchainToken
  intent: string
  nativeAsset?: BraveWallet.BlockchainToken
  recipient: string
  recipientLabel: string
  sourceToken?: BraveWallet.BlockchainToken
  senderAddress: string
  senderLabel: string
  token?: BraveWallet.BlockchainToken
}

export const makeSearchableTransaction = (
  tx: SerializableTransactionInfo,
  tokensList: BraveWallet.BlockchainToken[],
  networksRegistry: NetworksRegistry | undefined,
  accountInfosRegistry: EntityState<AccountInfoEntity>,
): SearchableTransaction => {
  const txNetwork =
    networksRegistry?.entities[
      networkEntityAdapter.selectId({
        chainId: tx.chainId,
        coin: getCoinFromTxDataUnion(tx.txDataUnion),
      })
    ]

  const nativeAsset = makeNetworkAsset(txNetwork)

  const token = findTransactionToken(tx, tokensList)

  const erc721BlockchainToken = [
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom,
  ].includes(tx.txType)
    ? token
    : undefined

  const { sourceToken, destinationToken } = parseSwapInfo({
    networksRegistry,
    tokensList,
    swapInfo: tx.swapInfo,
  })

  const approvalTarget = getTransactionApprovalTargetAddress(tx)
  const approvalTargetLabel = getAddressLabel(
    approvalTarget,
    accountInfosRegistry,
  )

  const senderAccount = tx.fromAccountId
  const senderAddress = senderAccount.address
  const senderLabel = getAccountLabel(senderAccount, accountInfosRegistry)
  const recipient = getTransactionToAddress(tx)
  const recipientLabel = getAddressLabel(recipient, accountInfosRegistry)

  const emptyAmount = new Amount('')

  const intent = getTransactionIntent({
    normalizedTransferredValue: '',
    tx,
    sourceAmount: emptyAmount,
    sourceToken,
    erc721TokenId: erc721BlockchainToken?.tokenId,
    destinationAmount: emptyAmount,
    destinationToken,
    token,
    transactionNetwork: txNetwork,
  })

  return {
    ...tx,
    approvalTarget,
    approvalTargetLabel,
    destinationToken,
    erc721BlockchainToken,
    intent,
    nativeAsset,
    recipient,
    recipientLabel,
    sourceToken,
    senderAddress,
    senderLabel,
    token,
  }
}

const findTokenBySearchValue = (
  searchValue: string,
  token?: BraveWallet.BlockchainToken,
) => {
  if (!token) {
    return false
  }

  return (
    token.name.toLowerCase().includes(searchValue)
    || token.symbol.toLowerCase().includes(searchValue)
    || token.contractAddress.toLowerCase().includes(searchValue)
  )
}

export const filterTransactionsBySearchValue = (
  txs: SearchableTransaction[],
  lowerCaseSearchValue: string,
) => {
  return txs.filter((tx) => {
    return (
      // Tokens
      findTokenBySearchValue(lowerCaseSearchValue, tx.token)
      // Swap source token
      || findTokenBySearchValue(lowerCaseSearchValue, tx.sourceToken)
      // Swap destination token
      || findTokenBySearchValue(lowerCaseSearchValue, tx.destinationToken)
      // ERC721 NFTs
      || findTokenBySearchValue(lowerCaseSearchValue, tx.erc721BlockchainToken)
      // Sender
      || tx.senderAddress.toLowerCase().includes(lowerCaseSearchValue)
      || tx.senderLabel.toLowerCase().includes(lowerCaseSearchValue)
      // Receiver
      || tx.recipient.toLowerCase().includes(lowerCaseSearchValue)
      || tx.recipientLabel.toLowerCase().includes(lowerCaseSearchValue)
      // Intent
      || tx.intent.toLowerCase().includes(lowerCaseSearchValue)
      // Hash
      || tx.txHash.toLowerCase().includes(lowerCaseSearchValue)
      // Approval Target
      || (tx.approvalTarget
        && tx.approvalTarget.toLowerCase().includes(lowerCaseSearchValue))
      || (tx.approvalTargetLabel
        && tx.approvalTargetLabel.toLowerCase().includes(lowerCaseSearchValue)) // Origin
      || tx.originInfo?.eTldPlusOne.toLowerCase().includes(lowerCaseSearchValue)
      || tx.originInfo?.originSpec.toLowerCase().includes(lowerCaseSearchValue)
    )
  })
}

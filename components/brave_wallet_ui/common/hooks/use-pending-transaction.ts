// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { create } from 'ethereum-blockies'

// actions
import * as WalletActions from '../actions/wallet_actions'

// utils
import Amount from '../../utils/amount'
import { findAccountName } from '../../utils/account-utils'
import { getLocale } from '../../../common/locale'
import { getNetworkFromTXDataUnion } from '../../utils/network-utils'
import { reduceAddress } from '../../utils/reduce-address'

// Custom Hooks
import { useTransactionParser } from './transaction-parser'
import usePricing from './pricing'
import useTokenInfo from './token'
import { useLib } from './useLib'

// types
import { WalletState, BraveWallet } from '../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType
} from '../constants/action_types'

export const usePendingTransactions = () => {
  // redux
  const dispatch = useDispatch()
  const {
    accounts,
    selectedNetwork,
    selectedPendingTransaction: transactionInfo,
    userVisibleTokensInfo: visibleTokens,
    transactionSpotPrices,
    gasEstimates,
    fullTokenList,
    pendingTransactions,
    defaultNetworks
  } = useSelector((state: { wallet: WalletState }) => state.wallet)
  const transactionGasEstimates = transactionInfo?.txDataUnion.ethTxData1559?.gasEstimation

  const transactionsNetwork = React.useMemo(() => {
    if (!transactionInfo) {
      return selectedNetwork
    }
    return getNetworkFromTXDataUnion(transactionInfo.txDataUnion, defaultNetworks, selectedNetwork)
  }, [defaultNetworks, transactionInfo, selectedNetwork])

  // custom hooks
  const { getBlockchainTokenInfo, getERC20Allowance } = useLib()
  const parseTransaction = useTransactionParser(transactionsNetwork)
  const { findAssetPrice } = usePricing(transactionSpotPrices)
  const {
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = useTokenInfo(getBlockchainTokenInfo, visibleTokens, fullTokenList, transactionsNetwork)

  // state
  const [suggestedMaxPriorityFeeChoices, setSuggestedMaxPriorityFeeChoices] = React.useState<string[]>([
    transactionGasEstimates?.slowMaxPriorityFeePerGas || '0',
    transactionGasEstimates?.avgMaxPriorityFeePerGas || '0',
    transactionGasEstimates?.fastMaxPriorityFeePerGas || '0'
  ])
  const [baseFeePerGas, setBaseFeePerGas] = React.useState<string>(transactionGasEstimates?.baseFeePerGas || '')
  const [currentTokenAllowance, setCurrentTokenAllowance] = React.useState<string>('')

  // computed state
  const transactionDetails = transactionInfo ? parseTransaction(transactionInfo) : undefined
  const transactionQueueNumber = pendingTransactions.findIndex(tx => tx.id === transactionInfo?.id) + 1
  const transactionsQueueLength = pendingTransactions.length

  const isERC20Approve = transactionInfo?.txType === BraveWallet.TransactionType.ERC20Approve
  const isERC721SafeTransferFrom = transactionInfo?.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  const isERC721TransferFrom = transactionInfo?.txType === BraveWallet.TransactionType.ERC721TransferFrom

  const isSolanaTransaction = transactionInfo?.txType && [
    BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
    BraveWallet.TransactionType.SolanaDappSignTransaction,
    BraveWallet.TransactionType.SolanaSPLTokenTransfer,
    BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
    BraveWallet.TransactionType.SolanaSystemTransfer
  ].includes(transactionInfo.txType)

  const isSolanaDappTransaction = transactionInfo?.txType && [
    BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
    BraveWallet.TransactionType.SolanaDappSignTransaction
  ].includes(transactionInfo.txType)

  const isAssociatedTokenAccountCreation =
    transactionInfo?.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation

  const isFilecoinTransaction =
    transactionInfo?.txType === BraveWallet.TransactionType.Other &&
    transactionInfo?.txDataUnion?.filTxData

  // methods
  const onEditAllowanceSave = React.useCallback((allowance: string) => {
    if (transactionInfo?.id && transactionDetails) {
      dispatch(WalletActions.updateUnapprovedTransactionSpendAllowance({
        txMetaId: transactionInfo.id,
        spenderAddress: transactionDetails.approvalTarget || '',
        allowance: new Amount(allowance)
          .multiplyByDecimals(transactionDetails.decimals)
          .toHex()
      }))
    }
  }, [transactionInfo?.id, transactionDetails])

  const updateUnapprovedTransactionNonce = React.useCallback((args: UpdateUnapprovedTransactionNonceType) => {
    dispatch(WalletActions.updateUnapprovedTransactionNonce(args))
  }, [])

  const queueNextTransaction = React.useCallback(() => dispatch(WalletActions.queueNextTransaction()), [])

  const rejectAllTransactions = React.useCallback(() => dispatch(WalletActions.rejectAllTransactions()), [])

  const updateUnapprovedTransactionGasFields = React.useCallback((payload: UpdateUnapprovedTransactionGasFieldsType) => {
    dispatch(WalletActions.updateUnapprovedTransactionGasFields(payload))
  }, [])

  // memos
  const fromOrb = React.useMemo(() => {
    return create({ seed: transactionDetails?.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails?.sender])

  const toOrb = React.useMemo(() => {
    return create({
      seed: transactionDetails?.recipient.toLowerCase(),
      size: 8,
      scale: 10
    }).toDataURL()
  }, [transactionDetails?.recipient])

  const fromAccountName = React.useMemo(() => {
    return findAccountName(accounts, transactionInfo?.fromAddress ?? '') ?? reduceAddress(transactionInfo?.fromAddress ?? '')
  }, [accounts, transactionInfo?.fromAddress])

  const transactionTitle = React.useMemo(
    (): string =>
      isSolanaDappTransaction
        ? getLocale('braveWalletApproveTransaction')
        : transactionDetails?.isSwap
          ? getLocale('braveWalletSwap')
          : getLocale('braveWalletSend')
    , [isSolanaDappTransaction, transactionDetails?.isSwap])

  const isConfirmButtonDisabled = React.useMemo(() => {
    if (!transactionDetails) {
      return true
    }

    return (
      !!transactionDetails?.sameAddressError ||
      !!transactionDetails?.contractAddressError ||
      transactionDetails?.insufficientFundsForGasError === undefined ||
      transactionDetails?.insufficientFundsError === undefined ||
      transactionDetails?.insufficientFundsForGasError ||
      transactionDetails?.insufficientFundsError ||
      !!transactionDetails?.missingGasLimitError
    )
  }, [transactionDetails])

  // effects
  React.useEffect(() => {
    const interval = setInterval(() => {
      if (transactionInfo) {
        dispatch(WalletActions.refreshGasEstimates(transactionInfo))
      }
    }, 15000)

    if (transactionInfo) {
      dispatch(WalletActions.refreshGasEstimates(transactionInfo))
    }

    return () => clearInterval(interval) // cleanup on component unmount
  }, [transactionInfo])

  React.useEffect(
    () => {
      setSuggestedMaxPriorityFeeChoices([
        gasEstimates?.slowMaxPriorityFeePerGas || '0',
        gasEstimates?.avgMaxPriorityFeePerGas || '0',
        gasEstimates?.fastMaxPriorityFeePerGas || '0'
      ])

      setBaseFeePerGas(gasEstimates?.baseFeePerGas || '0')
    },
    [gasEstimates]
  )

  React.useEffect(() => {
    if (transactionInfo?.txType !== BraveWallet.TransactionType.ERC20Approve) {
      return
    }

    if (!transactionDetails?.approvalTarget) {
      return
    }

    getERC20Allowance(
      transactionDetails.recipient,
      transactionDetails.sender,
      transactionDetails.approvalTarget
    ).then(result => {
      const allowance = new Amount(result)
        .divideByDecimals(transactionDetails.decimals)
        .format()
      setCurrentTokenAllowance(allowance)
    }).catch(e => console.error(e))
  }, [transactionInfo?.txType, transactionDetails, getERC20Allowance])

  React.useEffect(() => {
    if (
      transactionDetails?.recipient &&
      transactionInfo?.txType === BraveWallet.TransactionType.ERC20Approve
    ) {
      onFindTokenInfoByContractAddress(transactionDetails.recipient)
    }
  }, [transactionDetails?.recipient, transactionInfo?.txType])

  return {
    baseFeePerGas,
    currentTokenAllowance,
    findAssetPrice,
    foundTokenInfoByContractAddress,
    fromAccountName,
    fromAddress: transactionInfo?.fromAddress ?? '',
    fromOrb,
    isConfirmButtonDisabled,
    isERC20Approve,
    isERC721SafeTransferFrom,
    isERC721TransferFrom,
    isSolanaDappTransaction,
    isSolanaTransaction,
    isAssociatedTokenAccountCreation,
    isFilecoinTransaction,
    onEditAllowanceSave,
    queueNextTransaction,
    rejectAllTransactions,
    suggestedMaxPriorityFeeChoices,
    toOrb,
    transactionDetails,
    transactionQueueNumber,
    transactionsNetwork,
    transactionsQueueLength,
    transactionTitle,
    sendOptions: transactionInfo?.txDataUnion.solanaTxData?.sendOptions,
    updateUnapprovedTransactionGasFields,
    updateUnapprovedTransactionNonce
  }
}

// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { create } from 'ethereum-blockies'

// actions
import * as WalletActions from '../actions/wallet_actions'

// utils
import Amount from '../../utils/amount'
import { findAccountName } from '../../utils/account-utils'
import { getLocale } from '../../../common/locale'
import { getNetworkFromTXDataUnion } from '../../utils/network-utils'
import { reduceAddress } from '../../utils/reduce-address'
import { WalletSelectors } from '../selectors'

// Custom Hooks
import { useTransactionParser } from './transaction-parser'
import usePricing from './pricing'
import useTokenInfo from './token'
import { useLib } from './useLib'
import {
  useSafeWalletSelector,
  useUnsafeWalletSelector
} from './use-safe-selector'
import {
  useGetDefaultNetworksQuery,
  useGetSelectedChainQuery
} from '../slices/api.slice'

// Constants
import { BraveWallet } from '../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType
} from '../constants/action_types'
import { isSolanaTransaction, sortTransactionByDate } from '../../utils/tx-utils'
import { MAX_UINT256 } from '../constants/magics'

export const usePendingTransactions = () => {
  // redux
  const dispatch = useDispatch()
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const transactions = useUnsafeWalletSelector(WalletSelectors.transactions)
  const transactionInfo = useUnsafeWalletSelector(
    WalletSelectors.selectedPendingTransaction
  )
  const visibleTokens = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )
  const transactionSpotPrices = useUnsafeWalletSelector(
    WalletSelectors.transactionSpotPrices
  )
  const gasEstimates = useUnsafeWalletSelector(WalletSelectors.gasEstimates)
  const fullTokenList = useUnsafeWalletSelector(WalletSelectors.fullTokenList)
  const pendingTransactions = useUnsafeWalletSelector(
    WalletSelectors.pendingTransactions
  )
  const hasFeeEstimatesError = useSafeWalletSelector(
    WalletSelectors.hasFeeEstimatesError
  )

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: defaultNetworks = [] } = useGetDefaultNetworksQuery()


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
  const [erc20AllowanceResult, setERC20AllowanceResult] = React.useState<
    string | undefined
  >(undefined)

  // computed state
  const transactionDetails = transactionInfo ? parseTransaction(transactionInfo) : undefined
  const transactionQueueNumber = pendingTransactions.findIndex(tx => tx.id === transactionInfo?.id) + 1
  const transactionsQueueLength = pendingTransactions.length

  const isERC20Approve = transactionInfo?.txType === BraveWallet.TransactionType.ERC20Approve
  const isERC721SafeTransferFrom = transactionInfo?.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
  const isERC721TransferFrom = transactionInfo?.txType === BraveWallet.TransactionType.ERC721TransferFrom

  const isSolanaTxn = transactionInfo && isSolanaTransaction(transactionInfo)
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

  // List of all transactions that belong to the same group as the selected
  // pending transaction.
  const groupTransactions = React.useMemo(() =>
    transactionInfo?.groupId && transactionInfo?.fromAddress
      ? sortTransactionByDate(
          transactions[transactionInfo.fromAddress]
            .filter(txn => txn.groupId === transactionInfo.groupId))
      : [],
    [transactionInfo, transactions])

  const unconfirmedGroupTransactionIds = React.useMemo(() =>
    groupTransactions
      .filter(txn => txn.txStatus !== BraveWallet.TransactionStatus.Confirmed)
      .map(txn => txn.id),
    [groupTransactions])

  // Position of the selected pending transaction in the group, if exists.
  const selectedPendingTransactionGroupIndex = React.useMemo(() =>
    groupTransactions.findIndex(txn => transactionInfo?.id === txn.id),
    [groupTransactions, transactionInfo])

  // The selected pending transaction can only be approved if:
  //   - it does not belong to a transaction group
  //   - it is the first unconfirmed transaction in the group
  const canSelectedPendingTransactionBeApproved = React.useMemo(() =>
    unconfirmedGroupTransactionIds.findIndex(idx => transactionInfo?.id === idx) <= 0,
    [transactionInfo, unconfirmedGroupTransactionIds])

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

  const isLoadingGasFee =
    // FIL has gas info provided by txDataUnion
    !transactionDetails?.isFilecoinTransaction &&
    transactionDetails?.gasFee === ''

  const isConfirmButtonDisabled = React.useMemo(() => {
    if (hasFeeEstimatesError || isLoadingGasFee) {
      return true
    }

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
      !!transactionDetails?.missingGasLimitError ||
      !canSelectedPendingTransactionBeApproved
    )
  }, [transactionDetails, hasFeeEstimatesError, isLoadingGasFee])

  const {
    currentTokenAllowance,
    isCurrentAllowanceUnlimited
  } = React.useMemo(() => {
    if (!transactionDetails || erc20AllowanceResult === undefined) {
      return {
        currentTokenAllowance: undefined,
        isCurrentAllowanceUnlimited: false
      }
    }
    
    const currentTokenAllowance = new Amount(erc20AllowanceResult)
      .divideByDecimals(transactionDetails.decimals)
      .format()

    const isCurrentAllowanceUnlimited = erc20AllowanceResult === MAX_UINT256

    return {
      currentTokenAllowance,
      isCurrentAllowanceUnlimited
    }
  }, [erc20AllowanceResult])

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
    let subscribed = true
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
      subscribed && setERC20AllowanceResult(result)
    }).catch(e => console.error(e))

    // cleanup
    return () => {
      subscribed = false
    }
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
    isCurrentAllowanceUnlimited,
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
    isSolanaTransaction: isSolanaTxn,
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
    updateUnapprovedTransactionNonce,
    groupTransactions,
    selectedPendingTransactionGroupIndex,
    hasFeeEstimatesError,
    selectedPendingTransaction: transactionInfo,
    isLoadingGasFee
  }
}

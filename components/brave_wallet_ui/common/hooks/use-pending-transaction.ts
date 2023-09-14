// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { ThunkDispatch } from '@reduxjs/toolkit'
import { skipToken } from '@reduxjs/toolkit/query/react'

// actions
import { UIActions } from '../slices/ui.slice'
import { PanelActions } from '../../panel/actions'

// utils
import Amount from '../../utils/amount'
import { getPriceIdForToken } from '../../utils/api-utils'
import { isHardwareAccount } from '../../utils/account-utils'
import { getLocale } from '../../../common/locale'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'
import { UISelectors, WalletSelectors } from '../selectors'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  getTransactionGasFee,
  isSolanaTransaction,
  parseTransactionWithPrices,
  findTransactionToken,
  isEthereumTransaction
} from '../../utils/tx-utils'
import { makeNetworkAsset } from '../../options/asset-options'

// Custom Hooks
import useTokenInfo from './token'
import { useLib } from './useLib'
import { useAccountOrb, useAddressOrb } from './use-orb'
import {
  useSafeUISelector,
  useSafeWalletSelector,
} from './use-safe-selector'
import {
  useGetAccountInfosRegistryQuery,
  useGetAccountTokenCurrentBalanceQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetGasEstimation1559Query,
  useGetNetworkQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetTokenSpotPricesQuery,
  walletApi
} from '../slices/api.slice'
import {
  usePendingTransactionsQuery,
  useGetCombinedTokensListQuery,
  useAccountQuery
} from '../slices/api.slice.extra'
import {
  defaultQuerySubscriptionOptions,
  querySubscriptionOptions60s
} from '../slices/constants'

// Constants
import { BraveWallet } from '../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType
} from '../constants/action_types'
import { MAX_UINT256 } from '../constants/magics'

export const useSelectedPendingTransaction = () => {
  // redux
  const selectedPendingTransactionId = useSafeUISelector(
    UISelectors.selectedPendingTransactionId
  )

  // queries
  const { pendingTransactions } = usePendingTransactionsQuery({
    accountId: null,
    chainId: null,
    coinType: null
  })

  // computed
  const selectedPendingTransaction = !pendingTransactions.length
    ? undefined
    : pendingTransactions.find(
        (tx) => tx.id === selectedPendingTransactionId
      ) ?? pendingTransactions[0]

  // render
  return selectedPendingTransaction
}

export const usePendingTransactions = () => {
  // redux
  const dispatch = useDispatch<ThunkDispatch<any, any, any>>()
  const selectedPendingTransactionId = useSafeUISelector(
    UISelectors.selectedPendingTransactionId
  )
  const hasFeeEstimatesError = useSafeWalletSelector(
    WalletSelectors.hasFeeEstimatesError
  )

  // queries
  const { data: defaultFiat } = useGetDefaultFiatCurrencyQuery()
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const { pendingTransactions } = usePendingTransactionsQuery({
    accountId: null,
    chainId: null,
    coinType: null
  })
  const { data: accounts } = useGetAccountInfosRegistryQuery()

  const transactionInfo = React.useMemo(() => {
    if (!pendingTransactions.length) {
      return undefined
    }
    return (
      pendingTransactions.find(
        (tx) => tx.id === selectedPendingTransactionId
      ) ?? pendingTransactions[0]
    )
  }, [pendingTransactions, selectedPendingTransactionId])

  const txToken = findTransactionToken(transactionInfo, combinedTokensList)

  const txCoinType = transactionInfo
    ? getCoinFromTxDataUnion(transactionInfo.txDataUnion)
    : undefined

  const { data: transactionsNetwork } = useGetNetworkQuery(
    transactionInfo && txCoinType !== undefined
      ? {
          chainId: transactionInfo.chainId,
          coin: txCoinType
        }
      : skipToken
  )

  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(transactionsNetwork)
  }, [transactionsNetwork])

  const tokenPriceIds = React.useMemo(
    () =>
      [txToken, networkAsset]
        .filter((t): t is BraveWallet.BlockchainToken => Boolean(t))
        .map(getPriceIdForToken),
    [txToken, networkAsset]
  )

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length > 0 && defaultFiat
      ? { ids: tokenPriceIds, toCurrency: defaultFiat }
      : skipToken,
      querySubscriptionOptions60s
  )

  const { data: gasEstimates, isLoading: isLoadingGasEstimates } =
    useGetGasEstimation1559Query(
      transactionInfo && txCoinType === BraveWallet.CoinType.ETH
        ? transactionInfo.chainId
        : skipToken,
      defaultQuerySubscriptionOptions
    )

  const {
    data: solFeeEstimate,
    isLoading: isLoadingSolFeeEstimates = txCoinType ===
      BraveWallet.CoinType.SOL
  } = useGetSolanaEstimatedFeeQuery(
    txCoinType === BraveWallet.CoinType.SOL &&
      transactionInfo?.chainId &&
      transactionInfo?.id
      ? {
          chainId: transactionInfo.chainId,
          txId: transactionInfo.id
        }
      : skipToken,
    defaultQuerySubscriptionOptions
  )

  const { account: txAccount } = useAccountQuery(transactionInfo?.fromAccountId)

  // custom hooks
  const { getBlockchainTokenInfo, getERC20Allowance } = useLib()
  const { onFindTokenInfoByContractAddress, foundTokenInfoByContractAddress } =
    useTokenInfo(
      getBlockchainTokenInfo,
      combinedTokensList,
      transactionsNetwork
    )

  // state
  const [erc20AllowanceResult, setERC20AllowanceResult] = React.useState<
    string | undefined
  >(undefined)

  // tx detail & gas memos
  const gasFee = React.useMemo(() => {
    if (!transactionInfo) {
      return ''
    }

    return txCoinType === BraveWallet.CoinType.SOL
      ? solFeeEstimate ?? ''
      : getTransactionGasFee(transactionInfo)
  }, [transactionInfo, txCoinType, solFeeEstimate])

  const transactionDetails = React.useMemo(() => {
    if (
      !transactionInfo ||
      !spotPriceRegistry ||
      !txAccount ||
      !transactionsNetwork ||
      !accounts
    ) {
      return undefined
    }

    return parseTransactionWithPrices({
          tx: transactionInfo,
          accounts,
          gasFee,
          spotPriceRegistry,
          tokensList: combinedTokensList,
          transactionAccount: txAccount,
          transactionNetwork: transactionsNetwork
        })
  }, [
    transactionInfo,
    accounts,
    spotPriceRegistry,
    txAccount,
    transactionsNetwork,
    gasFee,
    combinedTokensList
  ])

  // balance queries
  const { data: nativeBalance } = useGetAccountTokenCurrentBalanceQuery(
    txAccount && transactionsNetwork
      ? {
          accountId: txAccount.accountId,
          token: {
            coin: transactionsNetwork.coin,
            chainId: transactionsNetwork.chainId,
            contractAddress: '',
            isErc721: false,
            isNft: false,
            tokenId: ''
          }
        }
      : skipToken
  )

  const { data: transferTokenBalance } = useGetAccountTokenCurrentBalanceQuery(
    txAccount && transactionDetails?.token
      ? {
          accountId: txAccount.accountId,
          token: {
            coin: transactionDetails.token.coin,
            chainId: transactionDetails.token.chainId,
            contractAddress: transactionDetails.token.contractAddress,
            isErc721: transactionDetails.token.isErc721,
            isNft: transactionDetails.token.isNft,
            tokenId: transactionDetails.token.tokenId
          }
        }
      : skipToken
  )

  const { data: sellTokenBalance } = useGetAccountTokenCurrentBalanceQuery(
    txAccount && transactionDetails?.sellToken
      ? {
          accountId: txAccount.accountId,
          token: {
            coin: transactionDetails.sellToken.coin,
            chainId: transactionDetails.sellToken.chainId,
            contractAddress: transactionDetails.sellToken.contractAddress,
            isErc721: transactionDetails.sellToken.isErc721,
            isNft: transactionDetails.sellToken.isNft,
            tokenId: transactionDetails.sellToken.tokenId
          }
        }
      : skipToken
  )

  const insufficientFundsError = React.useMemo(() => {
    return transactionInfo
      ? accountHasInsufficientFundsForTransaction({
          accountNativeBalance: nativeBalance || '',
          accountTokenBalance: transferTokenBalance || '',
          gasFee,
          sellAmountWei: transactionDetails?.sellAmountWei || Amount.empty(),
          sellTokenBalance: sellTokenBalance || '',
          tx: transactionInfo
        })
      : undefined
  }, [
    gasFee,
    nativeBalance,
    sellTokenBalance,
    transactionDetails?.sellAmountWei,
    transactionInfo,
    transferTokenBalance,
    txAccount
  ])

  const insufficientFundsForGasError = React.useMemo(() => {
    return accountHasInsufficientFundsForGas({
      accountNativeBalance: nativeBalance || '',
      gasFee
    })
  }, [nativeBalance, gasFee])

  const { suggestedMaxPriorityFeeChoices, baseFeePerGas } = React.useMemo(
    () => ({
      suggestedMaxPriorityFeeChoices: [
        gasEstimates?.slowMaxPriorityFeePerGas || '0',
        gasEstimates?.avgMaxPriorityFeePerGas || '0',
        gasEstimates?.fastMaxPriorityFeePerGas || '0'
      ],
      baseFeePerGas: gasEstimates?.baseFeePerGas || '0'
    }),
    [gasEstimates]
  )

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
      dispatch(
        walletApi.endpoints.updateUnapprovedTransactionSpendAllowance.initiate({
          chainId: transactionInfo.chainId,
          txMetaId: transactionInfo.id,
          spenderAddress: transactionDetails.approvalTarget || '',
          allowance: new Amount(allowance)
            .multiplyByDecimals(transactionDetails.decimals)
            .toHex()
        })
      )
    }
  }, [transactionInfo?.id, transactionDetails])

  const updateUnapprovedTransactionNonce = React.useCallback(
    (args: UpdateUnapprovedTransactionNonceType) => {
      dispatch(
        walletApi.endpoints.updateUnapprovedTransactionNonce.initiate(args)
      )
    },
    []
  )

  const queueNextTransaction = React.useCallback(() => {
    // if id hasn't been set, start at beginning of tx list
    const currentIndex = selectedPendingTransactionId
      ? pendingTransactions.findIndex(
          (tx) => tx.id === selectedPendingTransactionId
        )
      : 0

    const nextIndex = currentIndex + 1

    const newSelectedPendingTransactionId =
      nextIndex === pendingTransactions.length // at end of list?
        ? pendingTransactions[0]?.id // go to first item in list
        : pendingTransactions[nextIndex]?.id // go to next item in list

    dispatch(
      UIActions.setPendingTransactionId(newSelectedPendingTransactionId)
    )
  }, [selectedPendingTransactionId, pendingTransactions])

  const rejectAllTransactions = React.useCallback(
    () => dispatch(walletApi.endpoints.rejectAllTransactions.initiate()),
    []
  )

  const updateUnapprovedTransactionGasFields = React.useCallback(
    (payload: UpdateUnapprovedTransactionGasFieldsType) => {
      dispatch(
        walletApi.endpoints.updateUnapprovedTransactionGasFields.initiate(
          payload
        )
      )
    },
    []
  )

  const onReject = React.useCallback(() => {
    if (!transactionInfo) {
      return
    }

    dispatch(
      walletApi.endpoints.rejectTransaction.initiate({
        chainId: transactionInfo.chainId,
        coinType: getCoinFromTxDataUnion(
          transactionInfo.txDataUnion
        ),
        id: transactionInfo.id
      })
    )
  }, [transactionInfo])

  const onConfirm = React.useCallback(async () => {
    if (!transactionInfo) {
      return
    }

    if (isHardwareAccount(transactionInfo.fromAccountId)) {
      dispatch(
        walletApi.endpoints.approveHardwareTransaction.initiate({
          chainId: transactionInfo.chainId,
          fromAccountId: transactionInfo.fromAccountId,
          id: transactionInfo.id,
          txDataUnion: transactionInfo.txDataUnion,
          txType: transactionInfo.txType
        })
      )
      return
    }

    try {
      await dispatch(
        walletApi.endpoints.approveTransaction.initiate({
          chainId: transactionInfo.chainId,
          id: transactionInfo.id,
          coinType: getCoinFromTxDataUnion(
            transactionInfo.txDataUnion
          ),
          txType: transactionInfo.txType
        })
      )
      .unwrap()
      dispatch(
        PanelActions.setSelectedTransactionId(transactionInfo.id)
      )
      dispatch(PanelActions.navigateTo('transactionStatus'))
    } catch (error) {
      dispatch(
        UIActions.setTransactionProviderError({
          providerError: error.toString(),
          transactionId: transactionInfo.id
        })
      )
    }
  }, [transactionInfo])

  // memos
  const fromOrb = useAccountOrb(txAccount)
  const toOrb = useAddressOrb(transactionDetails?.recipient, { scale: 10 })

  const transactionTitle = React.useMemo(
    (): string =>
      isSolanaDappTransaction
        ? getLocale('braveWalletApproveTransaction')
        : transactionDetails?.isSwap
          ? getLocale('braveWalletSwap')
          : getLocale('braveWalletSend')
    , [isSolanaDappTransaction, transactionDetails?.isSwap])

  const isLoadingGasFee = React.useMemo(() => {
    // TODO(apaymyshev): handle bitcoin
    if (txCoinType === BraveWallet.CoinType.BTC) {
      return false;
    }

    // SOL
    if (txCoinType === BraveWallet.CoinType.SOL) {
      return isLoadingSolFeeEstimates
    }

    // FIL has gas info provided by txDataUnion
    if (transactionDetails?.isFilecoinTransaction) {
      return gasFee === ''
    }

    // EVM
    return isLoadingGasEstimates
  }, [
    txCoinType,
    isLoadingSolFeeEstimates,
    transactionDetails?.isFilecoinTransaction,
    gasFee,
    isLoadingGasEstimates
  ])

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
      insufficientFundsForGasError === undefined ||
      insufficientFundsError === undefined ||
      !!insufficientFundsForGasError ||
      !!insufficientFundsError ||
      !!transactionDetails?.missingGasLimitError
    )
  }, [
    transactionDetails,
    hasFeeEstimatesError,
    isLoadingGasFee,
    insufficientFundsError,
    insufficientFundsForGasError
  ])

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
    let subscribed = true
    if (transactionInfo?.txType !== BraveWallet.TransactionType.ERC20Approve) {
      return
    }

    if (!transactionDetails?.approvalTarget) {
      return
    }

    if (!txAccount) {
      return () => {
        subscribed = false
      }
    }

    getERC20Allowance(
      transactionDetails.recipient,
      txAccount.address,
      transactionDetails.approvalTarget,
      transactionDetails.chainId,
    )
      .then((result) => {
        subscribed && setERC20AllowanceResult(result)
      })
      .catch((e) => console.error(e))

    // cleanup
    return () => {
      subscribed = false
    }
  }, [
    transactionInfo?.txType,
    txAccount,
    transactionDetails,
    getERC20Allowance
  ])

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
    foundTokenInfoByContractAddress,
    fromAccount: txAccount,
    fromOrb,
    isConfirmButtonDisabled,
    isEthereumTransaction: isEthereumTransaction(transactionInfo),
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
    solanaSendOptions: transactionInfo?.txDataUnion.solanaTxData?.sendOptions,
    updateUnapprovedTransactionGasFields,
    updateUnapprovedTransactionNonce,
    hasFeeEstimatesError,
    selectedPendingTransaction: transactionInfo,
    isLoadingGasFee,
    gasFee,
    onConfirm,
    onReject,
    insufficientFundsError,
    insufficientFundsForGasError
  }
}

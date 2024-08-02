// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert.js'
import * as React from 'react'
import { useDispatch } from 'react-redux'
import { ThunkDispatch } from '@reduxjs/toolkit'
import { skipToken } from '@reduxjs/toolkit/query/react'

// actions
import { UIActions } from '../slices/ui.slice'
import { PanelActions } from '../../panel/actions'

// utils
import Amount from '../../utils/amount'
import { getPriceIdForToken } from '../../utils/pricing-utils'
import { isHardwareAccount } from '../../utils/account-utils'
import { getLocale } from '../../../common/locale'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'
import { UISelectors } from '../selectors'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  getTransactionGasFee,
  parseTransactionWithPrices,
  findTransactionToken,
  isEthereumTransaction,
  isZCashTransaction,
  isBitcoinTransaction,
  isEIP1559Transaction
} from '../../utils/tx-utils'
import { makeNetworkAsset } from '../../options/asset-options'

// Custom Hooks
import useGetTokenInfo from './use-get-token-info'
import { useAccountOrb, useAddressOrb } from './use-orb'
import { useSafeUISelector } from './use-safe-selector'
import {
  useApproveTransactionMutation,
  useGetAccountInfosRegistryQuery,
  useGetAccountTokenCurrentBalanceQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetERC20AllowanceQuery,
  useGetGasEstimation1559Query,
  useGetNetworkQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetTokenSpotPricesQuery,
  useRejectTransactionsMutation,
  walletApi
} from '../slices/api.slice'
import {
  usePendingTransactionsQuery,
  useGetCombinedTokensListQuery,
  useAccountQuery
} from '../slices/api.slice.extra'
import { useSwapTransactionParser } from './use-swap-tx-parser'
import {
  defaultQuerySubscriptionOptions,
  querySubscriptionOptions60s
} from '../slices/constants'

// Constants
import { BraveWallet, emptyProviderErrorCodeUnion } from '../../constants/types'
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
  const { pendingTransactions, isLoading: isLoadingPendingTransactions } =
    usePendingTransactionsQuery({
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
  return { selectedPendingTransaction, isLoading: isLoadingPendingTransactions }
}

export const usePendingTransactions = () => {
  // redux
  const dispatch = useDispatch<ThunkDispatch<any, any, any>>()
  const selectedPendingTransactionId = useSafeUISelector(
    UISelectors.selectedPendingTransactionId
  )

  // mutations
  const [rejectTransactions] = useRejectTransactionsMutation()
  const [approveTransaction] = useApproveTransactionMutation()

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

  const txToken = findTransactionToken(transactionInfo, combinedTokensList)

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

  const {
    data: solFeeEstimate,
    isLoading: isLoadingSolFeeEstimates = txCoinType ===
      BraveWallet.CoinType.SOL,
    isError: hasSolFeeEstimatesError
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

  const {
    data: gasEstimates,
    isLoading: isLoadingGasEstimates,
    isError: hasEvmFeeEstimatesError
  } = useGetGasEstimation1559Query(
    transactionsNetwork &&
      transactionsNetwork.coin === BraveWallet.CoinType.ETH &&
      transactionInfo &&
      isEIP1559Transaction(transactionInfo)
      ? transactionsNetwork.chainId
      : skipToken,
    defaultQuerySubscriptionOptions
  )

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

  // token approval queries
  const { data: erc20AllowanceResult } = useGetERC20AllowanceQuery(
    txAccount && transactionDetails?.approvalTarget
      ? {
          contractAddress: transactionDetails.recipient,
          ownerAddress: txAccount.address,
          spenderAddress: transactionDetails.approvalTarget,
          chainId: transactionDetails.chainId
        }
      : skipToken,
    defaultQuerySubscriptionOptions
  )

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

  const { sellToken, sellAmountWei } = useSwapTransactionParser(transactionInfo)

  const { data: sellTokenBalance } = useGetAccountTokenCurrentBalanceQuery(
    txAccount && sellToken
      ? {
          accountId: txAccount.accountId,
          token: {
            coin: sellToken.coin,
            chainId: sellToken.chainId,
            contractAddress: sellToken.contractAddress,
            isErc721: sellToken.isErc721,
            isNft: sellToken.isNft,
            tokenId: sellToken.tokenId
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
          sellAmountWei,
          sellTokenBalance: sellTokenBalance || '',
          tx: transactionInfo
        })
      : false
  }, [
    gasFee,
    nativeBalance,
    sellTokenBalance,
    sellAmountWei,
    transactionInfo,
    transferTokenBalance
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

  const transactionQueueNumber =
    pendingTransactions.findIndex((tx) => tx.id === transactionInfo?.id) + 1
  const transactionsQueueLength = pendingTransactions.length

  const isERC20Approve =
    transactionInfo?.txType === BraveWallet.TransactionType.ERC20Approve
  const isERC721SafeTransferFrom =
    transactionInfo?.txType ===
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  const isERC721TransferFrom =
    transactionInfo?.txType === BraveWallet.TransactionType.ERC721TransferFrom

  const isSolanaDappTransaction = transactionInfo?.txType
    ? [
        BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
        BraveWallet.TransactionType.SolanaDappSignTransaction
      ].includes(transactionInfo.txType)
    : false

  // methods
  const onEditAllowanceSave = React.useCallback(
    (allowance: string) => {
      if (transactionInfo?.id && transactionDetails) {
        dispatch(
          walletApi.endpoints.updateUnapprovedTransactionSpendAllowance //
            .initiate({
              chainId: transactionInfo.chainId,
              txMetaId: transactionInfo.id,
              spenderAddress: transactionDetails.approvalTarget || '',
              allowance: new Amount(allowance)
                .multiplyByDecimals(transactionDetails.decimals)
                .toHex()
            })
        )
      }
    },
    [transactionInfo, transactionDetails, dispatch]
  )

  const updateUnapprovedTransactionNonce = React.useCallback(
    (args: UpdateUnapprovedTransactionNonceType) => {
      dispatch(
        walletApi.endpoints.updateUnapprovedTransactionNonce.initiate(args)
      )
    },
    [dispatch]
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

    dispatch(UIActions.setPendingTransactionId(newSelectedPendingTransactionId))
  }, [selectedPendingTransactionId, pendingTransactions, dispatch])

  const rejectAllTransactions = React.useCallback(async () => {
    await rejectTransactions(
      pendingTransactions.map((tx) => ({
        id: tx.id,
        chainId: tx.chainId,
        coinType: getCoinFromTxDataUnion(tx.txDataUnion)
      }))
    ).unwrap()
  }, [pendingTransactions, rejectTransactions])

  const updateUnapprovedTransactionGasFields = React.useCallback(
    (payload: UpdateUnapprovedTransactionGasFieldsType) => {
      dispatch(
        walletApi.endpoints.updateUnapprovedTransactionGasFields.initiate(
          payload
        )
      )
    },
    [dispatch]
  )

  const onReject = React.useCallback(() => {
    if (!transactionInfo) {
      return
    }

    rejectTransactions([
      {
        chainId: transactionInfo.chainId,
        coinType: getCoinFromTxDataUnion(transactionInfo.txDataUnion),
        id: transactionInfo.id
      }
    ])
  }, [transactionInfo, rejectTransactions])

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
      const result = await approveTransaction({
        chainId: transactionInfo.chainId,
        id: transactionInfo.id,
        coinType: getCoinFromTxDataUnion(transactionInfo.txDataUnion),
        txType: transactionInfo.txType
      }).unwrap()
      if (!result.success) {
        dispatch(
          UIActions.setTransactionProviderError({
            providerError: {
              code: result.errorUnion,
              message: result.errorMessage
            },
            transactionId: transactionInfo.id
          })
        )
      }
    } catch (error) {
      dispatch(
        UIActions.setTransactionProviderError({
          providerError: {
            code: {
              ...emptyProviderErrorCodeUnion,
              providerError: BraveWallet.ProviderError.kUnknown
            },
            message: error.toString()
          },
          transactionId: transactionInfo.id
        })
      )
    } finally {
      dispatch(
        PanelActions.setSelectedTransactionId({
          chainId: transactionInfo.chainId,
          coin: getCoinFromTxDataUnion(transactionInfo.txDataUnion),
          id: transactionInfo.id
        })
      )
      dispatch(PanelActions.navigateTo('transactionStatus'))
    }
  }, [approveTransaction, dispatch, transactionInfo])

  // memos
  const fromOrb = useAccountOrb(txAccount)
  const toOrb = useAddressOrb(transactionDetails?.recipient, { scale: 10 })

  const transactionTitle = React.useMemo(
    (): string =>
      isSolanaDappTransaction
        ? getLocale('braveWalletApproveTransaction')
        : transactionDetails?.isSwap
        ? getLocale('braveWalletSwap')
        : getLocale('braveWalletSend'),
    [isSolanaDappTransaction, transactionDetails?.isSwap]
  )

  const isLoadingGasFee = React.useMemo(() => {
    if (txCoinType === undefined) {
      return false
    }

    // BTC and ZEC provide fee by txDataUnion
    if (
      txCoinType === BraveWallet.CoinType.BTC ||
      txCoinType === BraveWallet.CoinType.ZEC
    ) {
      return false
    }

    // SOL
    if (txCoinType === BraveWallet.CoinType.SOL) {
      return isLoadingSolFeeEstimates
    }

    // FIL has gas info provided by txDataUnion
    if (txCoinType === BraveWallet.CoinType.FIL) {
      return gasFee === ''
    }

    // EVM
    if (txCoinType === BraveWallet.CoinType.ETH) {
      return isLoadingGasEstimates
    }

    assertNotReached(`Unknown coin ${txCoinType}`)
  }, [txCoinType, isLoadingSolFeeEstimates, gasFee, isLoadingGasEstimates])

  const hasFeeEstimatesError =
    txCoinType === BraveWallet.CoinType.SOL
      ? hasSolFeeEstimatesError
      : hasEvmFeeEstimatesError

  const isConfirmButtonDisabled = React.useMemo(() => {
    if (hasFeeEstimatesError || isLoadingGasFee) {
      return true
    }

    if (!transactionDetails) {
      return true
    }

    return Boolean(
      transactionDetails?.sameAddressError ||
        transactionDetails?.contractAddressError ||
        insufficientFundsForGasError ||
        insufficientFundsError ||
        transactionDetails?.missingGasLimitError
    )
  }, [
    transactionDetails,
    hasFeeEstimatesError,
    isLoadingGasFee,
    insufficientFundsError,
    insufficientFundsForGasError
  ])

  const { currentTokenAllowance, isCurrentAllowanceUnlimited } =
    React.useMemo(() => {
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
    }, [erc20AllowanceResult, transactionDetails])

  const { tokenInfo: erc20ApproveTokenInfo } = useGetTokenInfo(
    transactionDetails?.recipient &&
      txCoinType &&
      transactionInfo?.txType === BraveWallet.TransactionType.ERC20Approve
      ? {
          contractAddress: transactionDetails.recipient,
          network: {
            chainId: transactionDetails.chainId,
            coin: txCoinType
          }
        }
      : skipToken
  )

  return {
    baseFeePerGas,
    currentTokenAllowance,
    isCurrentAllowanceUnlimited,
    erc20ApproveTokenInfo,
    fromAccount: txAccount,
    fromOrb,
    isConfirmButtonDisabled,
    isEthereumTransaction: isEthereumTransaction(transactionInfo),
    isERC20Approve,
    isERC721SafeTransferFrom,
    isERC721TransferFrom,
    isSolanaDappTransaction,
    isSolanaTransaction: transactionDetails?.isSolanaTransaction || false,
    isAssociatedTokenAccountCreation:
      transactionDetails?.isAssociatedTokenAccountCreation || false,
    isFilecoinTransaction: transactionDetails?.isFilecoinTransaction || false,
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
    insufficientFundsForGasError,
    isZCashTransaction: isZCashTransaction(transactionInfo),
    isBitcoinTransaction: isBitcoinTransaction(transactionInfo)
  }
}

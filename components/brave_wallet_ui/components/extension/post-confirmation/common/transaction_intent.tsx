// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo, //
} from '../../../../constants/types'

// Utils
import { getLocale, formatLocale } from '$web-common/locale'
import {
  findTransactionToken,
  getFormattedTransactionTransferredValue,
  getIsTxApprovalUnlimited,
  getTransactionApprovalTargetAddress,
  getTransactionToAddress,
  isBridgeTransaction,
  isSwapTransaction,
  getIsSolanaAssociatedTokenAccountCreation,
} from '../../../../utils/tx-utils'
import { getCoinFromTxDataUnion } from '../../../../utils/network-utils'
import { getAddressLabel } from '../../../../utils/account-utils'
import Amount from '../../../../utils/amount'

// Queries
import {
  useAccountQuery,
  useGetCombinedTokensListQuery,
} from '../../../../common/slices/api.slice.extra'
import {
  useGetAccountInfosRegistryQuery,
  useGetNetworkQuery,
  useGetZCashTransactionTypeQuery,
} from '../../../../common/slices/api.slice'
import {
  accountInfoEntityAdaptorInitialState, //
} from '../../../../common/slices/entities/account-info.entity'

// Hooks
import {
  useTransactionsNetwork, //
} from '../../../../common/hooks/use-transactions-network'
import {
  useSwapTransactionParser, //
} from '../../../../common/hooks/use-swap-tx-parser'
import { useExplorer } from '../../../../common/hooks/explorer'

// Styled Components
import { Button, ExplorerIcon } from './common.style'
import { Row, Text } from '../../../shared/style'

interface Props {
  transaction: SerializableTransactionInfo
  swapStatus?: BraveWallet.Gate3SwapStatus
}

export const TransactionIntent = (props: Props) => {
  const { transaction, swapStatus } = props

  // Computed & Queries
  const isBridge = isBridgeTransaction(transaction)
  const isSwap = isSwapTransaction(transaction)
  const isSwapOrBridge = isBridge || isSwap
  const hasSwapInfo = !!transaction.swapInfo
  const isERC20Approval =
    transaction.txType === BraveWallet.TransactionType.ERC20Approve
  const isTxApprovalUnlimited = getIsTxApprovalUnlimited(transaction)
  const txApprovalTarget = getTransactionApprovalTargetAddress(transaction)
  const txCoinType = getCoinFromTxDataUnion(transaction.txDataUnion)
  const txToAddress = getTransactionToAddress(transaction)
  const isSOLSwapOrBridge =
    txCoinType === BraveWallet.CoinType.SOL && isSwapOrBridge
  const isSolanaATACreation =
    getIsSolanaAssociatedTokenAccountCreation(transaction)

  const { account: txAccount } = useAccountQuery(transaction.fromAccountId)
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()

  const transactionsToken = findTransactionToken(
    transaction,
    combinedTokensList,
  )

  const transactionNetwork = useTransactionsNetwork(transaction)

  const {
    data: getZCashTransactionTypeResult = { txType: null, error: null },
  } = useGetZCashTransactionTypeQuery(
    txCoinType === BraveWallet.CoinType.ZEC
      && transactionNetwork
      && transactionsToken
      && txAccount
      && txToAddress
      ? {
          chainId: transactionNetwork.chainId,
          accountId: txAccount.accountId,
          useShieldedPool: transactionsToken.isShielded,
          address: txToAddress,
        }
      : skipToken,
  )

  const isShieldingFunds =
    getZCashTransactionTypeResult.txType === BraveWallet.ZCashTxType.kShielding
  const isUnshieldingFunds =
    getZCashTransactionTypeResult.txType
    === BraveWallet.ZCashTxType.kUnshielding

  // Custom Hooks
  const onClickViewOnBlockExplorer = useExplorer(transactionNetwork)

  const { normalizedTransferredValue } =
    getFormattedTransactionTransferredValue({
      tx: transaction,
      token: transactionsToken,
      txAccount,
      txNetwork: transactionNetwork,
    })

  const { data: txNetwork } = useGetNetworkQuery({
    chainId: transaction.chainId,
    coin: txCoinType,
  })

  const { data: bridgeToNetwork } = useGetNetworkQuery(
    isBridge
      && transaction.swapInfo?.destinationChainId
      && transaction.swapInfo?.destinationCoin !== undefined
      ? {
          chainId: transaction.swapInfo.destinationChainId,
          coin: transaction.swapInfo.destinationCoin,
        }
      : skipToken,
  )

  const { data: accountInfosRegistry = accountInfoEntityAdaptorInitialState } =
    useGetAccountInfosRegistryQuery(undefined)

  const {
    destinationToken,
    sourceToken,
    destinationAmount,
    sourceAmount,
    destinationAddress,
  } = useSwapTransactionParser(transaction)

  const formattedSourceAmount =
    sourceAmount && sourceToken
      ? sourceAmount
          .divideByDecimals(sourceToken.decimals)
          .formatAsAsset(6, sourceToken.symbol)
      : ''
  const formattedDestinationAmount =
    destinationAmount && destinationToken
      ? destinationAmount
          .divideByDecimals(destinationToken.decimals)
          .formatAsAsset(6, destinationToken.symbol)
      : ''

  const transactionFailed =
    transaction.txStatus === BraveWallet.TransactionStatus.Dropped
    || transaction.txStatus === BraveWallet.TransactionStatus.Error

  const transactionConfirmed =
    transaction.txStatus === BraveWallet.TransactionStatus.Confirmed

  const swapOrBridgeRecipient = destinationAddress || txAccount?.address || ''

  const recipientLabel = getAddressLabel(
    isERC20Approval
      ? txApprovalTarget
      : isSwapOrBridge
        ? swapOrBridgeRecipient
        : txToAddress,
    accountInfosRegistry,
  )

  const formattedSendAmount = React.useMemo(() => {
    if (!transactionsToken) {
      return ''
    }
    if (
      transactionsToken.isErc721
      || transactionsToken.isErc1155
      || transactionsToken.isNft
    ) {
      return `${transactionsToken.name} ${transactionsToken.symbol}`
    }
    return new Amount(normalizedTransferredValue).formatAsAsset(
      6,
      transactionsToken.symbol,
    )
  }, [transactionsToken, normalizedTransferredValue])

  const formattedApprovalAmount = isTxApprovalUnlimited
    ? `${getLocale('braveWalletTransactionApproveUnlimited')} ${
        transactionsToken?.symbol ?? ''
      }`
    : formattedSendAmount

  const sendSwapOrBridgeLocale = React.useMemo(() => {
    if (isBridge) {
      return getLocale('braveWalletBridge').toLocaleLowerCase()
    }
    if (isSwap) {
      return getLocale('braveWalletSwap').toLocaleLowerCase()
    }
    return getLocale('braveWalletSend').toLocaleLowerCase()
  }, [isBridge, isSwap])

  const swappingOrBridgingLocale = React.useMemo(() => {
    if (isBridge) {
      return getLocale('braveWalletBridging')
    }
    return getLocale('braveWalletSwapping')
  }, [isBridge])

  const firstDuringValue = React.useMemo(() => {
    if (isERC20Approval) {
      return formattedApprovalAmount
    }
    if (isSOLSwapOrBridge && transactionFailed) {
      return sendSwapOrBridgeLocale
    }
    if (isSOLSwapOrBridge) {
      return swappingOrBridgingLocale
    }
    if (isSwapOrBridge && transactionConfirmed) {
      return formattedDestinationAmount
    }
    if (isSwapOrBridge) {
      return formattedSourceAmount
    }
    return formattedSendAmount
  }, [
    isERC20Approval,
    formattedApprovalAmount,
    isSwapOrBridge,
    transactionConfirmed,
    formattedDestinationAmount,
    formattedSourceAmount,
    formattedSendAmount,
    isSOLSwapOrBridge,
    sendSwapOrBridgeLocale,
    transactionFailed,
    swappingOrBridgingLocale,
  ])

  const secondDuringValue = React.useMemo(() => {
    if (isSOLSwapOrBridge) {
      return txNetwork?.chainName ?? ''
    }
    if (isSwapOrBridge && transactionConfirmed) {
      return recipientLabel
    }
    if (isBridge) {
      return bridgeToNetwork?.chainName ?? ''
    }
    if (isSwap) {
      return formattedDestinationAmount
    }
    return recipientLabel
  }, [
    isSwapOrBridge,
    transactionConfirmed,
    recipientLabel,
    isBridge,
    bridgeToNetwork,
    isSwap,
    formattedDestinationAmount,
    isSOLSwapOrBridge,
    txNetwork,
  ])

  const descriptionLocale = React.useMemo(() => {
    if (isSolanaATACreation && transactionFailed) {
      return 'braveWalletFailedToCreateAssociatedTokenAccount'
    }
    if (isSolanaATACreation && transactionConfirmed) {
      return 'braveWalletAssociatedTokenAccountCreated'
    }
    if (isSolanaATACreation) {
      return 'braveWalletCreatingAssociatedTokenAccount'
    }
    if (transactionFailed && isSOLSwapOrBridge) {
      return 'braveWalletErrorAttemptingToTransactOnNetwork'
    }
    if (transactionFailed) {
      return 'braveWalletErrorAttemptingToTransact'
    }
    if (isSOLSwapOrBridge) {
      return 'braveWalletSwappingOrBridgingOnNetwork'
    }
    if (isSwapOrBridge && transactionConfirmed) {
      return 'braveWalletAmountAddedToAccount'
    }
    if (isBridge) {
      return 'braveWalletBridgingAmountToNetwork'
    }
    if (isSwap) {
      return 'braveWalletSwappingAmountToAmountOnNetwork'
    }
    if (isERC20Approval) {
      return 'braveWalletApprovingAmountOnExchange'
    }
    if (transactionConfirmed && isShieldingFunds) {
      return 'braveWalletAmountHasBeenShielded'
    }
    if (transactionConfirmed && isUnshieldingFunds) {
      return 'braveWalletAmountHasBeenUnshielded'
    }
    if (isShieldingFunds) {
      return 'braveWalletShieldingAmount'
    }
    if (isUnshieldingFunds) {
      return 'braveWalletUnshieldingAmount'
    }
    if (transactionConfirmed) {
      return 'braveWalletAmountSentToAccount'
    }
    return 'braveWalletSendingAmountToAccount'
  }, [
    transactionFailed,
    isSwapOrBridge,
    transactionConfirmed,
    isBridge,
    isSwap,
    isERC20Approval,
    isSOLSwapOrBridge,
    isShieldingFunds,
    isUnshieldingFunds,
    isSolanaATACreation,
  ])

  // Derive swap status states
  const swapSuccess =
    swapStatus?.status === BraveWallet.Gate3SwapStatusCode.kSuccess
  const swapFailed =
    swapStatus?.status === BraveWallet.Gate3SwapStatusCode.kFailed
    || swapStatus?.status === BraveWallet.Gate3SwapStatusCode.kRefunded

  const swapInfoDescription = React.useMemo(() => {
    if (!hasSwapInfo) {
      return null
    }

    const destNetworkName = bridgeToNetwork?.chainName ?? txNetwork?.chainName

    // Success: "The amount of 100 USDC has been added to your account 0x123 on Polygon"
    if (swapSuccess) {
      return getLocale('braveWalletAmountAddedToAccount')
        .replace('$1', formattedDestinationAmount)
        .replace(
          '$2',
          `${recipientLabel} ${getLocale(
            'braveWalletSwappingOrBridgingOnNetwork',
          )
            .replace('$1', '')
            .replace('$2', destNetworkName ?? '')
            .trim()}`,
        )
    }

    // Non-terminal & Failed states share the same format, only the action word differs
    // Failed: "Swap/Bridge", Non-terminal: "Swapping/Bridging"
    const actionLocale = isBridge
      ? getLocale(swapFailed ? 'braveWalletBridge' : 'braveWalletBridging')
      : getLocale(swapFailed ? 'braveWalletSwap' : 'braveWalletSwapping')

    if (isBridge) {
      // "{action} 0.5 ETH on Ethereum to USDC on Polygon"
      const sourceWithNetwork = `${formattedSourceAmount} ${getLocale(
        'braveWalletSwappingOrBridgingOnNetwork',
      )
        .replace('$1', '')
        .replace('$2', txNetwork?.chainName ?? '')
        .trim()}`
      const destWithNetwork = `${destinationToken?.symbol ?? ''} ${getLocale(
        'braveWalletSwappingOrBridgingOnNetwork',
      )
        .replace('$1', '')
        .replace('$2', bridgeToNetwork?.chainName ?? '')
        .trim()}`
      return `${actionLocale} ${sourceWithNetwork} to ${destWithNetwork}`
    }

    // "{action} 0.5 ETH to USDC on Ethereum"
    const onNetwork = getLocale('braveWalletSwappingOrBridgingOnNetwork')
      .replace('$1', '')
      .replace('$2', txNetwork?.chainName ?? '')
      .trim()
    return `${actionLocale} ${formattedSourceAmount} to ${destinationToken?.symbol ?? ''} ${onNetwork}`
  }, [
    hasSwapInfo,
    swapSuccess,
    swapFailed,
    isBridge,
    formattedSourceAmount,
    formattedDestinationAmount,
    destinationToken?.symbol,
    recipientLabel,
    txNetwork?.chainName,
    bridgeToNetwork?.chainName,
  ])

  // Handler for explorer button click
  const onExplorerClick = React.useCallback(() => {
    // Use swap explorer URL if provided
    if (swapStatus?.explorerUrl) {
      window.open(swapStatus.explorerUrl, '_blank', 'noreferrer')
      return
    }
    // Fallback to existing block explorer logic
    onClickViewOnBlockExplorer(
      isSwapOrBridge
        && transaction.swapInfo?.provider === BraveWallet.SwapProvider.kLiFi
        ? 'lifi'
        : 'tx',
      transaction.txHash,
    )()
  }, [
    swapStatus?.explorerUrl,
    onClickViewOnBlockExplorer,
    isSwapOrBridge,
    transaction.swapInfo?.provider,
    transaction.txHash,
  ])

  // If both swapInfo and swapStatus exist, the transaction was
  // created via gate3.
  if (hasSwapInfo && swapInfoDescription && swapStatus) {
    return (
      <Row
        gap='4px'
        flexWrap='wrap'
        padding='16px'
      >
        <Text
          textColor='secondary'
          textSize='14px'
        >
          {swapInfoDescription}
        </Text>
        <Button onClick={onExplorerClick}>
          <ExplorerIcon />
        </Button>
      </Row>
    )
  }

  // Legacy locales
  const description = formatLocale(
    descriptionLocale,
    {
      $1: (
        <Text
          textColor='primary'
          textSize='14px'
          isBold
        >
          {firstDuringValue}
        </Text>
      ),
      $2: (
        <Row
          width='unset'
          gap='4px'
        >
          <Text
            textColor='primary'
            textSize='14px'
            isBold
          >
            {secondDuringValue}
          </Text>
          <Button onClick={onExplorerClick}>
            <ExplorerIcon />
          </Button>
        </Row>
      ),
      $3: transactionFailed
        ? sendSwapOrBridgeLocale
        : (txNetwork?.chainName ?? ''),
    },
    {
      noErrorOnMissingReplacement: true,
    },
  )

  return (
    <Row
      gap='4px'
      flexWrap='wrap'
      padding='16px'
    >
      <Text
        textColor='secondary'
        textSize='14px'
      >
        {description}
      </Text>
    </Row>
  )
}

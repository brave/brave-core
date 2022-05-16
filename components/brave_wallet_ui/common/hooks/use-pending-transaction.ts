import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { create } from 'ethereum-blockies'

// actions
import * as WalletActions from '../actions/wallet_actions'

// assets
import { AssetIcon } from '../../components/buy-send-swap/select-asset-item/style'

// utils
import Amount from '../../utils/amount'
import { reduceAccountDisplayName } from '../../utils/reduce-account-name'
import { getLocale } from '../../../common/locale'
import { withPlaceholderIcon } from '../../components/shared'

// Custom Hooks
import { useTransactionParser } from './transaction-parser'
import usePricing from './pricing'
import useTokenInfo from './token'
import { useLib } from './useLib'

import { WalletState, BraveWallet } from '../../constants/types'
import { UpdateUnapprovedTransactionGasFieldsType, UpdateUnapprovedTransactionNonceType } from '../constants/action_types'
import { getNetworkFromTXDataUnion } from '../../utils/network-utils'

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
    solFeeEstimates,
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
  const parseTransaction = useTransactionParser(
    transactionsNetwork,
    accounts,
    transactionSpotPrices,
    visibleTokens,
    fullTokenList,
    solFeeEstimates
  )
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
  const isSolanaTransaction =
    transactionInfo?.txType === BraveWallet.TransactionType.SolanaSystemTransfer ||
    transactionInfo?.txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
    transactionInfo?.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
  const isAssociatedTokenAccountCreation = transactionInfo?.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation

  // methods
  const findAccountName = (address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
  }

  const onEditAllowanceSave = (allowance: string) => {
    if (transactionInfo && transactionDetails) {
      dispatch(WalletActions.updateUnapprovedTransactionSpendAllowance({
        txMetaId: transactionInfo.id,
        spenderAddress: transactionDetails.approvalTarget || '',
        allowance: new Amount(allowance)
          .multiplyByDecimals(transactionDetails.decimals)
          .toHex()
      }))
    }
  }

  const updateUnapprovedTransactionNonce = (args: UpdateUnapprovedTransactionNonceType) => {
    dispatch(WalletActions.updateUnapprovedTransactionNonce(args))
  }

  const queueNextTransaction = () => dispatch(WalletActions.queueNextTransaction())

  const rejectAllTransactions = () => dispatch(WalletActions.rejectAllTransactions())

  const updateUnapprovedTransactionGasFields = (payload: UpdateUnapprovedTransactionGasFieldsType) => {
    dispatch(WalletActions.updateUnapprovedTransactionGasFields(payload))
  }

  // memos
  const fromOrb = React.useMemo(() => {
    return create({ seed: transactionDetails?.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails])

  const toOrb = React.useMemo(() => {
    return create({
      seed: transactionDetails?.recipient.toLowerCase(),
      size: 8,
      scale: 10
    }).toDataURL()
  }, [transactionDetails])

  const fromAccountName = React.useMemo(() => reduceAccountDisplayName(
    (transactionInfo && findAccountName(transactionInfo.fromAddress)) ?? '',
    11
  ), [transactionInfo])

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 0 })
  }, [])

  const transactionTitle = React.useMemo(
    (): string =>
      transactionDetails?.isSwap
        ? getLocale('braveWalletSwap')
        : getLocale('braveWalletSend')
    , [transactionDetails])

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
  }, [])

  React.useEffect(() => {
    if (
      transactionDetails &&
      transactionInfo?.txType === BraveWallet.TransactionType.ERC20Approve
    ) {
      onFindTokenInfoByContractAddress(transactionDetails.recipient)
    }
  }, [])

  return {
    AssetIconWithPlaceholder,
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
    isSolanaTransaction,
    isAssociatedTokenAccountCreation,
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
    updateUnapprovedTransactionGasFields,
    updateUnapprovedTransactionNonce
  }
}

// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import * as EthereumBlockies from 'ethereum-blockies'

// Hooks
import { useExplorer, useTransactionParser } from '../../../common/hooks'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getTransactionStatusString, isSolanaTransaction } from '../../../utils/tx-utils'
import { toProperCase } from '../../../utils/string-utils'
import Amount from '../../../utils/amount'
import { getNetworkFromTXDataUnion, getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { getLocale } from '../../../../common/locale'

// Constants
import {
  BraveWallet,
  WalletAccountType,
  DefaultCurrencies,
  WalletState,
  SerializableTransactionInfo
} from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  OrbContainer,
  FromCircle,
  ToCircle,
  DetailRow,
  DetailTitle,
  DetailButton,
  StatusRow,
  BalanceColumn,
  TransactionValue,
  PanelDescription,
  SpacerText,
  FromToRow,
  AccountNameText,
  ArrowIcon,
  AlertIcon
} from './style'
import {
  DetailTextDarkBold,
  DetailTextDark
} from '../shared-panel-styles'
import Header from '../../buy-send-swap/select-header'
import { StatusBubble } from '../../shared/style'
import { TransactionStatusTooltip } from '../transaction-status-tooltip'
import { Tooltip } from '../../shared'
import { serializedTimeDeltaToJSDate } from '../../../utils/datetime-utils'
import { useGetDefaultNetworksQuery } from '../../../common/slices/api.slice'

export interface Props {
  transaction: SerializableTransactionInfo
  selectedNetwork?: BraveWallet.NetworkInfo
  accounts: WalletAccountType[]
  visibleTokens: BraveWallet.BlockchainToken[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  onBack: () => void
  onRetryTransaction: (transaction: SerializableTransactionInfo) => void
  onSpeedupTransaction: (transaction: SerializableTransactionInfo) => void
  onCancelTransaction: (transaction: SerializableTransactionInfo) => void
}

const TransactionDetailPanel = (props: Props) => {
  // props
  const {
    transaction,
    selectedNetwork,
    accounts,
    defaultCurrencies,
    onBack,
    onRetryTransaction,
    onSpeedupTransaction,
    onCancelTransaction
  } = props

  // redux
  const {
    transactions,
    transactionProviderErrorRegistry,
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  // queries
  const { data: defaultNetworks = [] } = useGetDefaultNetworksQuery()

  const transactionsNetwork = React.useMemo(() => {
    return getNetworkFromTXDataUnion(transaction.txDataUnion, defaultNetworks, selectedNetwork)
  }, [defaultNetworks, transaction, selectedNetwork])

  const transactionsList = React.useMemo(() => {
    return accounts.map((account) => {
      return transactions[account.address]
    }).flat(1)
  }, [accounts, transactions])

  const liveTransaction = React.useMemo(() => {
    return transactionsList.find(tx => tx.id === transaction.id) ?? transaction
  }, [transaction, transactionsList])

  const parseTransaction = useTransactionParser(transactionsNetwork)
  const transactionDetails = React.useMemo(
    () => parseTransaction(liveTransaction),
    [liveTransaction]
  )

  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.sender])

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.recipient.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.recipient])

  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)

  const onClickRetryTransaction = () => {
    if (liveTransaction) {
      onRetryTransaction(liveTransaction)
    }
  }

  const onClickSpeedupTransaction = () => {
    if (liveTransaction) {
      onSpeedupTransaction(liveTransaction)
    }
  }

  const onClickCancelTransaction = () => {
    if (liveTransaction) {
      onCancelTransaction(liveTransaction)
    }
  }

  const transactionTitle = React.useMemo((): string => {
    if (transactionDetails.isSwap) {
      return toProperCase(getLocale('braveWalletSwap'))
    }
    if (liveTransaction.txType === BraveWallet.TransactionType.ERC20Approve) {
      return toProperCase(getLocale('braveWalletApprovalTransactionIntent'))
    }
    return toProperCase(getLocale('braveWalletTransactionSent'))
  }, [transactionDetails, liveTransaction])

  const transactionValue = React.useMemo((): string => {
    if (liveTransaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
      liveTransaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom) {
      return transactionDetails.erc721BlockchainToken?.name + ' ' + transactionDetails.erc721TokenId
    }

    if (liveTransaction.txType === BraveWallet.TransactionType.ERC20Approve && transactionDetails.isApprovalUnlimited) {
      return `${getLocale('braveWalletTransactionApproveUnlimited')} ${transactionDetails.symbol}`
    }

    return new Amount(transactionDetails.value)
      .formatAsAsset(undefined, transactionDetails.symbol)
  }, [transactionDetails, liveTransaction])

  const transactionFiatValue = React.useMemo((): string => {
    if (liveTransaction.txType !== BraveWallet.TransactionType.ERC721TransferFrom &&
      liveTransaction.txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
      liveTransaction.txType !== BraveWallet.TransactionType.ERC20Approve) {
      return new Amount(transactionDetails.fiatValue)
        .formatAsFiat(defaultCurrencies.fiat)
    }
    return ''
  }, [transactionDetails, liveTransaction, defaultCurrencies])

  const isSolanaTxn = isSolanaTransaction(liveTransaction)
  const isFilecoinTransaction = getCoinFromTxDataUnion(liveTransaction.txDataUnion) === BraveWallet.CoinType.FIL

  return (
    <StyledWrapper>
      <Header
        title={getLocale('braveWalletTransactionDetails')}
        onBack={onBack}
      />
      <OrbContainer>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
      </OrbContainer>
      <FromToRow>
        <Tooltip
          text={transactionDetails.sender}
          isAddress={true}
          position='left'
        >
          <AccountNameText>{transactionDetails.senderLabel}</AccountNameText>
        </Tooltip>
        <ArrowIcon />
        <Tooltip
          text={transactionDetails.recipient}
          isAddress={true}
          position='right'
        >
          <AccountNameText>{transactionDetails.recipientLabel}</AccountNameText>
        </Tooltip>
      </FromToRow>
      <PanelDescription>{transactionTitle}</PanelDescription>
      <TransactionValue>{transactionValue}</TransactionValue>
      <PanelDescription>{transactionFiatValue}</PanelDescription>
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailStatus')}
        </DetailTitle>
        <StatusRow>
          <StatusBubble status={transactionDetails.status} />
          <DetailTextDarkBold>
            {getTransactionStatusString(transactionDetails.status)}
          </DetailTextDarkBold>

          {transactionDetails.status === BraveWallet.TransactionStatus.Error && transactionProviderErrorRegistry[liveTransaction.id] &&
            <TransactionStatusTooltip text={
              `${transactionProviderErrorRegistry[liveTransaction.id].code}: ${transactionProviderErrorRegistry[liveTransaction.id].message}`
            }>
              <AlertIcon />
            </TransactionStatusTooltip>
          }
        </StatusRow>
      </DetailRow>
      {/* Will remove this conditional for solana once https://github.com/brave/brave-browser/issues/22040 is implemented. */}
      {!isSolanaTxn &&
        <DetailRow>
          <DetailTitle>
            {getLocale('braveWalletAllowSpendTransactionFee')}
          </DetailTitle>
          <BalanceColumn>
            <DetailTextDark>
              {transactionsNetwork && new Amount(transactionDetails.gasFee)
                .divideByDecimals(transactionsNetwork.decimals)
                .formatAsAsset(6, transactionsNetwork.symbol)
              }
            </DetailTextDark>
            <DetailTextDark>
              {
                new Amount(transactionDetails.gasFeeFiat)
                  .formatAsFiat(defaultCurrencies.fiat)
              }
            </DetailTextDark>
          </BalanceColumn>
        </DetailRow>
      }
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailDate')}
        </DetailTitle>
        <DetailTextDark>
          {serializedTimeDeltaToJSDate(transactionDetails.createdTime).toUTCString()}
        </DetailTextDark>
      </DetailRow>
      {![BraveWallet.TransactionStatus.Rejected, BraveWallet.TransactionStatus.Error].includes(transactionDetails.status) &&
        <DetailRow>
          <DetailTitle>
            {getLocale('braveWalletTransactionDetailHash')}
          </DetailTitle>
          <DetailButton onClick={onClickViewOnBlockExplorer('tx', liveTransaction?.txHash)}>
            {reduceAddress(liveTransaction.txHash)}
          </DetailButton>
        </DetailRow>
      }
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailNetwork')}
        </DetailTitle>
        <DetailTextDark>
          {transactionsNetwork?.chainName ?? ''}
        </DetailTextDark>
      </DetailRow>

      {[BraveWallet.TransactionStatus.Approved, BraveWallet.TransactionStatus.Submitted].includes(transactionDetails.status) &&
        !isSolanaTxn &&
        !isFilecoinTransaction &&
        <DetailRow>
          <DetailTitle />
          <StatusRow>
            <DetailButton onClick={onClickSpeedupTransaction}>{getLocale('braveWalletTransactionDetailSpeedUp')}</DetailButton>
            <SpacerText>|</SpacerText>
            <DetailButton onClick={onClickCancelTransaction}>{getLocale('braveWalletButtonCancel')}</DetailButton>
          </StatusRow>
        </DetailRow>
      }
      {transactionDetails.status === BraveWallet.TransactionStatus.Error &&
        !isSolanaTxn &&
        !isFilecoinTransaction &&
        <DetailRow>
          <DetailTitle />
          <StatusRow>
            <DetailButton onClick={onClickRetryTransaction}>{getLocale('braveWalletTransactionRetry')}</DetailButton>
          </StatusRow>
        </DetailRow>
      }
    </StyledWrapper>
  )
}

export default TransactionDetailPanel

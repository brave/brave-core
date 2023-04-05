// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as EthereumBlockies from 'ethereum-blockies'

// Utils
import { getLocale } from '../../../../common/locale'
import { toProperCase } from '../../../utils/string-utils'
import { getTransactionStatusString } from '../../../utils/tx-utils'
import { formatDateAsRelative, serializedTimeDeltaToJSDate } from '../../../utils/datetime-utils'
import { getNetworkFromTXDataUnion } from '../../../utils/network-utils'
import { reduceAddress } from '../../../utils/reduce-address'
import { WalletSelectors } from '../../../common/selectors'
import Amount from '../../../utils/amount'

// Types
import { BraveWallet, SerializableTransactionInfo } from '../../../constants/types'
import { SwapExchangeProxy } from '../../../common/constants/registry'

// Hooks
import { useTransactionParser } from '../../../common/hooks'
import { useSafeWalletSelector } from '../../../common/hooks/use-safe-selector'

// Components
import { TransactionIntentDescription } from './transaction-intent-description'

// Styled Components
import {
  DetailTextDarkBold,
  DetailTextDark
} from '../shared-panel-styles'
import { StatusBubble } from '../../shared/style'
import {
  DetailColumn,
  FromCircle,
  StatusAndTimeRow,
  StatusRow,
  StyledWrapper,
  ToCircle,
  TransactionDetailRow
} from './style'
import { useGetDefaultNetworksQuery } from '../../../common/slices/api.slice'

export interface Props {
  selectedNetwork?: BraveWallet.NetworkInfo
  transaction: SerializableTransactionInfo
  onSelectTransaction: (transaction: SerializableTransactionInfo) => void
}

const { ERC20Approve, ERC721TransferFrom, ERC721SafeTransferFrom } = BraveWallet.TransactionType

export const TransactionsListItem = ({
  transaction,
  selectedNetwork,
  onSelectTransaction
}: Props) => {
  // redux
  const defaultFiatCurrency = useSafeWalletSelector(WalletSelectors.defaultFiatCurrency)

  // queries
  const { data: defaultNetworks = [] } = useGetDefaultNetworksQuery()

  // methods
  const onClickTransaction = () => {
    onSelectTransaction(transaction)
  }

  // memos & custom hooks
  const transactionsNetwork = React.useMemo(() => {
    return getNetworkFromTXDataUnion(transaction.txDataUnion, defaultNetworks, selectedNetwork)
  }, [defaultNetworks, transaction, selectedNetwork])

  const parseTransaction = useTransactionParser(transactionsNetwork)

  const transactionDetails = React.useMemo(
    () => parseTransaction(transaction),
    [transaction]
  )

  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.sender])

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.recipient.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.recipient])

  const transactionIntentLocale = React.useMemo((): React.ReactNode => {
    // approval
    if (transaction.txType === ERC20Approve) {
      return toProperCase(getLocale('braveWalletApprovalTransactionIntent'))
    }

    // Detect sending to 0x Exchange Proxy
    if (transactionDetails.isSwap) {
      return getLocale('braveWalletSwap')
    }

    // default or when: [ETHSend, ERC20Transfer, ERC721TransferFrom, ERC721SafeTransferFrom].includes(transaction.txType)
    let erc721ID = transaction.txType === ERC721TransferFrom || transaction.txType === ERC721SafeTransferFrom
      ? ' ' + transactionDetails.erc721TokenId
      : ''

    return (
      <DetailTextDark>
        {`${
            toProperCase(getLocale('braveWalletTransactionSent'))
          } ${
            transactionDetails.value
          } ${
            transactionDetails.symbol
          } ${
            erc721ID
          } (${
            new Amount(transactionDetails.fiatValue).formatAsFiat(defaultFiatCurrency) || '...'
          })`}
      </DetailTextDark>
    )
  }, [transaction.txType, transactionDetails, defaultFiatCurrency])

  const transactionIntentDescription = React.useMemo(() => {
    // default or when: [ETHSend, ERC20Transfer, ERC721TransferFrom, ERC721SafeTransferFrom].includes(transaction.txType)
    let from = `${reduceAddress(transactionDetails.sender)} `
    let to = reduceAddress(transactionDetails.recipient)
    const wrapFromText =
      transaction.txType === ERC20Approve ||
      transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy

    if (transaction.txType === ERC20Approve) {
      // Approval
      from = transactionDetails.isApprovalUnlimited
        ? `${getLocale('braveWalletTransactionApproveUnlimited')} ${transactionDetails.symbol}`
        : new Amount(transactionDetails.value).formatAsAsset(undefined, transactionDetails.symbol)
      to = transactionDetails.approvalTargetLabel || ''
    } else if (transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy) {
      // Brave Swap
      // FIXME: Add as new TransactionType on the service side.
      from = `${transactionDetails.value} ${transactionDetails.symbol}`
      to = transactionDetails.recipientLabel
    }

    return <TransactionIntentDescription from={from} to={to} wrapFrom={wrapFromText} />
  }, [transactionDetails])

  // render
  return (
    <StyledWrapper onClick={onClickTransaction}>
      <DetailColumn>
        <TransactionDetailRow>
          <DetailColumn>
            <FromCircle orb={fromOrb} />
            <ToCircle orb={toOrb} />
          </DetailColumn>

          <DetailColumn>
            <span>
              <DetailTextDark>{transactionIntentLocale}</DetailTextDark>&nbsp;
              {transactionIntentDescription}
            </span>
            <StatusAndTimeRow>
              <DetailTextDarkBold>
                {formatDateAsRelative(serializedTimeDeltaToJSDate(transactionDetails.createdTime))}
              </DetailTextDarkBold>

              <StatusRow>
                <StatusBubble status={transactionDetails.status} />
                <DetailTextDarkBold>
                  {getTransactionStatusString(transactionDetails.status)}
                </DetailTextDarkBold>
              </StatusRow>
            </StatusAndTimeRow>
          </DetailColumn>

        </TransactionDetailRow>
      </DetailColumn>

    </StyledWrapper>
  )
}

export default TransactionsListItem

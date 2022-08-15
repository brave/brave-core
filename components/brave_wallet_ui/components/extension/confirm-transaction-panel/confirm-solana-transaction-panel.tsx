// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// types
import { BraveWallet, WalletState } from '../../../constants/types'

// Utils
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'

// Components
import Tooltip from '../../shared/tooltip/index'
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import PanelTab from '../panel-tab'
import NavButton from '../buttons/nav-button'
import { TransactionInfo } from './transaction-info'
import { SolanaTransactionDetailBox } from '../transaction-box/solana-transaction-detail-box'

// Styles
import { Skeleton } from '../../shared/loading-skeleton/styles'
import {
  TabRow,
  URLText,
  WarningBox,
  WarningTitle,
  LearnMoreButton,
  WarningBoxTitleRow
} from '../shared-panel-styles'
import {
  StyledWrapper,
  FromCircle,
  ToCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  TransactionAmountBig,
  TransactionFiatAmountBig,
  MessageBox,
  TransactionTypeText,
  ButtonRow,
  AccountCircleWrapper,
  ArrowIcon,
  FromToRow,
  QueueStepText,
  QueueStepRow,
  QueueStepButton,
  ErrorText,
  GroupBox,
  GroupBoxColumn,
  GroupBoxTitle,
  GroupBoxText,
  GroupEnumeration,
  SmallLoadIcon
} from './style'
import { StatusBubble } from '../../shared/style'

type confirmPanelTabs = 'transaction' | 'details'
export interface Props {
  onConfirm: () => void
  onReject: () => void
}

const onClickLearnMore = () => {
  chrome.tabs.create({ url: 'https://support.brave.com/hc/en-us/articles/5546517853325' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
}

export const ConfirmSolanaTransactionPanel = ({
  onConfirm,
  onReject
}: Props) => {
  // redux
  const {
    activeOrigin,
    defaultCurrencies,
    selectedPendingTransaction: transactionInfo
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const originInfo = transactionInfo?.originInfo ?? activeOrigin

  // custom hooks
  const pendingTxInfo = usePendingTransactions()
  const {
    fromAddress,
    fromOrb,
    isConfirmButtonDisabled,
    isAssociatedTokenAccountCreation,
    queueNextTransaction,
    rejectAllTransactions,
    toOrb,
    transactionDetails,
    transactionQueueNumber,
    transactionsNetwork,
    transactionsQueueLength,
    transactionTitle,
    isSolanaDappTransaction,
    fromAccountName,
    groupTransactions,
    selectedPendingTransactionGroupIndex
  } = pendingTxInfo

  // state
  const [selectedTab, setSelectedTab] = React.useState<confirmPanelTabs>('transaction')

  // methods
  const onSelectTab = React.useCallback(
    (tab: confirmPanelTabs) => () => setSelectedTab(tab),
  [])

  // render
  if (!transactionDetails || !transactionInfo) {
    return <StyledWrapper>
      <Skeleton width={'100%'} height={'100%'} enableAnimation />
    </StyledWrapper>
  }

  return (
    <StyledWrapper>

      <TopRow>
        <NetworkText>{reduceNetworkDisplayName(transactionsNetwork.chainName)}</NetworkText>

        {transactionsQueueLength > 1 &&
          <QueueStepRow>
            <QueueStepText>
              {transactionQueueNumber} {getLocale('braveWalletQueueOf')} {transactionsQueueLength}
            </QueueStepText>
            <QueueStepButton
              onClick={queueNextTransaction}
            >
              {transactionQueueNumber === transactionsQueueLength
                ? getLocale('braveWalletQueueFirst')
                : getLocale('braveWalletQueueNext')
              }
            </QueueStepButton>
          </QueueStepRow>
        }

      </TopRow>

      <AccountCircleWrapper>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
      </AccountCircleWrapper>
      <URLText>
        <CreateSiteOrigin
          originSpec={originInfo.originSpec}
          eTldPlusOne={originInfo.eTldPlusOne}
        />
      </URLText>
      <FromToRow>
        <Tooltip
          text={fromAddress}
          isAddress={true}
          position='left'
        >
          <AccountNameText>{fromAccountName}</AccountNameText>
        </Tooltip>

        {transactionDetails.recipient && transactionDetails.recipient !== fromAddress &&
          <>
            <ArrowIcon />
            <Tooltip
              text={transactionDetails.recipient}
              isAddress={true}
              position='right'
            >
              <AccountNameText>{transactionDetails.recipientLabel}</AccountNameText>
            </Tooltip>
          </>
        }
      </FromToRow>

      <TransactionTypeText>{transactionTitle}</TransactionTypeText>

      {!isSolanaDappTransaction &&
        <>
          <TransactionAmountBig>
            {new Amount(transactionDetails.valueExact)
                .formatAsAsset(undefined, transactionDetails.symbol)
            }
          </TransactionAmountBig>

          <TransactionFiatAmountBig>
            {
              transactionDetails.fiatValue.formatAsFiat(defaultCurrencies.fiat)
            }
          </TransactionFiatAmountBig>
        </>
      }

      {isAssociatedTokenAccountCreation &&
        <WarningBox warningType='warning'>
          <WarningBoxTitleRow>
            <WarningTitle warningType='warning'>
              {getLocale('braveWalletConfirmTransactionAccountCreationFee')}
              <LearnMoreButton
                onClick={onClickLearnMore}
              >
                {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
              </LearnMoreButton>
            </WarningTitle>
          </WarningBoxTitleRow>
        </WarningBox>
      }

      {groupTransactions.length > 0 && selectedPendingTransactionGroupIndex >= 0 && transactionInfo &&
        <GroupBox>
          <GroupBoxColumn>
            <GroupBoxTitle>
              Transaction group
            </GroupBoxTitle>
            {
              groupTransactions.map((txn, idx) =>
                <GroupBoxText dark={selectedPendingTransactionGroupIndex === idx} key={idx}>
                  <GroupEnumeration>
                    [{idx + 1}/{groupTransactions.length}]
                  </GroupEnumeration>

                  <StatusBubble status={txn.txStatus} />

                  {txn.txStatus === BraveWallet.TransactionStatus.Unapproved && getLocale('braveWalletTransactionStatusUnapproved')}
                  {txn.txStatus === BraveWallet.TransactionStatus.Approved && getLocale('braveWalletTransactionStatusApproved')}
                  {txn.txStatus === BraveWallet.TransactionStatus.Rejected && getLocale('braveWalletTransactionStatusRejected')}
                  {txn.txStatus === BraveWallet.TransactionStatus.Submitted && getLocale('braveWalletTransactionStatusSubmitted')}
                  {txn.txStatus === BraveWallet.TransactionStatus.Confirmed && getLocale('braveWalletTransactionStatusConfirmed')}
                  {txn.txStatus === BraveWallet.TransactionStatus.Error && getLocale('braveWalletTransactionStatusError')}
                  {txn.txStatus === BraveWallet.TransactionStatus.Dropped && getLocale('braveWalletTransactionStatusDropped')}

                  {[BraveWallet.TransactionStatus.Approved, BraveWallet.TransactionStatus.Submitted]
                    .includes(txn.txStatus) && <SmallLoadIcon />}
                </GroupBoxText>
              )
            }
          </GroupBoxColumn>
        </GroupBox>
      }

      <TabRow>
        <PanelTab
          isSelected={selectedTab === 'transaction'}
          onSubmit={onSelectTab('transaction')}
          text='Transaction'
        />
        <PanelTab
          isSelected={selectedTab === 'details'}
          onSubmit={onSelectTab('details')}
          text='Details'
        />
      </TabRow>

      <MessageBox
        isDetails={selectedTab === 'details'}
        isApprove={false}
      >

        {selectedTab === 'transaction'
          ? <TransactionInfo />
          : <SolanaTransactionDetailBox
              data={transactionInfo?.txDataUnion?.solanaTxData}
              instructions={transactionDetails.instructions}
              txType={transactionInfo.txType}
            />
        }
      </MessageBox>

      {transactionsQueueLength > 1 &&
        <QueueStepButton
          needsMargin={true}
          onClick={rejectAllTransactions}
        >
          {getLocale('braveWalletQueueRejectAll').replace('$1', transactionsQueueLength.toString())}
        </QueueStepButton>
      }

      {
        [
          transactionDetails?.contractAddressError,
          transactionDetails?.sameAddressError,
          transactionDetails?.missingGasLimitError
        ].map((error, index) => !!error && <ErrorText key={`${index}-${error}`}>{error}</ErrorText>)
      }

      <ButtonRow>
        <NavButton
          buttonType='reject'
          text={getLocale('braveWalletAllowSpendRejectButton')}
          onSubmit={onReject}
        />
        <NavButton
          buttonType='confirm'
          text={getLocale('braveWalletAllowSpendConfirmButton')}
          onSubmit={onConfirm}
          disabled={isConfirmButtonDisabled}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default ConfirmSolanaTransactionPanel

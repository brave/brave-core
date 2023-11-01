// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { WalletSelectors } from '../../../common/selectors'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import {
  useUnsafeWalletSelector //
} from '../../../common/hooks/use-safe-selector'

// Components
import Tooltip from '../../shared/tooltip/index'
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import PanelTab from '../panel-tab'
import { TransactionInfo } from './transaction-info'
import { SolanaTransactionDetailBox } from '../transaction-box/solana-transaction-detail-box'
import { TransactionQueueSteps } from './common/queue'
import { Footer } from './common/footer'

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
  AccountCircleWrapper,
  ArrowIcon,
  FromToRow
} from './style'

type confirmPanelTabs = 'transaction' | 'details'

const onClickLearnMore = () => {
  chrome.tabs.create(
    { url: 'https://support.brave.com/hc/en-us/articles/5546517853325' },
    () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    }
  )
}

export const ConfirmSolanaTransactionPanel = () => {
  // redux
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)
  const defaultCurrencies = useUnsafeWalletSelector(
    WalletSelectors.defaultCurrencies
  )

  // custom hooks
  const {
    fromAccount,
    fromOrb,
    isAssociatedTokenAccountCreation,
    toOrb,
    transactionDetails,
    transactionsNetwork,
    transactionTitle,
    isSolanaDappTransaction,
    selectedPendingTransaction,
    onConfirm,
    onReject,
    queueNextTransaction,
    transactionQueueNumber,
    transactionsQueueLength
  } = usePendingTransactions()
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin

  // state
  const [selectedTab, setSelectedTab] =
    React.useState<confirmPanelTabs>('transaction')

  // methods
  const onSelectTab = React.useCallback(
    (tab: confirmPanelTabs) => () => setSelectedTab(tab),
    []
  )

  // render
  if (
    !transactionDetails ||
    !selectedPendingTransaction ||
    !transactionsNetwork ||
    !fromAccount
  ) {
    return (
      <StyledWrapper>
        <Skeleton
          width={'100%'}
          height={'100%'}
          enableAnimation
        />
      </StyledWrapper>
    )
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{transactionsNetwork.chainName}</NetworkText>
        <TransactionQueueSteps
          queueNextTransaction={queueNextTransaction}
          transactionQueueNumber={transactionQueueNumber}
          transactionsQueueLength={transactionsQueueLength}
        />
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
          text={fromAccount.address}
          isAddress={true}
          position='left'
        >
          <AccountNameText>{fromAccount.name}</AccountNameText>
        </Tooltip>

        {transactionDetails.recipient &&
          transactionDetails.recipient !== fromAccount.address && (
            <>
              <ArrowIcon />
              <Tooltip
                text={transactionDetails.recipient}
                isAddress={true}
                position='right'
              >
                <AccountNameText>
                  {transactionDetails.recipientLabel}
                </AccountNameText>
              </Tooltip>
            </>
          )}
      </FromToRow>

      <TransactionTypeText>{transactionTitle}</TransactionTypeText>

      {!isSolanaDappTransaction && (
        <>
          <TransactionAmountBig>
            {new Amount(transactionDetails.valueExact).formatAsAsset(
              undefined,
              transactionDetails.symbol
            )}
          </TransactionAmountBig>

          <TransactionFiatAmountBig>
            {new Amount(transactionDetails.fiatValue).formatAsFiat(
              defaultCurrencies.fiat
            )}
          </TransactionFiatAmountBig>
        </>
      )}

      {isAssociatedTokenAccountCreation && (
        <WarningBox warningType='warning'>
          <WarningBoxTitleRow>
            <WarningTitle warningType='warning'>
              {getLocale('braveWalletConfirmTransactionAccountCreationFee')}
              <LearnMoreButton onClick={onClickLearnMore}>
                {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
              </LearnMoreButton>
            </WarningTitle>
          </WarningBoxTitleRow>
        </WarningBox>
      )}

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

      <MessageBox isDetails={selectedTab === 'details'}>
        {selectedTab === 'transaction' ? (
          <TransactionInfo />
        ) : (
          <SolanaTransactionDetailBox
            data={selectedPendingTransaction?.txDataUnion?.solanaTxData}
            instructions={transactionDetails.instructions}
            txType={selectedPendingTransaction.txType}
          />
        )}
      </MessageBox>
      <Footer
        onConfirm={onConfirm}
        onReject={onReject}
      />
    </StyledWrapper>
  )
}

export default ConfirmSolanaTransactionPanel

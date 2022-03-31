/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { ExternalWallet } from '../../lib/external_wallet'

import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { ExternalWalletView } from './external_wallet_view'
import { ExternalWalletAction } from './external_wallet_action'
import { RewardsSummary, RewardsSummaryData } from './rewards_summary'
import { PendingRewardsView } from './pending_rewards_view'
import { PlusIcon } from './icons/plus_icon'
import { WalletInfoIcon } from './icons/wallet_info_icon'

import * as style from './wallet_card.style'

const rangeFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short',
  day: 'numeric'
})

interface Props {
  balance: number
  externalWallet: ExternalWallet | null
  earningsThisMonth: number
  earningsLastMonth: number
  nextPaymentDate: number
  exchangeRate: number
  exchangeCurrency: string
  showSummary: boolean
  summaryData: RewardsSummaryData
  autoContributeEnabled: boolean
  onExternalWalletAction: (action: ExternalWalletAction) => void
  onViewPendingTips?: () => void
  onViewStatement?: () => void
}

export function getCurrentMonthRange () {
  const now = new Date(Date.now())
  const start = new Date(now.getFullYear(), now.getMonth(), 1)
  const end = new Date(now.getFullYear(), now.getMonth() + 1, 0)
  return rangeFormatter.format(start) + ' – ' + rangeFormatter.format(end)
}

export function WalletCard (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  function onAddFundsClick () {
    props.onExternalWalletAction('add-funds')
  }

  return (
    <style.root>
      <style.overview>
        <style.balancePanel>
          <ExternalWalletView
            externalWallet={props.externalWallet}
            onExternalWalletAction={props.onExternalWalletAction}
          />
          <style.rewardsBalance>
            <style.balanceHeader>
              {getString('walletYourBalance')}
            </style.balanceHeader>
            <style.batAmount data-test-id='rewards-balance-text'>
              <TokenAmount amount={props.balance} />
            </style.batAmount>
            <style.exchangeAmount>
              <ExchangeAmount
                amount={props.balance}
                rate={props.exchangeRate}
              />
            </style.exchangeAmount>
          </style.rewardsBalance>
        </style.balancePanel>
        <style.earningsPanel>
          <style.dateRange>
            {getCurrentMonthRange()}
          </style.dateRange>
          <style.earningsHeader>
            {getString('walletEstimatedEarnings')}
          </style.earningsHeader>
          <style.batAmount>
              <TokenAmount amount={props.earningsThisMonth} />
            </style.batAmount>
            <style.exchangeAmount>
              ≈ &nbsp;
              <ExchangeAmount
                amount={props.earningsThisMonth}
                rate={props.exchangeRate}
                currency={props.exchangeCurrency}
              />
            </style.exchangeAmount>
        </style.earningsPanel>
      </style.overview>
      {
        props.showSummary
          ? <style.summaryBox>
              <style.summaryActions>
                <style.addFunds>
                  <button onClick={onAddFundsClick}>
                    <PlusIcon />{getString('walletAddFunds')}
                  </button>
                </style.addFunds>
                {
                  props.onViewStatement &&
                    <style.viewStatement>
                      <button
                        onClick={props.onViewStatement}
                        data-test-id='view-statement-button'
                      >
                        <WalletInfoIcon />{getString('walletViewStatement')}
                      </button>
                    </style.viewStatement>
                }
              </style.summaryActions>
              <RewardsSummary
                data={props.summaryData}
                autoContributeEnabled={props.autoContributeEnabled}
                hideAdEarnings={Boolean(props.externalWallet)}
                earningsLastMonth={props.earningsLastMonth}
                nextPaymentDate={props.nextPaymentDate}
                exchangeRate={props.exchangeRate}
                exchangeCurrency={props.exchangeCurrency}
                onViewPendingTips={props.onViewPendingTips}
              />
            </style.summaryBox>
          : <style.pendingBox>
              <PendingRewardsView
                earningsLastMonth={props.earningsLastMonth}
                earningsReceived={props.summaryData.adEarnings > 0}
                nextPaymentDate={props.nextPaymentDate}
              />
            </style.pendingBox>
      }
    </style.root>
  )
}

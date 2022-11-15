/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { ProviderPayoutStatus } from '../../lib/provider_payout_status'
import { PendingRewardsView } from './pending_rewards_view'
import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'

import * as style from './rewards_summary.style'

const monthFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long',
  year: 'numeric'
})

export interface RewardsSummaryData {
  adEarnings: number
  autoContributions: number
  oneTimeTips: number
  monthlyTips: number
  pendingTips: number
}

interface Props {
  data: RewardsSummaryData
  providerPayoutStatus: ProviderPayoutStatus
  autoContributeEnabled: boolean
  hideAdEarnings: boolean
  earningsLastMonth: number
  nextPaymentDate: number
  exchangeRate: number
  exchangeCurrency?: string
  onViewPendingTips?: () => void
}

export function RewardsSummary (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const { data } = props

  function renderRowWithNode (
    amount: number,
    message: React.ReactNode,
    key: string
  ) {
    return (
      <tr>
        <td>{message}</td>
        <td className='amount' data-test-id={`rewards-summary-${key}`}>
          <TokenAmount
            minimumFractionDigits={2}
            amount={amount}
          />
        </td>
        <td className='exchange'>
          <ExchangeAmount
            amount={amount}
            rate={props.exchangeRate}
            currency={props.exchangeCurrency}
          />
        </td>
      </tr>
    )
  }

  function renderRow (amount: number, message: string, key: string) {
    const messageNode = getString(message)
    return renderRowWithNode(amount, messageNode, key)
  }

  function renderPendingTips () {
    if (!data.pendingTips || !props.onViewPendingTips) {
      return null
    }

    return (
      renderRowWithNode(data.pendingTips,
        <style.pendingAction>
          <button
            data-test-id='view-pending-button'
            onClick={props.onViewPendingTips}
          >
            {getString('walletPendingContributions')}
          </button>
        </style.pendingAction>,
        'pending')
    )
  }

  return (
    <style.root>
      <style.header>
        <div>{getString('walletRewardsSummary')}</div>
        <div>{monthFormatter.format(Date.now())}</div>
      </style.header>
      <style.body>
        <style.dataTable>
          <table>
            <tbody>
              {
                // Ad earnings may be hidden to account for the fact that
                // earnings are directly transfered to users that have linked
                // external wallets, and the client may not have knowledge of
                // those transfer amounts. In such a case, displaying zero would
                // be misleading.
                !props.hideAdEarnings &&
                  renderRow(data.adEarnings, 'walletRewardsFromAds', 'ads')
              }
              {
                props.autoContributeEnabled && renderRow(
                  -data.autoContributions, 'walletAutoContribute', 'ac')
              }
              {renderRow(-data.oneTimeTips, 'walletOneTimeTips', 'one-time')}
              {renderRow(-data.monthlyTips, 'walletMonthlyTips', 'monthly')}
              {renderPendingTips()}
            </tbody>
          </table>
        </style.dataTable>
        <PendingRewardsView
          earningsLastMonth={props.earningsLastMonth}
          nextPaymentDate={props.nextPaymentDate}
          providerPayoutStatus={props.providerPayoutStatus}
        />
      </style.body>
    </style.root>
  )
}

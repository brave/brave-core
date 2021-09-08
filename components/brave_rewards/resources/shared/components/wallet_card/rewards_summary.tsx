/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
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
}

interface Props {
  data: RewardsSummaryData
  hideAdEarnings: boolean
  earningsLastMonth: number
  nextPaymentDate: number
  exchangeRate: number
  exchangeCurrency?: string
}

export function RewardsSummary (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const { data } = props

  function renderRow (amount: number, message: string, key: string) {
    return (
      <tr>
        <td>{getString(message)}</td>
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
              {renderRow(-data.autoContributions, 'walletAutoContribute', 'ac')}
              {renderRow(-data.oneTimeTips, 'walletOneTimeTips', 'one-time')}
              {renderRow(-data.monthlyTips, 'walletMonthlyTips', 'monthly')}
            </tbody>
          </table>
        </style.dataTable>
        <PendingRewardsView
          amount={props.earningsLastMonth}
          nextPaymentDate={props.nextPaymentDate}
        />
      </style.body>
    </style.root>
  )
}

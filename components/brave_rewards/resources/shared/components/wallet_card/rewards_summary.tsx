/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { PendingRewardsView } from './pending_rewards_view'
import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'

import * as styles from './rewards_summary.style'

const monthFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long',
  year: 'numeric'
})

export interface RewardsSummaryData {
  grantClaims: number
  adEarnings: number
  autoContributions: number
  oneTimeTips: number
  monthlyTips: number
}

interface Props {
  data: RewardsSummaryData
  earningsLastMonth: number
  nextPaymentDate: Date
  exchangeRate: number
  exchangeCurrency?: string
}

export function RewardsSummary (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const { data } = props

  function renderRow (message: string, amount: number) {
    return (
      <tr>
        <td>{getString(message)}</td>
        <td className='amount'>
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
    <styles.root>
      <styles.header>
        <div>{getString('walletRewardsSummary')}</div>
        <div>{monthFormatter.format(Date.now())}</div>
      </styles.header>
      <styles.body>
        <styles.dataTable>
          <table>
            <thead>
              <tr><th colSpan={3}>{getString('walletHistory')}</th></tr>
            </thead>
            <tbody>
            {renderRow('walletTotalGrantsClaimed', data.grantClaims)}
            {renderRow('walletRewardsFromAds', data.adEarnings)}
            {renderRow('walletAutoContribute', data.autoContributions)}
            {renderRow('walletOneTimeTips', data.oneTimeTips)}
            {renderRow('walletMonthlyTips', data.monthlyTips)}
            </tbody>
          </table>
        </styles.dataTable>
        <PendingRewardsView
          amount={props.earningsLastMonth}
          nextPaymentDate={props.nextPaymentDate}
        />
      </styles.body>
    </styles.root>
  )
}

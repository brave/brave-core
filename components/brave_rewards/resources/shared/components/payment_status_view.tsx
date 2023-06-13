/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../lib/locale_context'
import { ProviderPayoutStatus } from '../lib/provider_payout_status'

import { NewTabLink } from './new_tab_link'
import { EarningsRange } from './earnings_range'
import { MoneyBagIcon } from './icons/money_bag_icon'
import { PaymentCompleteIcon } from './icons/payment_complete_icon'

import * as urls from '../lib/rewards_urls'

const pendingDaysFormatter = new Intl.NumberFormat(undefined, {
  style: 'unit',
  unit: 'day',
  unitDisplay: 'long',
  maximumFractionDigits: 0
})

const monthNameFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long'
})

export function getDaysUntilRewardsPayment (nextPaymentDate: number | Date) {
  if (typeof nextPaymentDate === 'number') {
    nextPaymentDate = new Date(nextPaymentDate)
  }

  // Round next payment date down to midnight local time
  nextPaymentDate = new Date(
    nextPaymentDate.getFullYear(),
    nextPaymentDate.getMonth(),
    nextPaymentDate.getDate())

  const now = Date.now()

  // Only show pending days when payment date is within the current month
  if (nextPaymentDate.getMonth() !== new Date(now).getMonth()) {
    return ''
  }

  const delta = nextPaymentDate.getTime() - now
  const days = Math.ceil(delta / 24 / 60 / 60 / 1000)
  if (days < 1) {
    return ''
  }

  return pendingDaysFormatter.format(days)
}

function getPaymentMonth () {
  const date = new Date()
  const lastMonth = new Date(date.getFullYear(), date.getMonth() - 1)
  return monthNameFormatter.format(lastMonth)
}

interface RewardAmountProps {
  min: number
  max: number
}

function RewardAmount (props: RewardAmountProps) {
  return (
    <span className='rewards-payment-amount'>
      <EarningsRange
        minimum={props.min}
        maximum={props.max}
        minimumFractionDigits={3}
      />
    </span>
  )
}

interface Props {
  minEarnings: number
  maxEarnings: number
  nextPaymentDate: number
  providerPayoutStatus: ProviderPayoutStatus
}

export function PaymentStatusView (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  if (props.minEarnings <= 0) {
    return null
  }

  const estimatedPendingDays = getDaysUntilRewardsPayment(props.nextPaymentDate)
  if (estimatedPendingDays) {
    return (
      <div className='rewards-payment-pending'>
        <div><MoneyBagIcon /></div>
        <div>
          {
            formatMessage(getString('rewardsPaymentPending'), [
              getPaymentMonth(),
              <RewardAmount
                key='amount'
                min={props.minEarnings}
                max={props.maxEarnings}
              />,
              estimatedPendingDays
            ])
          }
        </div>
      </div>
    )
  }

  if (props.providerPayoutStatus === 'complete') {
    return (
      <div className='rewards-payment-completed'>
        <div><PaymentCompleteIcon /></div>
        <div>
          {
            formatMessage(getString('rewardsPaymentCompleted'), [
              getPaymentMonth()
            ])
          }
        </div>
      </div>
    )
  }

  if (props.providerPayoutStatus === 'processing') {
    return (
      <div className='rewards-payment-processing'>
        <div>
          {
            formatMessage(getString('rewardsPaymentProcessing'), [
              getPaymentMonth(),
              <RewardAmount
                key='amount'
                min={props.minEarnings}
                max={props.maxEarnings}
              />
            ])
          }&nbsp;
          <span className='rewards-payment-check-status'>
            <NewTabLink key='status' href={urls.payoutStatusURL}>
              {getString('rewardsPaymentCheckStatus')}
            </NewTabLink>
          </span>
        </div>
      </div>
    )
  }

  return null
}

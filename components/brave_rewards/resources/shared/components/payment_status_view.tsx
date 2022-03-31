/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../lib/locale_context'

import { NewTabLink } from './new_tab_link'
import { TokenAmount } from './token_amount'
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

const completedMessageDaysVisible = 3

export function shouldShowRewardsPaymentCompleted (
  nextPaymentDate: number | Date
) {
  if (typeof nextPaymentDate === 'number') {
    nextPaymentDate = new Date(nextPaymentDate)
  }

  const now = Date.now()
  if (nextPaymentDate.getTime() < now) {
    return false
  }

  const days = new Date(now).getDate() - nextPaymentDate.getDate()
  return days >= 0 && days <= completedMessageDaysVisible
}

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
  amount: number
}

function RewardAmount (props: RewardAmountProps) {
  return (
    <span className='rewards-payment-amount'>
      <span className='plus'>+</span>
      <TokenAmount
        minimumFractionDigits={1}
        amount={props.amount}
      />
    </span>
  )
}

interface Props {
  earningsLastMonth: number
  earningsReceived: boolean
  nextPaymentDate: number
}

export function PaymentStatusView (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  if (props.earningsLastMonth <= 0) {
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
              <RewardAmount key='amount' amount={props.earningsLastMonth} />,
              getPaymentMonth(),
              estimatedPendingDays
            ])
          }
        </div>
      </div>
    )
  }

  if (!shouldShowRewardsPaymentCompleted(props.nextPaymentDate)) {
    return null
  }

  if (props.earningsReceived) {
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

  return (
    <div className='rewards-payment-processing'>
      <div>
        {
          formatMessage(getString('rewardsPaymentProcessing'), [
            <RewardAmount key='amount' amount={props.earningsLastMonth} />,
            getPaymentMonth()
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

export function shouldRenderPendingRewards (
  earningsLastMonth: number,
  nextPaymentDate: number
) {
  return earningsLastMonth > 0 && (
    getDaysUntilRewardsPayment(nextPaymentDate) ||
    shouldShowRewardsPaymentCompleted(nextPaymentDate))
}

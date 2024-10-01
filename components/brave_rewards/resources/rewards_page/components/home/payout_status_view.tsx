/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useAppState } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { formatMessage } from '../../../shared/lib/locale_context'

import {
  getProviderPayoutStatus,
  getDaysUntilPayout,
  truncatePayoutDate
} from '../../../shared/lib/provider_payout_status'

import { NewTabLink } from '../../../shared/components/new_tab_link'
import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './payout_status_view.style'

const dateFormatter = Intl.DateTimeFormat(undefined, {
  month: 'long',
  day: 'numeric'
})

const monthNameFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long'
})

function formatPayoutDate(nextPayoutDate: number | Date) {
  return dateFormatter.format(truncatePayoutDate(nextPayoutDate))
}

function getPayoutMonth() {
  const date = new Date()
  const lastMonth = new Date(date.getFullYear(), date.getMonth() - 1)
  return monthNameFormatter.format(lastMonth)
}

export function PayoutStatusView() {
  const { getString } = useLocaleContext()

  const [parameters, adsInfo, externalWallet] = useAppState((state) => [
    state.rewardsParameters,
    state.adsInfo,
    state.externalWallet
  ])

  if (!parameters || !adsInfo || !externalWallet) {
    return null
  }

  if (adsInfo.minEarningsPreviousMonth <= 0) {
    return null
  }

  const payoutStatus = getProviderPayoutStatus(
    parameters.payoutStatus,
    externalWallet.provider)

  if (payoutStatus === 'complete') {
    return (
      <div {...style}>
        <Icon name='info-filled' />
        <div>
          {formatMessage(getString('payoutCompletedText'), [getPayoutMonth()])}
          &nbsp;
          <span className='rewards-payment-link'>
            <NewTabLink href={urls.contactSupportURL}>
              {getString('payoutSupportLink')}
            </NewTabLink>
          </span>
        </div>
      </div>
    )
  }

  if (payoutStatus === 'processing') {
    return (
      <div {...style}>
        <Icon name='info-filled' />
        <div>
          {
            formatMessage(getString('payoutProcessingText'), [
              getPayoutMonth()
            ])
          }&nbsp;
          <span className='rewards-payment-link'>
            <NewTabLink href={urls.payoutStatusURL}>
              {getString('payoutCheckStatusLink')}
            </NewTabLink>
          </span>
        </div>
      </div>
    )
  }

  const estimatedPendingDays = getDaysUntilPayout(adsInfo.nextPaymentDate)
  if (estimatedPendingDays) {
    return (
      <div {...style}>
        <Icon name='info-filled' />
        <div>
          {
            formatMessage(getString('payoutPendingText'), [
              getPayoutMonth(),
              formatPayoutDate(adsInfo.nextPaymentDate)
            ])
          }
        </div>
      </div>
    )
  }

  return null
}

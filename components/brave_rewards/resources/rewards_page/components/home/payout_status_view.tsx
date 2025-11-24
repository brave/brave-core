/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useAppState } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { formatString } from '$web-common/formatString'

import {
  getProviderPayoutStatus,
  getDaysUntilPayout,
  truncatePayoutDate,
} from '../../../shared/lib/provider_payout_status'

import { NewTabLink } from '../../../shared/components/new_tab_link'
import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './payout_status_view.style'

const dateFormatter = Intl.DateTimeFormat(undefined, {
  month: 'long',
  day: 'numeric',
})

const monthNameFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long',
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

  const parameters = useAppState((state) => state.rewardsParameters)
  const adsInfo = useAppState((state) => state.adsInfo)
  const externalWallet = useAppState((state) => state.externalWallet)

  if (!parameters || !adsInfo || !externalWallet) {
    return null
  }

  if (adsInfo.minEarningsPreviousMonth <= 0) {
    return null
  }

  const payoutStatus = getProviderPayoutStatus(
    parameters.payoutStatus,
    externalWallet.provider,
  )

  if (payoutStatus === 'complete') {
    return (
      <div data-css-scope={style.scope}>
        <Icon name='info-filled' />
        <div>
          {formatString(getString('payoutCompletedText'), [getPayoutMonth()])}
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
      <div data-css-scope={style.scope}>
        <Icon name='info-filled' />
        <div>
          {formatString(getString('payoutProcessingText'), [getPayoutMonth()])}
          &nbsp;
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
      <div data-css-scope={style.scope}>
        <Icon name='info-filled' />
        <div>
          {formatString(getString('payoutPendingText'), [
            getPayoutMonth(),
            formatPayoutDate(adsInfo.nextPaymentDate),
          ])}
        </div>
      </div>
    )
  }

  return null
}

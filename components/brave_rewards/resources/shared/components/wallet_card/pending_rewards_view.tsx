/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { PaymentStatusView } from '../payment_status_view'
import { ProviderPayoutStatus } from '../../lib/provider_payout_status'

import * as style from './pending_rewards_view.style'

interface Props {
  minEarnings: number
  maxEarnings: number
  nextPaymentDate: number
  providerPayoutStatus: ProviderPayoutStatus
}

export function PendingRewardsView (props: Props) {
  return (
    <style.root>
      <PaymentStatusView
        minEarnings={props.minEarnings}
        maxEarnings={props.maxEarnings}
        nextPaymentDate={props.nextPaymentDate}
        providerPayoutStatus={props.providerPayoutStatus}
      />
    </style.root>
  )
}

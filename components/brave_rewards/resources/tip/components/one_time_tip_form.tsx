/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  PaymentKind,
  RewardsParameters,
  PublisherInfo,
  BalanceInfo
} from '../lib/interfaces'

import { HostContext } from '../lib/host_context'

import { BatTipForm } from './bat_tip_form'
import { PaymentKindSwitch } from './payment_kind_switch'
import { ExchangeAmount } from './exchange_amount'

import * as style from './one_time_tip_form.style'

function generateTipOptions (
  rewardsParameters: RewardsParameters,
  publisherInfo: PublisherInfo
) {
  const publisherAmounts = publisherInfo.amounts
  if (publisherAmounts && publisherAmounts.length > 0) {
    return publisherAmounts
  }
  const { tipChoices } = rewardsParameters
  if (tipChoices.length > 0) {
    return tipChoices
  }
  return [1, 5, 10]
}

function getDefaultTipAmount (
  rewardsParameters: RewardsParameters | undefined,
  publisherInfo: PublisherInfo | undefined,
  balanceInfo: BalanceInfo | undefined
) {
  if (!rewardsParameters || !publisherInfo || !balanceInfo) {
    return 0
  }
  const options = generateTipOptions(rewardsParameters, publisherInfo)
  if (options.length > 0) {
    // Select the highest amount that is greater than or equal
    // to the user's balance, starting from the middle option.
    for (let i = Math.floor(options.length / 2); i >= 0; --i) {
      if (i === 0 || options[i] <= balanceInfo.total) {
        return options[i]
      }
    }
  }
  return 0
}

export function OneTimeTipForm () {
  const host = React.useContext(HostContext)

  const [balanceInfo, setBalanceInfo] = React.useState(
    host.state.balanceInfo)
  const [rewardsParameters, setRewardsParameters] = React.useState(
    host.state.rewardsParameters)
  const [publisherInfo, setPublisherInfo] = React.useState(
    host.state.publisherInfo)

  const [paymentKind, setPaymentKind] = React.useState<PaymentKind>('bat')

  React.useEffect(() => {
    return host.addListener((state) => {
      setBalanceInfo(state.balanceInfo)
      setRewardsParameters(state.rewardsParameters)
      setPublisherInfo(state.publisherInfo)
    })
  }, [host])

  if (!balanceInfo || !rewardsParameters || !publisherInfo) {
    return null
  }

  const tipOptions = generateTipOptions(rewardsParameters, publisherInfo)

  const tipAmountOptions = tipOptions.map((value) => ({
    value,
    currency: 'BAT',
    exchangeAmount: (
      <ExchangeAmount amount={value} rate={rewardsParameters.rate} />
    )
  }))

  const defaultTipAmount = getDefaultTipAmount(
    rewardsParameters,
    publisherInfo,
    balanceInfo)

  const onSubmitTip = (tipAmount: number) => {
    if (tipAmount > 0) {
      host.processTip(tipAmount, 'one-time')
    }
  }

  return (
    <style.root>
      <style.kind>
        <PaymentKindSwitch
          userBalance={balanceInfo.total}
          currentValue={paymentKind}
          onChange={setPaymentKind}
        />
      </style.kind>
      <style.form>
        <BatTipForm
          tipKind='one-time'
          userBalance={balanceInfo.total}
          tipAmountOptions={tipAmountOptions}
          defaultTipAmount={defaultTipAmount}
          onSubmitTip={onSubmitTip}
        />
      </style.form>
    </style.root>
  )
}

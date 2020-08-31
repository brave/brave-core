/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  BalanceInfo,
  PaymentKind,
  RewardsParameters,
  PublisherInfo
} from '../lib/interfaces'

import { HostContext } from '../lib/host_context'
import { formatFiatAmount } from '../lib/formatting'

import { CurrentMonthlyForm } from './current_monthly_form'
import { FormSubmitButton } from './form_submit_button'
import { PaymentKindSwitch } from './payment_kind_switch'
import { TipAmountSelector } from './tip_amount_selector'
import { BatString } from './bat_string'
import { TermsOfService } from './terms_of_service'
import { CalendarIcon } from './icons/calendar_icon'

import * as style from './monthly_tip_form.style'

function generateTipOptions (
  rewardsParameters: RewardsParameters,
  publisherInfo: PublisherInfo
) {
  const publisherAmounts = publisherInfo.amounts
  if (publisherAmounts && publisherAmounts.length > 0) {
    return publisherAmounts
  }
  const { monthlyTipChoices } = rewardsParameters
  if (monthlyTipChoices.length > 0) {
    return monthlyTipChoices
  }
  return [1, 5, 10]
}

function getDefaultTip (tipOptions: number[], currentAmount: number) {
  if (tipOptions.length === 0) {
    return 0
  }

  if (currentAmount > 0) {
    const i = tipOptions.indexOf(currentAmount)
    if (i >= 0) {
      return tipOptions[i]
    }
  }

  const middle = Math.floor(tipOptions.length / 2)
  return tipOptions[middle]
}

export function MonthlyTipForm () {
  const host = React.useContext(HostContext)
  const { getString } = host

  const [balanceInfo, setBalanceInfo] = React.useState<BalanceInfo | undefined>()
  const [paymentKind, setPaymentKind] = React.useState<PaymentKind>('bat')
  const [rewardsParameters, setRewardsParameters] = React.useState<RewardsParameters | undefined>()
  const [publisherInfo, setPublisherInfo] = React.useState<PublisherInfo | undefined>()
  const [tipOptions, setTipOptions] = React.useState<number[]>([])
  const [tipAmount, setTipAmount] = React.useState<number>(0)
  const [currentMonthlyTip, setCurrentMonthlyTip] = React.useState<number>(0)
  const [nextReconcileDate, setNextReconcileDate] = React.useState<Date | undefined>()

  const [changeAmountSelected, setChangeAmountSelected] = React.useState<boolean>(() => {
    return host.getDialogArgs().entryPoint === 'set-monthly'
  })

  React.useEffect(() => {
    return host.addListener((state) => {
      setBalanceInfo(state.balanceInfo)
      setRewardsParameters(state.rewardsParameters)
      setPublisherInfo(state.publisherInfo)
      setNextReconcileDate(state.nextReconcileDate)
      setCurrentMonthlyTip(state.currentMontlyTip || 0)
    })
  }, [host])

  // Generate tip options when parameters and publisher info are available
  React.useEffect(() => {
    const optionSelected = tipAmount !== 0
    if (!optionSelected && rewardsParameters && publisherInfo) {
      setTipOptions(generateTipOptions(rewardsParameters, publisherInfo))
    }
  }, [rewardsParameters, publisherInfo, tipAmount])

  // Select a default tip amount
  React.useEffect(() => {
    if (tipAmount === 0 && tipOptions.length > 0) {
      setTipAmount(getDefaultTip(tipOptions, currentMonthlyTip))
    }
  }, [tipOptions, currentMonthlyTip])

  if (!balanceInfo || !rewardsParameters || !publisherInfo) {
    return null
  }

  if (currentMonthlyTip > 0 && !changeAmountSelected) {
    const onCancel = () => { host.processTip(0, 'monthly') }
    const onChange = () => { setChangeAmountSelected(true) }
    return (
      <CurrentMonthlyForm
        currentMonthlyTip={currentMonthlyTip}
        nextReconcileDate={nextReconcileDate}
        onCancelTip={onCancel}
        onChangeAmount={onChange}
      />
    )
  }

  function processTip () {
    if (tipAmount > 0) {
      host.processTip(tipAmount, 'monthly')
    }
  }

  const tipAmountOptions = tipOptions.map(
    (value) => ({
      value,
      amount: value.toFixed(0),
      currency: <BatString />,
      fiatAmount: formatFiatAmount(value, rewardsParameters.rate)
    })
  )

  return (
    <style.root>
      <style.main>
        <PaymentKindSwitch
          userBalance={balanceInfo.total}
          currentValue={paymentKind}
          onChange={setPaymentKind}
        />
        <style.amounts>
          <TipAmountSelector
            options={tipAmountOptions}
            selectedValue={tipAmount}
            onSelect={setTipAmount}
          />
        </style.amounts>
      </style.main>
      <style.footer>
        <TermsOfService />
        <style.submit>
          <FormSubmitButton onClick={processTip}>
            <CalendarIcon /> {getString('doMonthly')}
          </FormSubmitButton>
        </style.submit>
      </style.footer>
    </style.root>
  )
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { PaymentKind, RewardsParameters, PublisherInfo } from '../lib/interfaces'
import { HostContext } from '../lib/host_context'
import { LocaleContext } from '../../shared/lib/locale_context'
import { formatExchangeAmount } from '../lib/formatting'

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

function getDefaultTipAmount (
  rewardsParameters: RewardsParameters | undefined,
  publisherInfo: PublisherInfo | undefined,
  currentAmount: number
) {
  if (!rewardsParameters || !publisherInfo) {
    return 0
  }

  const options = generateTipOptions(rewardsParameters, publisherInfo)

  if (currentAmount > 0) {
    // Select the current monthly tip amount, if available
    const i = options.indexOf(currentAmount)
    if (i >= 0) {
      return options[i]
    }
  }

  // Select the middle amount
  const middle = Math.floor(options.length / 2)
  return options[middle]
}

export function MonthlyTipForm () {
  const host = React.useContext(HostContext)
  const { getString } = React.useContext(LocaleContext)

  const [balanceInfo, setBalanceInfo] = React.useState(
    host.state.balanceInfo)
  const [rewardsParameters, setRewardsParameters] = React.useState(
    host.state.rewardsParameters)
  const [publisherInfo, setPublisherInfo] = React.useState(
    host.state.publisherInfo)
  const [currentMonthlyTip, setCurrentMonthlyTip] = React.useState(
    host.state.currentMonthlyTip || 0)
  const [nextReconcileDate, setNextReconcileDate] = React.useState(
    host.state.nextReconcileDate)

  const [paymentKind, setPaymentKind] = React.useState<PaymentKind>('bat')

  const [tipAmount, setTipAmount] = React.useState(() => {
    return getDefaultTipAmount(
      rewardsParameters,
      publisherInfo,
      currentMonthlyTip)
  })

  const [changeAmountSelected, setChangeAmountSelected] = React.useState(
    host.getDialogArgs().entryPoint === 'set-monthly')

  React.useEffect(() => {
    return host.addListener((state) => {
      setBalanceInfo(state.balanceInfo)
      setRewardsParameters(state.rewardsParameters)
      setPublisherInfo(state.publisherInfo)
      setNextReconcileDate(state.nextReconcileDate)
      setCurrentMonthlyTip(state.currentMonthlyTip || 0)
    })
  }, [host])

  React.useEffect(() => {
    // Select a default tip amount
    if (tipAmount === 0) {
      setTipAmount(getDefaultTipAmount(
        rewardsParameters,
        publisherInfo,
        currentMonthlyTip))
    }
  }, [rewardsParameters, publisherInfo, currentMonthlyTip])

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

  const tipOptions = generateTipOptions(rewardsParameters, publisherInfo)

  const tipAmountOptions = tipOptions.map((value) => ({
    value,
    currency: <BatString />,
    exchangeAmount: formatExchangeAmount(value, rewardsParameters.rate)
  }))

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

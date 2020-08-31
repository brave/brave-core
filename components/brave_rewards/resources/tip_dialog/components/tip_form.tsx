/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  Host,
  BalanceInfo,
  PublisherInfo,
  RewardsParameters,
  TipKind
} from '../lib/interfaces'

import { HostContext } from '../lib/host_context'

import { SliderSwitch, SliderSwitchOption } from './slider_switch'
import { FormSlider } from './form_slider'
import { TipComplete } from './tip_complete'
import { OneTimeTipForm } from './one_time_tip_form'
import { MonthlyTipForm } from './monthly_tip_form'

import * as style from './tip_form.style'

function getTipKindOptions (
  host: Host,
  showMonthlyStar: boolean
): SliderSwitchOption<TipKind>[] {
  let monthlyText = host.getString('monthlyText')
  if (showMonthlyStar) {
    monthlyText = '* ' + monthlyText
  }
  return [
    { value: 'one-time', content: host.getString('oneTimeTip') },
    { value: 'monthly', content: monthlyText }
  ]
}

export function TipForm () {
  const host = React.useContext(HostContext)
  const { getString } = host

  const [balanceInfo, setBalanceInfo] = React.useState<BalanceInfo | undefined>()
  const [publisherInfo, setPublisherInfo] = React.useState<PublisherInfo | undefined>()
  const [rewardsParameters, setRewardsParameters] = React.useState<RewardsParameters | undefined>()
  const [tipAmount, setTipAmount] = React.useState<number>(0)
  const [tipProcessed, setTipProcessed] = React.useState<boolean>(false)
  const [currentMonthlyTip, setCurrentMonthlyTip] = React.useState<number>(0)
  const [wasMonthlySelected, setWasMonthlySelected] = React.useState<boolean>(false)

  const [tipKind, setTipKind] = React.useState<TipKind>(() => {
    const { entryPoint } = host.getDialogArgs()
    return entryPoint === 'one-time' ? 'one-time' : 'monthly'
  })

  React.useEffect(() => {
    return host.addListener((state) => {
      setTipProcessed(Boolean(state.tipProcessed))
      setTipAmount(state.tipAmount || 0)
      setRewardsParameters(state.rewardsParameters)
      setPublisherInfo(state.publisherInfo)
      setBalanceInfo(state.balanceInfo)
      setCurrentMonthlyTip(state.currentMontlyTip || 0)
    })
  }, [host])

  if (!rewardsParameters || !publisherInfo || !balanceInfo) {
    return <style.loading />
  }

  if (tipProcessed) {
    return <TipComplete tipKind={tipKind} tipAmount={tipAmount} />
  }

  function onTipKindSelect (value: TipKind) {
    if (value === 'monthly') {
      setWasMonthlySelected(true)
    }
    setTipKind(value)
  }

  const showMonthlyIndicator =
    !wasMonthlySelected &&
    host.getDialogArgs().entryPoint === 'one-time' &&
    currentMonthlyTip > 0

  return (
    <style.root>
      <style.header>
        {getString('supportThisCreator')}
      </style.header>
      <style.tipKind>
        {
          showMonthlyIndicator &&
            <style.monthlyIndicator>
              <style.monthlyIndicatorStar>*</style.monthlyIndicatorStar>&nbsp;
              {getString('currentlySupporting')}
            </style.monthlyIndicator>
        }
        <SliderSwitch<TipKind>
          options={getTipKindOptions(host, showMonthlyIndicator)}
          selectedValue={tipKind}
          onSelect={onTipKindSelect}
        />
      </style.tipKind>
      <style.main>
        <FormSlider activeForm={tipKind === 'one-time' ? 0 : 1}>
          <OneTimeTipForm />
          <MonthlyTipForm />
        </FormSlider>
      </style.main>
    </style.root>
  )
}

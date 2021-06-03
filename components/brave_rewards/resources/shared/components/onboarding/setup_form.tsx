/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { RewardsTourProps } from './rewards_tour_props'
import * as style from './setup_form.style'

const adsPerHourOptions = [1, 2, 3, 4, 5]

function getAutoContributeOptions (options: number[], initialAmount: number) {
  // Create a sorted list of options containing |initialAmount|
  const set = new Set(options)
  set.add(initialAmount)
  const list = [...set].sort((a, b) => a - b)

  // Return a list with at most 4 options, with the initial amount
  // at the 3rd position if possible
  const pos = list.indexOf(initialAmount)
  let start = 0
  if (pos > 2) {
    start = Math.min(pos - 2, Math.max(0, list.length - 4))
  }
  return list.slice(start, start + 4)
}

export function SetupForm (props: RewardsTourProps) {
  const { getString } = React.useContext(LocaleContext)
  const [initialAmount] = React.useState(props.autoContributeAmount)
  const acOptions = getAutoContributeOptions(
    props.autoContributeAmountOptions,
    initialAmount)

  return (
    <style.root>
      <style.section>
        <style.label>
          {getString('onboardingSetupAdsHeader')}
        </style.label>
        <style.sublabel>
          {getString('onboardingSetupAdsSubheader')}
        </style.sublabel>
        <style.optionBar>
          {
            adsPerHourOptions.map((value) => {
              const className = 'large-text ' +
                (value === props.adsPerHour ? 'selected' : '')
              const onClick = () => {
                if (value !== props.adsPerHour) {
                  props.onAdsPerHourChanged(value)
                }
              }
              return (
                <button key={value} onClick={onClick} className={className}>
                  {value}
                </button>
              )
            })
          }
        </style.optionBar>
      </style.section>
      <style.section>
        <style.label>
          {getString('onboardingSetupContributeHeader')}
        </style.label>
        <style.sublabel>
          {getString('onboardingSetupContributeSubheader')}
        </style.sublabel>
        <style.optionBar>
          {
            acOptions.map((amount) => {
              const className = amount === props.autoContributeAmount
                ? 'selected'
                : ''
              const onClick = () => {
                if (amount !== props.autoContributeAmount) {
                  props.onAutoContributeAmountChanged(amount)
                }
              }
              return (
                <button key={amount} onClick={onClick} className={className}>
                  <style.acAmount>{amount.toFixed(0)}</style.acAmount>&nbsp;
                  <style.acCurrency>BAT</style.acCurrency>
                </button>
              )
            })
          }
        </style.optionBar>
      </style.section>
    </style.root>
  )
}

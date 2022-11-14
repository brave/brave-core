/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { RewardsTourProps } from './rewards_tour_props'
import * as style from './setup_form.style'

const adsPerHourOptions = [1, 2, 3, 4, 5]

export function SetupForm (props: RewardsTourProps) {
  const { getString, getPluralString } = React.useContext(LocaleContext)
  const rootRef = React.useRef<HTMLDivElement | null>(null)
  const [adsPerHourText, setAdsPerHourText] = React.useState('')

  React.useEffect(() => {
    getPluralString('onboardingSetupAdsPerHour', props.adsPerHour)
      .then(setAdsPerHourText)
  }, [props.adsPerHour])

  React.useEffect(() => {
    if (!rootRef.current) {
      return
    }

    const selected = rootRef.current.querySelector('.selected')
    if (!selected) {
      return
    }

    rootRef.current.style.setProperty(
      '--optionbar-handle-position',
      `${(selected as HTMLElement).offsetLeft}px`)
  }, [props.adsPerHour, rootRef.current])

  return (
    <style.root ref={rootRef}>
      <style.label>
        {getString('onboardingSetupAdsHeader')}
      </style.label>
      <style.sublabel>
        {
          props.canConnectAccount
            ? getString('onboardingSetupAdsText1')
            : getString('onboardingSetupAdsText2')
        }
      </style.sublabel>
      <style.graphic />
      <style.adsPerHour>
        {adsPerHourText}
      </style.adsPerHour>
      <style.optionBar>
        {
          adsPerHourOptions.map((value) => {
            const className = value === props.adsPerHour ? 'selected' : ''
            const onClick = () => {
              if (value !== props.adsPerHour) {
                props.onAdsPerHourChanged(value)
              }
            }
            return (
              <button key={value} onClick={onClick}>
                <style.optionMarker className={className} />
              </button>
            )
          })
        }
        <style.optionHandle />
      </style.optionBar>
      <style.sublabel>
        {getString('onboardingSetupChangeLater')}
      </style.sublabel>
    </style.root>
  )
}

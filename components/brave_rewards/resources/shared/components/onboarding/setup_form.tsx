/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { adsPerHourOptions } from '../../lib/ads_options'
import { LocaleContext } from '../../lib/locale_context'
import { RewardsTourProps } from './rewards_tour_props'
import { Slider } from './slider'

import * as style from './setup_form.style'

export function SetupForm (props: RewardsTourProps) {
  const { getString, getPluralString } = React.useContext(LocaleContext)
  const [adsPerHourText, setAdsPerHourText] = React.useState('')

  React.useEffect(() => {
    let active = true
    getPluralString('onboardingSetupAdsPerHour', props.adsPerHour)
      .then((value) => { active && setAdsPerHourText(value) })
    return () => { active = false }
  }, [props.adsPerHour])

  return (
    <style.root>
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
      <style.slider>
        <Slider
          value={props.adsPerHour}
          options={adsPerHourOptions}
          autoFocus={true}
          onChange={props.onAdsPerHourChanged}
        />
      </style.slider>
      <style.sublabel>
        {getString('onboardingSetupChangeLater')}
      </style.sublabel>
    </style.root>
  )
}

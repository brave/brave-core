/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, createLocaleContextForTesting } from '../../../lib/locale_context'
import { WithThemeVariables } from '../../with_theme_variables'

import { RewardsTourModal } from '../rewards_tour_modal'
import { RewardsOptInModal } from '../rewards_opt_in_modal'
import { SettingsOptInForm } from '../settings_opt_in_form'
import { RewardsTourPromo } from '../rewards_tour_promo'

import { localeStrings } from './locale_strings'

const localeContext = createLocaleContextForTesting(localeStrings)

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

interface StoryWrapperProps {
  style?: React.CSSProperties
  children: React.ReactNode
}

function StoryWrapper (props: StoryWrapperProps) {
  return (
    <LocaleContext.Provider value={localeContext}>
      <WithThemeVariables>
        <div style={props.style || {}}>
          {props.children}
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

function getRewardsTourProps () {
  return {
    firstTimeSetup: true,
    adsPerHour: 3,
    canAutoContribute: true,
    canConnectAccount: true,
    onAdsPerHourChanged: actionLogger('onAdsPerHourChanged'),
    onConnectAccount: actionLogger('onConnectAccount'),
    onDone: actionLogger('onDone')
  }
}

export default {
  title: 'Rewards/Onboarding'
}

export function TourModal () {
  const [adsPerHour, setAdsPerHour] = React.useState(3)

  return (
    <StoryWrapper>
      <RewardsTourModal
        {...getRewardsTourProps()}
        adsPerHour={adsPerHour}
        onAdsPerHourChanged={setAdsPerHour}
        onClose={actionLogger('onClose')}
      />
    </StoryWrapper>
  )
}

export function TourModalWide () {
  return (
    <StoryWrapper>
      <RewardsTourModal
        {...getRewardsTourProps()}
        layout='wide'
        onClose={actionLogger('onClose')}
      />
    </StoryWrapper>
  )
}

TourModalWide.storyName = 'Tour Modal (Wide)'

export function OptInModal () {
  return (
    <StoryWrapper>
      <RewardsOptInModal
        initialView='declare-country'
        availableCountries={['US']}
        result={'success'}
        onEnable={actionLogger('onEnable')}
        onTakeTour={actionLogger('onTakeTour')}
        onHideResult={actionLogger('onHideResult')}
      />
    </StoryWrapper>
  )
}

export function SettingsOptIn () {
  return (
    <StoryWrapper style={{ width: '619px' }}>
      <SettingsOptInForm
        onEnable={actionLogger('onEnable')}
        onTakeTour={actionLogger('onTakeTour')}
      />
    </StoryWrapper>
  )
}

export function TourPromo () {
  return (
    <StoryWrapper style={{ width: '373px' }}>
      <RewardsTourPromo
        onClose={actionLogger('onClose')}
        onTakeTour={actionLogger('onTakeTour')}
      />
    </StoryWrapper>
  )
}

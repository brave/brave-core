/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Locale } from '../../lib/locale_context'
import { RewardsTourProps } from './rewards_tour_props'
import { SetupForm } from './setup_form'
import { ArrowNextIcon } from '../icons/arrow_next_icon'

import * as style from './rewards_tour_panels.style'

interface TourPanel {
  id: string
  heading: React.ReactNode
  text: React.ReactNode
  content?: React.ReactNode
  actions?: React.ReactNode
}

type TourPanelFunction = (locale: Locale, props: RewardsTourProps) => TourPanel

function panelWelcome (locale: Locale) {
  const { getString } = locale
  return {
    id: 'welcome',
    heading: getString('onboardingPanelWelcomeHeader'),
    text: getString('onboardingPanelWelcomeText')
  }
}

function panelAds (locale: Locale) {
  const { getString } = locale
  return {
    id: 'ads',
    heading: getString('onboardingPanelAdsHeader'),
    text: getString('onboardingPanelAdsText')
  }
}

function panelAC (locale: Locale) {
  const { getString } = locale
  return {
    id: 'ac',
    heading: getString('onboardingPanelAcHeader'),
    text: getString('onboardingPanelAcText')
  }
}

function panelTipping (locale: Locale) {
  const { getString } = locale
  return {
    id: 'tipping',
    heading: getString('onboardingPanelTippingHeader'),
    text: getString('onboardingPanelTippingText')
  }
}

function panelSetup (locale: Locale, props: RewardsTourProps) {
  const { getString } = locale
  return {
    id: 'setup',
    heading: getString('onboardingPanelSetupHeader'),
    text: null,
    content: <SetupForm {...props} />
  }
}

function panelComplete (locale: Locale) {
  const { getString } = locale
  return {
    id: 'complete',
    heading: getString('onboardingPanelCompleteHeader'),
    text: getString('onboardingPanelCompleteText')
  }
}

export function getTourPanels (props: RewardsTourProps): TourPanelFunction[] {
  const showConnect = props.firstTimeSetup && props.canConnectAccount
  return [
    panelWelcome,
    panelAds,
    ...(props.canAutoContribute ? [panelAC] : []),
    panelTipping,
    ...(props.firstTimeSetup ? [panelSetup] : []),
    ...(showConnect ? [] : [panelComplete])
  ]
}

export function getConnectWalletPanel (
  locale: Locale,
  props: RewardsTourProps
): TourPanel | null {
  if (!props.firstTimeSetup || !props.canConnectAccount) {
    return null
  }

  const { getString } = locale

  return {
    id: 'connect',
    heading: getString('onboardingPanelCompleteHeader'),
    text: getString('onboardingPanelVerifySubtext'),
    actions: (
      <style.verifyActions>
        <button className='verify-now' onClick={props.onConnectAccount}>
          {getString('onboardingPanelVerifyNow')}<ArrowNextIcon />
        </button>
        <button className='verify-later' onClick={props.onDone}>
          {getString('onboardingPanelVerifyLater')}
        </button>
      </style.verifyActions>
    )
  }
}

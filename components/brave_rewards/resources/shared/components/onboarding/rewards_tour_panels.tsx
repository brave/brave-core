/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Locale } from '../../lib/locale_context'
import { RewardsTourProps } from './rewards_tour_props'
import { SetupForm } from './setup_form'

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

function panelSchedule (locale: Locale) {
  const { getString } = locale
  return {
    id: 'schedule',
    heading: getString('onboardingPanelScheduleHeader'),
    text: getString('onboardingPanelScheduleText')
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

function panelRedeem (locale: Locale) {
  const { getString } = locale
  return {
    id: 'redeem',
    heading: getString('onboardingPanelRedeemHeader'),
    text: getString('onboardingPanelRedeemText')
  }
}

function panelSetup (locale: Locale, props: RewardsTourProps) {
  const { getString } = locale
  return {
    id: 'setup',
    heading: getString('onboardingPanelSetupHeader'),
    text: (
      <style.formText>{getString('onboardingPanelSetupText')}</style.formText>
    ),
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
  const canAutoContribute = props.autoContributeAmountOptions.length > 0

  return [
    panelWelcome,
    panelAds,
    panelSchedule,
    ...(canAutoContribute ? [panelAC] : []),
    panelTipping,
    panelRedeem,
    ...(props.firstTimeSetup && canAutoContribute ? [panelSetup] : []),
    panelComplete
  ]
}

export function getVerifyWalletPanel (
  locale: Locale,
  props: RewardsTourProps
): TourPanel | null {
  if (!props.firstTimeSetup) {
    return null
  }

  let textKey = ''
  let noteKey = ''
  switch (props.externalWalletProvider) {
    case 'bitflyer':
      textKey = 'onboardingPanelBitflyerText'
      noteKey = 'onboardingPanelBitflyerNote'
      break
    default:
      return null
  }

  const { getString } = locale

  return {
    id: props.externalWalletProvider,
    heading: getString('onboardingPanelVerifyHeader'),
    text: (
      <>
        {getString(textKey)}
        <style.verifySubtext>
          {getString('onboardingPanelVerifySubtext')}
          <style.verifyNote>
            {getString(noteKey)}
          </style.verifyNote>
        </style.verifySubtext>
      </>
    ),
    actions: (
      <style.verifyActions>
        <button className='verify-later' onClick={props.onDone}>
          {getString('onboardingPanelVerifyLater')}
        </button>
        <button className='verify-now' onClick={props.onVerifyWalletClick}>
          {getString('onboardingPanelVerifyNow')}
        </button>
      </style.verifyActions>
    )
  }
}

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { NewTabLink } from '../new_tab_link'
import { TermsOfService } from '../terms_of_service'
import { LoadingIcon } from '../icons/loading_icon'
import { ErrorIcon } from './icons/error_icon'
import { CountrySelect } from './country_select'

import * as urls from '../../lib/rewards_urls'

import * as style from './rewards_opt_in.style'

export type OnboardingResult =
  'success' |
  'wallet-generation-disabled' |
  'country-already-declared' |
  'unexpected-error'

type InitialView = 'default' | 'declare-country'

interface Props {
  initialView: InitialView
  result: OnboardingResult | null
  availableCountries: string[]
  defaultCountry: string
  onEnable: (country: string) => void
  onHideResult: () => void
}

export function RewardsOptIn (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [countryCode, setCountryCode] = React.useState('')
  const [processing, setProcessing] = React.useState(false)
  const [showCountrySelect, setShowCountrySelect] =
    React.useState(props.initialView === 'declare-country')

  React.useEffect(() => {
    setProcessing(false)
  }, [props.result])

  function getResultMessages (result: OnboardingResult) {
    if (result === 'success') {
      return {
        header: getString('onboardingGeoSuccessHeader'),
        text: getString('onboardingGeoSuccessText'),
      }
    }

    if (result === 'wallet-generation-disabled') {
      return {
        header: getString('onboardingErrorHeaderDisabled'),
        text: formatMessage(getString('onboardingErrorTextDisabled'), {
          tags: {
            $1: (content) =>
              <NewTabLink
                key='link'
                href='https://support.brave.com/hc/en-us/articles/9312922941069'
              >
                {content}
              </NewTabLink>
          }
        })
      }
    }

    if (result === 'country-already-declared') {
      return {
        header: getString('onboardingErrorHeader'),
        text: getString('onboardingErrorTextDeclareCountry')
      }
    }

    return {
      header: getString('onboardingErrorHeader'),
      text: getString('onboardingErrorText')
    }
  }

  if (props.result) {
    const messages = getResultMessages(props.result)
    if (!messages) {
      return null
    }

    return (
      <style.root>
        <style.header className='onboarding-result'>
          {props.result === 'success'
            ? <style.successIcon /> : <ErrorIcon />}
          {messages.header}
        </style.header>
        <style.text>
          {messages.text}
        </style.text>
        <style.mainAction>
          <button onClick={props.onHideResult}>
            {
              props.result === 'success'
                ? getString('onboardingDone')
                : getString('onboardingClose')
            }
          </button>
        </style.mainAction>
        {
          props.result === 'success'
            ? <style.learnMore className='learn-more-success'>
                <NewTabLink href={urls.rewardsTourURL}>
                  {getString('onboardingHowDoesBraveRewardsWork')}
                </NewTabLink>
              </style.learnMore>
            : <style.errorCode>
                {props.result}
              </style.errorCode>
        }
      </style.root>
    )
  }

  if (showCountrySelect) {
    const onContinueClick = (event: React.UIEvent) => {
      setProcessing(true)
      props.onEnable(countryCode)
    }

    return (
      <style.root>
        <style.geoPinIcon />
        <style.header className='country-select'>
          {getString('onboardingGeoHeader')}
        </style.header>
        <style.text>
          {getString('onboardingGeoText')}
        </style.text>
        <style.selectCountry>
          <CountrySelect
            countries={props.availableCountries}
            defaultCountry={props.defaultCountry}
            placeholderText={getString('onboardingSelectCountry')}
            value={countryCode}
            onChange={setCountryCode}
          />
        </style.selectCountry>
        <style.mainAction>
          <button
            onClick={onContinueClick}
            disabled={!countryCode || processing}
            data-test-id='select-country-button'
          >
            {processing ? <LoadingIcon /> : getString('onboardingContinue')}
          </button>
        </style.mainAction>
      </style.root>
    )
  }

  const onEnableClick = () => {
    setShowCountrySelect(true)
  }

  return (
    <style.root>
      <style.optInIcon />
      <style.optInHeader>
        {getString('onboardingEarnHeader')}
      </style.optInHeader>
      <style.optInText>
        {getString('onboardingEarnText')}
      </style.optInText>
      <style.mainAction>
        <button onClick={onEnableClick} data-test-id='opt-in-button'>
          {getString('onboardingStartUsingRewards')}
        </button>
      </style.mainAction>
      <style.learnMore>
        <NewTabLink href={urls.rewardsTourURL}>
          {getString('onboardingHowDoesItWork')}
        </NewTabLink>
      </style.learnMore>
      <style.terms>
        <TermsOfService />
      </style.terms>
    </style.root>
  )
}

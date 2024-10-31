/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import { EnableRewardsResult, AvailableCountryInfo } from '../../lib/app_state'
import { AppModelContext } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { useCallbackWrapper } from '../../lib/callback_wrapper'
import { formatMessage } from '../../../shared/lib/locale_context'
import { CountrySelectModal } from './country_select_modal'
import { OnboardingErrorModal } from './onboarding_error_modal'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import * as urls from '../../../shared/lib/rewards_urls'
import batCoinImage from '../../assets/bat_coin_color_animated.svg'

import { style } from './onboarding.style'

interface Props {
  onOnboardingCompleted: () => void
}

export function Onboarding(props: Props) {
  const wrapCallback = useCallbackWrapper()
  const model = React.useContext(AppModelContext)
  const { getString } = useLocaleContext()

  const [availableCountries, setAvailableCountries] =
    React.useState<AvailableCountryInfo>({
      countryCodes: [],
      defaultCountryCode: ''
    })

  const [showCountrySelect, setShowCountrySelect] = React.useState(false)
  const [isLoading, setLoading] = React.useState(false)
  const [enableRewardsResult, setEnableRewardsResult] =
    React.useState<EnableRewardsResult | null>(null)

  React.useEffect(() => {
    model.getAvailableCountries().then(wrapCallback(setAvailableCountries))
  }, [])

  function onCountrySelectClose() {
    setShowCountrySelect(false)
  }

  function onCountrySelected(country: string) {
    setLoading(true)
    model.enableRewards(country).then(wrapCallback((result) => {
      setLoading(false)
      setEnableRewardsResult(result)
      if (result === 'success') {
        props.onOnboardingCompleted()
      } else {
        setShowCountrySelect(false)
      }
    }))
  }

  function onStartClick() {
    setShowCountrySelect(true)
  }

  function clearResult() {
    setEnableRewardsResult(null)
  }

  function renderModal() {
    if (enableRewardsResult && enableRewardsResult !== 'success') {
      return (
        <OnboardingErrorModal
          result={enableRewardsResult}
          onClose={clearResult}
        />
      )
    }

    if (showCountrySelect) {
      return (
        <CountrySelectModal
          countries={availableCountries.countryCodes}
          defaultCountry={availableCountries.defaultCountryCode}
          loading={isLoading}
          onClose={onCountrySelectClose}
          onCountrySelected={onCountrySelected}
        />
      )
    }

    return null
  }

  return (
    <div className='onboarding' {...style}>
      <div className='brave-rewards-logo' />
      <div className='bat-image'>
        <img alt='BAT' src={batCoinImage} />
      </div>
      <div className='header'>
        {getString('onboardingTitle')}
      </div>
      <div className='text'>
        <div>
          <Icon name='check-normal' />
          <div>{getString('onboardingTextItem1')}</div>
        </div>
        <div>
          <Icon name='check-normal' />
          <div>{getString('onboardingTextItem2')}</div>
        </div>
      </div>
      <div className='action'>
        <Button className='onboarding-button' onClick={onStartClick}>
          {getString('onboardingButtonLabel')}
        </Button>
      </div>
      <div className='learn-more'>
        <NewTabLink href={urls.rewardsTourURL}>
          {getString('onboardingLearnMoreLabel')}
        </NewTabLink>
      </div>
      <div className='terms'>
        {
          formatMessage(getString('onboardingTermsText'), {
            tags: {
              $1: (content) => (
                <NewTabLink key='terms' href={urls.termsOfServiceURL}>
                  {content}
                </NewTabLink>
              ),
              $3: (content) => (
                <NewTabLink key='privacy' href={urls.privacyPolicyURL}>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
      </div>
      {renderModal()}
    </div>
  )
}

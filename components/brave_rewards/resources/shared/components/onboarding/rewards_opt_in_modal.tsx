/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { Modal } from '../modal'
import { TermsOfService } from '../terms_of_service'
import { NewTabLink } from '../new_tab_link'
import { BatIcon } from '../icons/bat_icon'
import { LoadingIcon } from '../icons/loading_icon'
import { ErrorIcon } from './icons/error_icon'
import { SuccessIcon } from './icons/success_icon'
import { GeoPinIcon } from '../icons/geo_pin_icon'
import { CountrySelect, getCountryName } from './country_select'
import { privacyPolicyURL } from '../../lib/rewards_urls'

import * as style from './rewards_opt_in_modal.style'

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
  onTakeTour: () => void
  onEnable: (country: string) => void
  onHideResult: () => void
}

export function RewardsOptInModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [countryCode, setCountryCode] = React.useState('')
  const [processing, setProcessing] = React.useState(false)
  const [initialView] = React.useState(props.initialView)
  const [showCountrySelect, setShowCountrySelect] =
    React.useState(props.initialView === 'declare-country')

  React.useEffect(() => {
    setProcessing(false)

    // Only the "declare-country" view displays a success message to the user.
    if (props.result === 'success' && initialView !== 'declare-country') {
      props.onHideResult()
    }
  }, [initialView, props.result])

  function getResultMessages (result: OnboardingResult) {
    if (result === 'success') {
      if (initialView === 'declare-country') {
        return {
          header: getString('onboardingGeoSuccessHeader'),
          text: formatMessage(getString('onboardingGeoSuccessText'), [
            <strong key='country'>{getCountryName(countryCode)}</strong>
          ])
        }
      }
      return null
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

    if (initialView === 'declare-country') {
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
      <Modal>
        <style.root>
          <style.header className='onboarding-result'>
            {props.result === 'success' ? <SuccessIcon /> : <ErrorIcon />}
            {messages.header}
          </style.header>
          <style.text>
            {messages.text}
          </style.text>
          <style.mainAction>
            <button onClick={props.onHideResult}>
              {getString('onboardingClose')}
            </button>
          </style.mainAction>
          <style.errorCode>
            {props.result !== 'success' && props.result}
          </style.errorCode>
        </style.root>
      </Modal>
    )
  }

  if (showCountrySelect) {
    const onContinueClick = (event: React.UIEvent) => {
      setProcessing(true)
      props.onEnable(countryCode)
    }

    const text = props.initialView === 'declare-country'
      ? getString('onboardingGeoTextDeclareCountry')
      : getString('onboardingGeoText')

    return (
      <Modal>
        <style.root>
          <style.header className='country-select'>
            <GeoPinIcon />{getString('onboardingGeoHeader')}
          </style.header>
          <style.text>
            {
              formatMessage(text, {
                tags: {
                  $1: (content) =>
                    <NewTabLink key='link' href={privacyPolicyURL}>
                      {content}
                    </NewTabLink>
                }
              })
            }
          </style.text>
          <style.selectCountry>
            <CountrySelect
              countries={props.availableCountries}
              placeholderText={getString('onboardingSelectCountry')}
              value={countryCode}
              onChange={setCountryCode}
              autoFocus
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
          <style.terms>
            <TermsOfService />
          </style.terms>
        </style.root>
      </Modal>
    )
  }

  const onEnableClick = () => {
    setShowCountrySelect(true)
  }

  return (
    <Modal>
      <style.root>
        <style.header>
          <BatIcon />{getString('onboardingEarnHeader')}
        </style.header>
        <style.text>
          {getString('onboardingEarnText')}
        </style.text>
        <style.mainAction>
          <button
            onClick={onEnableClick}
            data-test-id='opt-in-button'
            autoFocus
          >
            {getString('onboardingStartUsingRewards')}
          </button>
        </style.mainAction>
        <style.takeTour>
          <button onClick={props.onTakeTour}>
            {getString('onboardingTakeTour')}
          </button>
        </style.takeTour>
        <style.terms>
          <TermsOfService />
        </style.terms>
      </style.root>
    </Modal>
  )
}

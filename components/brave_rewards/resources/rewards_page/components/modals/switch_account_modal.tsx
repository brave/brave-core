/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { EnableRewardsResult, AvailableCountryInfo } from '../../lib/app_state'
import { useLocaleContext } from '../../lib/locale_strings'
import formatMessage from '$web-common/formatMessage'
import { AppModelContext } from '../../lib/app_model_context'
import { getExternalWalletProviderName } from '../../../shared/lib/external_wallet'
import { BatIcon } from '../../../shared/components/icons/bat_icon'
import { WalletProviderIcon } from '../../../shared/components/icons/wallet_provider_icon'
import { OnboardingErrorModal } from '../onboarding/onboarding_error_modal'
import { CountrySelect } from '../onboarding/country_select'
import { Modal } from '../common/modal'
import { RouterContext } from '../../lib/router'
import * as routes from '../../lib/app_routes'

import { style } from './switch_account_modal.style'

interface Props {
  onClose: () => void
}

export function SwitchAccountModal(props: Props) {
  const { getString } = useLocaleContext()
  const router = React.useContext(RouterContext)
  const model = React.useContext(AppModelContext)
  const [loading, setLoading] = React.useState(false)

  const [countryCode, setCountryCode] = React.useState(() => {
    return model.getState().countryCode
  })

  const [provider] = React.useState(() => {
    return model.getState().externalWallet?.provider ?? null
  })

  const [enableRewardsResult, setEnableRewardsResult] =
    React.useState<EnableRewardsResult | null>(null)

  const [availableCountries, setAvailableCountries] =
    React.useState<AvailableCountryInfo>({
      countryCodes: [],
      defaultCountryCode: '',
    })

  React.useEffect(() => {
    model.getAvailableCountries().then(setAvailableCountries)
  }, [])

  React.useEffect(() => {
    if (!provider) {
      props.onClose()
    }
  }, [])

  async function resetAndEnableRewards() {
    setLoading(true)
    await model.resetRewards()
    const result = await model.enableRewards(countryCode)
    setLoading(false)
    if (result === 'success') {
      router.setRoute(routes.connectAccount)
      // Delay closing the modal in order to smooth the transition from here to
      // the connect account UI.
      await new Promise((resolve) => setTimeout(resolve, 300))
      props.onClose()
    } else {
      setEnableRewardsResult(result)
    }
  }

  if (enableRewardsResult && enableRewardsResult !== 'success') {
    return (
      <OnboardingErrorModal
        result={enableRewardsResult}
        onClose={props.onClose}
      />
    )
  }

  if (!provider) {
    return null
  }

  const providerName = getExternalWalletProviderName(provider)

  return (
    <Modal onEscape={props.onClose}>
      <Modal.Header
        title={getString('resetRewardsTitle')}
        onClose={props.onClose}
      />
      <div data-css-scope={style.scope}>
        <div className='icon-header'>
          <span className='bat'>
            <BatIcon />
          </span>
          <span className='link-broken'>
            <Icon name='link-broken' />
          </span>
          <span className='provider'>
            <WalletProviderIcon provider={provider} />
          </span>
        </div>
        <h4>
          {formatMessage(getString('switchAccountTitle'), [providerName])}
        </h4>
        <ul>
          <li>
            {formatMessage(getString('switchAccountText1'), [providerName])}
          </li>
          <li>{getString('switchAccountText2')}</li>
          <li>{getString('switchAccountText3')}</li>
        </ul>
        <div className='region-select'>
          <label>{getString('switchAccountCountrySelectLabel')}</label>
          <CountrySelect
            countries={availableCountries.countryCodes}
            placeholderText={getString('countrySelectPlaceholder')}
            value={countryCode}
            onChange={setCountryCode}
          />
        </div>
      </div>
      <Modal.Actions
        actions={[
          {
            text: getString('cancelButtonLabel'),
            onClick: props.onClose,
          },
          {
            className: 'reset-button',
            text: getString('resetButtonLabel'),
            onClick: resetAndEnableRewards,
            isDisabled: !countryCode || loading,
            isPrimary: true,
          },
        ]}
      />
    </Modal>
  )
}

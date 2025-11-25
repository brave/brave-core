/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import Tooltip from '@brave/leo/react/tooltip'

import { AppModelContext, useAppState } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { formatString } from '$web-common/formatString'
import { TabOpenerContext } from '../../../shared/components/new_tab_link'
import { WalletProviderIcon } from '../../../shared/components/icons/wallet_provider_icon'
import {
  getExternalWalletProviderName,
  shouldResetExternalWallet,
} from '../../../shared/lib/external_wallet'
import { AccountBalance } from '../common/account_balance'

import { style } from './payout_account_card.style'

const payoutDateFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long',
  day: 'numeric',
})

export function PayoutAccountCard() {
  const model = React.useContext(AppModelContext)
  const tabOpener = React.useContext(TabOpenerContext)
  const { getString } = useLocaleContext()

  const externalWallet = useAppState((state) => state.externalWallet)
  const balance = useAppState((state) => state.balance)
  const adsInfo = useAppState((state) => state.adsInfo)

  const [showDetails, setShowDetails] = React.useState(false)

  React.useEffect(() => {
    if (!setShowDetails) {
      return
    }
    const listener = () => {
      setShowDetails(false)
    }
    document.addEventListener('click', listener)
    return () => document.removeEventListener('click', listener)
  }, [setShowDetails])

  if (!externalWallet || shouldResetExternalWallet(externalWallet.provider)) {
    return null
  }

  const providerName = getExternalWalletProviderName(externalWallet.provider)

  function onReconnectClick() {
    if (externalWallet) {
      model.beginExternalWalletLogin(externalWallet.provider)
    }
  }

  function renderReconnect() {
    if (!externalWallet) {
      return null
    }
    return (
      <section className='reconnect'>
        <span className='text'>
          {formatString(getString('payoutAccountLoginText'), [providerName])}
        </span>
        <Button onClick={onReconnectClick}>
          <span slot='icon-before'>
            <WalletProviderIcon provider={externalWallet.provider} />
          </span>
          {formatString(getString('payoutAccountLoginButtonLabel'), [
            providerName,
          ])}
        </Button>
      </section>
    )
  }

  function renderAccountInfo() {
    if (!externalWallet) {
      return null
    }
    return (
      <section className='account-info'>
        <div className='balance'>
          <label>{getString('payoutAccountBalanceLabel')}</label>
          <span>
            <AccountBalance balance={balance} />
          </span>
        </div>
        <div className='account'>
          <label>{getString('payoutAccountLabel')}</label>
          <button
            className={'account-drop-down' + (showDetails ? ' open' : '')}
            onClick={(event) => {
              event.stopPropagation()
              setShowDetails(!showDetails)
            }}
          >
            <span className='provider-icon'>
              <WalletProviderIcon provider={externalWallet.provider} />
            </span>
            <span className='account-name'>
              {externalWallet.name || providerName}
            </span>
            <Icon name='arrow-small-down' />
          </button>
          {renderAccountDetails()}
        </div>
      </section>
    )
  }

  function renderAccountDetails() {
    if (!externalWallet || !showDetails) {
      return null
    }
    return (
      <div
        className='account-details'
        onClick={(event) => event.stopPropagation()}
        onKeyDown={(event) => {
          if (event.key === 'Escape') {
            setShowDetails(false)
          }
        }}
      >
        <div className='header'>
          <div className='provider'>
            <WalletProviderIcon provider={externalWallet.provider} />
            <span className='provider-name'>{providerName}</span>
            <Label color='green'>
              <Icon
                name='check-circle-outline'
                slot='icon-before'
              />
              {getString('payoutAccountConnectedLabel')}
            </Label>
          </div>
          <div className='account-name'>{externalWallet.name}</div>
        </div>
        <button onClick={() => tabOpener.openTab(externalWallet.url)}>
          <Icon name='launch' />
          {getString('payoutAccountLink')}
        </button>
      </div>
    )
  }

  return (
    <div
      className='content-card'
      data-css-scope={style.scope}
    >
      <h4>
        {externalWallet.authenticated ? (
          <Icon name='check-circle-filled' />
        ) : (
          <WalletProviderIcon provider={externalWallet.provider} />
        )}
        <span className='title'>
          {externalWallet.authenticated
            ? getString('payoutAccountTitle')
            : getString('payoutAccountLoggedOutTitle')}
        </span>
        <Tooltip
          mode='default'
          className='info'
        >
          <Icon name='info-outline' />
          <div slot='content'>{getString('payoutAccountTooltip')}</div>
        </Tooltip>
      </h4>
      {externalWallet.authenticated ? renderAccountInfo() : renderReconnect()}
      {adsInfo && (
        <div className='content-card-footer'>
          <span>{getString('adsSettingsPayoutDateLabel')}</span>
          <span className='date'>
            {payoutDateFormatter.format(new Date(adsInfo.nextPaymentDate))}
          </span>
        </div>
      )}
    </div>
  )
}

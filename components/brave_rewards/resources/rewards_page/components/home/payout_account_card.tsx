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
import { formatMessage } from '../../../shared/lib/locale_context'
import { TabOpenerContext } from '../../../shared/components/new_tab_link'
import { WalletProviderIcon } from '../../../shared/components/icons/wallet_provider_icon'
import { getExternalWalletProviderName } from '../../../shared/lib/external_wallet'

import { style } from './payout_account_card.style'

export function PayoutAccountCard() {
  const model = React.useContext(AppModelContext)
  const tabOpener = React.useContext(TabOpenerContext)
  const { getString } = useLocaleContext()

  const [externalWallet, balance] = useAppState((state) => [
    state.externalWallet,
    state.balance
  ])

  const [showDetails, setShowDetails] = React.useState(false)

  React.useEffect(() => {
    if (!setShowDetails) {
      return
    }
    const listener = () => { setShowDetails(false) }
    document.addEventListener('click', listener);
    return () => document.removeEventListener('click', listener)
  }, [setShowDetails])

  if (!externalWallet) {
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
          {formatMessage(getString('payoutAccountLoginText'), [providerName])}
        </span>
        <Button onClick={onReconnectClick}>
          <span slot='icon-before'>
            <WalletProviderIcon provider={externalWallet.provider} />
          </span>
          {
            formatMessage(getString('payoutAccountLoginButtonLabel'), [
              providerName
            ])
          }
        </Button>
      </section>
    )
  }

  function renderBalance() {
    if (!balance.hasValue()) {
      return null
    }
    return `${balance.value()} BAT`
  }

  function renderAccountInfo() {
    if (!externalWallet) {
      return null
    }
    return (
      <section>
        <div className='balance'>
          <label>
            {getString('payoutAccountBalanceLabel')}
          </label>
          <span>
            {renderBalance()}
          </span>
        </div>
        <div className='account'>
          <label>
            {getString('payoutAccountLabel')}
          </label>
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
            <span>{externalWallet.name}</span>
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
            <span className='provider-name'>
              {
                formatMessage(getString('payoutAccountDetailsTitle'), [
                  providerName
                ])
              }
            </span>
            <Label color='green'>
              <Icon name='check-circle-outline' slot='icon-before' />
              {getString('payoutAccountConnectedLabel')}
            </Label>
          </div>
          <div className='account-name'>
            {externalWallet.name}
          </div>
        </div>
        <button onClick={() => tabOpener.openTab(externalWallet.url)}>
          <Icon name='launch' />
          {getString('payoutAccountLink')}
        </button>
      </div>
    )
  }

  return (
    <div className='content-card' {...style}>
      <h4>
        {
          externalWallet.authenticated ?
            <Icon name='check-circle-filled' /> :
            <WalletProviderIcon provider={externalWallet.provider} />
        }
        <span className='title'>
          {
            externalWallet.authenticated ?
              getString('payoutAccountTitle') :
              getString('payoutAccountLoggedOutTitle')
          }
        </span>
        <Tooltip mode='default' className='info'>
          <Icon name='info-outline' />
          <div slot='content'>{getString('payoutAccountTooltip')}</div>
        </Tooltip>
      </h4>
      {externalWallet.authenticated ? renderAccountInfo() : renderReconnect()}
    </div>
  )
}

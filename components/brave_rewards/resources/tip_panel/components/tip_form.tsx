/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../lib/locale_strings'
import { ModelContext, useModelState } from '../lib/model_context'
import { useScopedCallback } from '../lib/scoped_callback'
import { formatMessage } from '../../shared/lib/locale_context'
import { NewTabLink, TabOpenerContext } from '../../shared/components/new_tab_link'
import { getExternalWalletProviderName } from '../../shared/lib/external_wallet'
import { batAmountFormatter } from '../lib/formatters'
import { LoadingIcon } from '../../shared/components/icons/loading_icon'
import { InfoBox } from './info_box'
import { BalanceView } from './balance_view'
import { MonthlyToggle } from './monthly_toggle'
import { AmountInput } from './amount_input'
import { Terms } from './terms'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './tip_form.style'

type SendStatus = 'none' | 'pending' | 'sent' | 'failed'

export function TipForm () {
  const tabOpener = React.useContext(TabOpenerContext)
  const model = React.useContext(ModelContext)
  const { getString } = useLocaleContext()
  const state = useModelState((state) => state)

  const scopedCallback = useScopedCallback()
  const [sendAmount, setSendAmount] = React.useState(0)
  const [sendStatus, setSendStatus] = React.useState<SendStatus>('none')
  const [monthlyChecked, setMonthlyChecked] = React.useState(false)

  const insufficientBalance = state.rewardsUser.balance.valueOr(0) < sendAmount

  function sendButtonDisabled () {
    return sendAmount <= 0 || insufficientBalance || sendStatus === 'pending'
  }

  function onConnectClick () {
    tabOpener.openTab(urls.connectURL)
  }

  function onReconnectClick () {
    model.reconnectWallet()
  }

  function onSendClick () {
    if (sendAmount <= 0 || sendStatus === 'pending') {
      return
    }

    setSendStatus('pending')

    model.sendContribution(sendAmount, monthlyChecked).then(
      scopedCallback(() => setSendStatus('sent')),
      scopedCallback(() => setSendStatus('failed')))
  }

  function onWeb3Click () {
    tabOpener.openTab(state.creatorBanner.web3URL)
  }

  function renderSendButtonLabel () {
    switch (sendStatus) {
      case 'failed': return getString('tryAgainButtonLabel')
      case 'pending': return <LoadingIcon />
      default: return getString('sendButtonLabel')
    }
  }

  // The following flags control form rendering and are set in `renderInfo`.
  let providerName = ''
  let sendEnabled = false
  let showBalance = true
  let needsReconnect = false
  let showConnect = false

  function renderInfo () {
    if (!state.rewardsUser.walletProvider) {
      showBalance = false
      showConnect = true
      return (
        <InfoBox title={getString('notConnectedTitle')}>
          {getString('notConnectedText')}
        </InfoBox>
      )
    }

    providerName =
      getExternalWalletProviderName(state.rewardsUser.walletProvider)

    if (!state.rewardsUser.walletAuthorized) {
      needsReconnect = true
      const title = formatMessage(getString('reconnectTitle'), [providerName])
      return (
        <InfoBox title={title} style='warn'>
          <div>
            {formatMessage(getString('reconnectText'), [providerName])}
          </div>
          {
            state.creatorBanner.web3URL &&
              <div>{getString('reconnectWeb3Text')}</div>
          }
        </InfoBox>
      )
    }

    if (state.creatorWallets.length === 0 && state.creatorBanner.web3URL) {
      showBalance = false
      return (
        <InfoBox title={getString('web3OnlyTitle')}>
          <div>
            {formatMessage(getString('providerMismatchText'), [providerName])}
          </div>
          {
            state.creatorBanner.web3URL &&
              <div>{getString('providerMismatchWeb3Text')}</div>
          }
        </InfoBox>
      )
    }

    const providersMatch = state.creatorWallets.some((wallet) => {
      return wallet === state.rewardsUser.walletProvider
    })

    if (!providersMatch) {
      return (
        <InfoBox title={getString('providerMismatchTitle')}>
          <div>
            {formatMessage(getString('providerMismatchText'), [providerName])}
          </div>
          {
            state.creatorBanner.web3URL &&
              <div>{getString('providerMismatchWeb3Text')}</div>
          }
        </InfoBox>
      )
    }

    sendEnabled = true

    if (sendStatus === 'failed') {
      return (
        <InfoBox title={getString('contributionFailedTitle')} style='error'>
          {getString('contributionFailedText')}
        </InfoBox>
      )
    }

    if (insufficientBalance) {
      return (
        <InfoBox title={getString('insufficientBalanceTitle')} style='error'>
          {
            formatMessage(getString('insufficientBalanceText'), [
              batAmountFormatter.format(state.rewardsUser.balance.valueOr(0))
            ])
          }
        </InfoBox>
      )
    }

    if (state.monthlyContributionSet) {
      return (
        <InfoBox title={getString('monthlySetTitle')}>
          {
            formatMessage(getString('monthlySetText'), {
              tags: {
                $1: (content) => (
                  <NewTabLink key='link' href={urls.settingsURL}>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </InfoBox>
      )
    }

    return null
  }

  if (sendStatus === 'sent') {
    const onShareClick = () => { model.shareContribution() }
    return (
      <style.root>
        <style.successCard>
          <style.successTitle>
            {getString('contributionSentTitle')}
          </style.successTitle>
          <style.successText>
            {getString('contributionSentText')}
          </style.successText>
          <style.buttons>
            <button onClick={onShareClick}>
              {getString('shareButtonLabel')}
            </button>
          </style.buttons>
        </style.successCard>
        <Terms />
      </style.root>
    )
  }

  const infoBox = renderInfo()

  return (
    <style.root>
      <style.card className={insufficientBalance ? 'insufficient-balance' : ''}>
        <style.title>
          {getString('sendFormTitle')}
        </style.title>
        <style.inputPanel>
          <style.controls>
            {
              showBalance &&
                <BalanceView
                  userBalance={state.rewardsUser.balance}
                  walletProvider={state.rewardsUser.walletProvider}
                />
            }
            {
              sendEnabled &&
                <AmountInput
                  amountOptions={state.rewardsParameters.contributionAmounts}
                  userBalance={state.rewardsUser.balance.valueOr(0)}
                  exchangeRate={state.rewardsParameters.exchangeRate}
                  exchangeCurrency={state.rewardsParameters.exchangeCurrency}
                  onAmountUpdated={setSendAmount}
                />
            }
            {
              sendEnabled && !state.monthlyContributionSet &&
                <MonthlyToggle
                  checked={monthlyChecked}
                  onChange={setMonthlyChecked}
                />
            }
          </style.controls>
          {infoBox}
        </style.inputPanel>
        <style.buttons>
          {
            needsReconnect &&
              <button onClick={onReconnectClick}>
                {
                  formatMessage(getString('reconnectButtonLabel'), [
                    providerName
                  ])
                }
              </button>
          }
          {
            showConnect &&
              <button onClick={onConnectClick}>
                {getString('connectButtonLabel')}
              </button>
          }
          {
            sendEnabled &&
              <button
                onClick={onSendClick}
                disabled={sendButtonDisabled()}
                className={sendStatus === 'pending' ? 'pressed' : ''}
              >
                {renderSendButtonLabel()}
              </button>
          }
          {
            state.creatorBanner.web3URL &&
              <button onClick={onWeb3Click}>
                {getString('web3ButtonLabel')}
              </button>
          }
        </style.buttons>
      </style.card>
      <Terms />
    </style.root>
  )
}

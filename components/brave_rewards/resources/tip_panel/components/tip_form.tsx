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
import { getExternalWalletProviderName, isSelfCustodyProvider } from '../../shared/lib/external_wallet'
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

  const isSelfCustodyUser =
    state.rewardsUser.walletProvider &&
    isSelfCustodyProvider(state.rewardsUser.walletProvider)

  function sendButtonDisabled () {
    return sendAmount <= 0 || insufficientBalance || sendStatus === 'pending'
  }

  function onReconnectClick () {
    tabOpener.openTab(state.rewardsUser.reconnectUrl)
  }

  function onSendClick () {
    if (sendAmount <= 0 || sendStatus === 'pending') {
      return
    }

    setSendStatus('pending')

    model.sendContribution(sendAmount, monthlyChecked).then(
      scopedCallback((success) => {
        setSendStatus(success ? 'sent' : 'failed')
      }))
  }

  function onWeb3Click () {
    tabOpener.openTab(state.creatorBanner.web3Url)
  }

  function onShareClick () {
    const text = formatMessage(getString('shareText'), [
      sendAmount,
      model.getState().creatorBanner.title
    ]).join('')

    tabOpener.openTab(
      `https://twitter.com/intent/tweet?text=${encodeURIComponent(text)}`
    )
  }

  function renderSendButtonLabel () {
    switch (sendStatus) {
      case 'failed': return getString('tryAgainButtonLabel')
      case 'pending': return <LoadingIcon />
    }

    const { walletProvider } = state.rewardsUser
    if (!walletProvider) {
      return getString('sendButtonLabel')
    }

    return formatMessage(getString('sendWithButtonLabel'), [
      getExternalWalletProviderName(walletProvider)
    ])
  }

  function maybeRenderMonthlyTipNote () {
    if (!state.monthlyContributionSet) {
      return null
    }
    return (
      <InfoBox title={getString('monthlySetTitle')}>
        {
          formatMessage(getString('monthlySetText'), {
            tags: {
              $1: (content) => (
                <NewTabLink key='link' href={urls.settingsURL + '#monthly'}>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
      </InfoBox>
    )
  }

  // The following flags control form rendering and are set in `renderInfo`.
  let providerName = ''
  let sendEnabled = false
  let showBalance = true
  let needsReconnect = false

  function renderInfo () {
    if (isSelfCustodyUser && !state.creatorBanner.web3Url) {
      showBalance = false
      return (
        <InfoBox title={getString('providerMismatchTitle')}>
          <div>
            {getString('selfCustodyNoWeb3Label')}
          </div>
        </InfoBox>
      )
    }

    // If the user does not have a wallet provider, then for simplicity assume
    // that this is a legacy "unconnected" user. A 2.5 or later user that is
    // unconnected should never be shown this UI, but if somehow they are it
    // will do no harm; a zero balance will be displayed.
    if (!state.rewardsUser.walletProvider) {
      if (state.creatorWallets.length > 0) {
        sendEnabled = true
      }
      return maybeRenderMonthlyTipNote()
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
            state.creatorBanner.web3Url &&
              <div>{getString('reconnectWeb3Text')}</div>
          }
        </InfoBox>
      )
    }

    if (state.creatorWallets.length === 0 && state.creatorBanner.web3Url) {
      showBalance = false
      return (
        <InfoBox title={getString('web3OnlyTitle')}>
          <div>
            {formatMessage(getString('providerMismatchText'), [providerName])}
          </div>
          {
            state.creatorBanner.web3Url &&
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
            state.creatorBanner.web3Url &&
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

    if (insufficientBalance && state.rewardsUser.balance.hasValue()) {
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

    return maybeRenderMonthlyTipNote()
  }

  if (sendStatus === 'sent') {
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

  if (isSelfCustodyUser && state.creatorBanner.web3Url) {
    return (
      <style.root>
        <style.card>
          <style.selfCustody>
            <style.selfCustodyTitle>
              {getString('selfCustodyTitle')}
            </style.selfCustodyTitle>
            <style.selfCustodyHeader>
            {getString('selfCustodyHeader')}
            </style.selfCustodyHeader>
            <style.selfCustodyText>
              {getString('selfCustodyText')}
            </style.selfCustodyText>
          </style.selfCustody>
          <style.buttons>
            <button onClick={onWeb3Click}>
              {getString('selfCustodySendButtonLabel')}
            </button>
          </style.buttons>
        </style.card>
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
            sendEnabled &&
              <button
                onClick={onSendClick}
                disabled={sendButtonDisabled()}
                className={sendStatus === 'pending' ? 'pressed' : ''}
                data-test-id='send-button'
              >
                {renderSendButtonLabel()}
              </button>
          }
          {
            state.creatorBanner.web3Url &&
              <button onClick={onWeb3Click}>
                {getString('web3ButtonLabel')}
              </button>
          }
        </style.buttons>
        <style.successBackgroundPreloader />
      </style.card>
      {sendEnabled && <Terms />}
    </style.root>
  )
}

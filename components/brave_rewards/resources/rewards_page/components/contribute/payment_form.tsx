/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import RadioButton from '@brave/leo/react/radioButton'

import { AppModelContext, useAppState } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { formatMessage } from '../../../shared/lib/locale_context'
import { TabOpenerContext } from '../../../shared/components/new_tab_link'
import { WalletProviderIcon } from '../../../shared/components/icons/wallet_provider_icon'
import { getExternalWalletProviderName } from '../../../shared/lib/external_wallet'

import { style } from './payment_form.style'

const minimumAmount = 0.002
const maximumAmount = 100

const optionValueFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2
})

const exchangeFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2,
  maximumFractionDigits: 2,
  minimumSignificantDigits: 1,
  maximumSignificantDigits: 1,
  // @ts-expect-error: "roudingPriority" not yet recognized
  roundingPriority: 'morePrecision',
  roundingMode: 'ceil'
})

interface Props {
  onCancel: () => void
  onSend: (amount: number, recurring: boolean) => void
}

export function PaymentForm(props: Props) {
  const { getString } = useLocaleContext()
  const model = React.useContext(AppModelContext)
  const tabOpener = React.useContext(TabOpenerContext)

  const [
    creator,
    externalWallet,
    parameters,
    balance
  ] = useAppState((state) => [
    state.currentCreator,
    state.externalWallet,
    state.rewardsParameters,
    state.balance
  ])

  const [selectedOption, setSelectedOption] = React.useState(0)
  const [customAmountText, setCustomAmountText] = React.useState('')
  const [customAmount, setCustomAmount] = React.useState(0)
  const [isRecurring, setIsRecurring] = React.useState(false)

  const otherOptionValue = -1

  if (!externalWallet || !parameters) {
    return null
  }

  const sendValue =
    selectedOption === otherOptionValue ? customAmount : selectedOption

  function onCustomAmountChange(event: React.FormEvent<HTMLInputElement>) {
    const textValue = event.currentTarget.value
    if (!textValue.match(/^\d{0,3}(\.\d{0,3})?$/)) {
      return
    }
    if (!textValue) {
      setCustomAmountText('')
      setCustomAmount(0)
      return
    }
    const value = parseFloat(textValue) || 0
    if (value && value < minimumAmount || value > maximumAmount) {
      return
    }
    setCustomAmountText(textValue)
    setCustomAmount(value)
  }

  function renderExchange(batAmount: number) {
    if (!parameters) {
      return null
    }
    return exchangeFormatter.format(batAmount * parameters.rate) + ' USD'
  }

  function isSendDisabled() {
    return sendValue <= 0 || sendValue > balance.valueOr(0)
  }

  function renderSendText() {
    if (sendValue <= 0 || !balance.hasValue()) {
      return getString('contributeSendButtonLabel')
    }
    if (sendValue > balance.valueOr(0)) {
      return getString('contributeInsufficientFundsButtonLabel')
    }
    return formatMessage(getString('contributeSendAmountButtonLabel'), [
      sendValue
    ])
  }

  function renderBalanceText() {
    if (!externalWallet?.authenticated) {
      return getString('contributeBalanceUnavailableText')
    }
    if (!balance.hasValue()) {
      return ''
    }
    return `${balance.value()} BAT`
  }

  function renderBalance() {
    if (!externalWallet) {
      return null
    }
    return (
      <div className='balance'>
        <div className='bat-icon'>
          <Icon name='bat-color' />
        </div>
        <div>
          <div className='balance-header'>
            {getString('contributeBalanceTitle')}
            <WalletProviderIcon provider={externalWallet.provider} />
            <span className='provider-name'>
              {getExternalWalletProviderName(externalWallet.provider)}
            </span>
          </div>
          <div className='balance-value'>
            {renderBalanceText()}
          </div>
        </div>
      </div>
    )
  }

  function renderLoggedOut() {
    if (!externalWallet || !creator) {
      return null
    }
    const { provider } = externalWallet
    const providerName = getExternalWalletProviderName(provider)
    const { web3URL } = creator.banner
    return (
      <div {...style}>
        {renderBalance()}
        <div className='reconnect'>
          <Icon name='warning-triangle-filled' />
          <div className='content'>
            <h4>
              {
                formatMessage(getString('contributeLoggedOutTitle'), [
                  providerName
                ])
              }
            </h4>
            <p>{getString('contributeLoggedOutText')}</p>
            {web3URL && <p>{getString('contributeLoggedOutWeb3Text')}</p>}
          </div>
        </div>
        <div className='actions'>
          <Button onClick={() => { model.beginExternalWalletLogin(provider)}}>
            {
              formatMessage(getString('contributeLoginButtonLabel'), [
                providerName
              ])
            }
          </Button>
          {
            web3URL &&
              <Button
                kind='outline'
                onClick={() => tabOpener.openTab(web3URL)}
              >
                {getString('contributeLoggedOutWeb3ButtonLabel')}
              </Button>
          }
        </div>
      </div>
    )
  }

  if (!externalWallet.authenticated) {
    return renderLoggedOut()
  }

  return (
    <div {...style}>
      {renderBalance()}
      <div>
        <div className='form-header'>
          {getString('contributeAmountTitle')}
        </div>
        <div className='options'>
          {
            parameters.tipChoices.map((value) =>
              <div key={value} className='option'>
                <RadioButton
                  name='amount'
                  value={value}
                  currentValue={selectedOption}
                  onChange={() => setSelectedOption(value)}
                >
                  {optionValueFormatter.format(value)} BAT
                </RadioButton>
                <div className='exchange'>
                  {renderExchange(value)}
                </div>
              </div>
            )
          }
          <div className='option'>
            <RadioButton
              name='amount'
              value={otherOptionValue}
              currentValue={selectedOption}
              onChange={() => setSelectedOption(otherOptionValue)}
            >
              {getString('contributeOtherLabel')}
            </RadioButton>
            {
              selectedOption === otherOptionValue && <>
                <div className='custom-input'>
                  <input
                    value={customAmountText}
                    onChange={onCustomAmountChange}
                    onFocus={(event) => event.currentTarget.select()}
                    autoFocus
                  />
                </div>
                <div className='exchange'>
                  {renderExchange(customAmount)}
                </div>
              </>
            }
          </div>
        </div>
      </div>
      <div className='recurring'>
        <Checkbox
          checked={isRecurring}
          onChange={(detail) => setIsRecurring(detail.checked)}
        >
          {getString('contributeRecurringLabel')}
        </Checkbox>
        <div className='frequency'>
          {isRecurring && getString('contributeMonthlyLabel')}
        </div>
      </div>
      <div className='actions'>
        <Button
          isDisabled={isSendDisabled()}
          onClick={() => props.onSend(sendValue, isRecurring)}
        >
          {renderSendText()}
        </Button>
      </div>
    </div>
  )
}

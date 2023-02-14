/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useLocaleContext } from '../lib/locale_strings'
import { batAmountFormatter } from '../lib/formatters'
import { optional } from '../../shared/lib/optional'
import { SwapIcon } from './icons/swap_icon'

import * as style from './amount_input.style'

const minimumAmount = 0.25
const maximumAmount = 100
const amountStep = 0.25

const exchangeAmountFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2,
  maximumFractionDigits: 2
})

function currencyFormatter (currency: string) {
  return new Intl.NumberFormat(undefined, {
    style: 'currency',
    currency,
    minimumFractionDigits: 2,
    maximumFractionDigits: 2
  })
}

function currencySymbol (currency: string) {
  for (const elem of currencyFormatter(currency).formatToParts(1)) {
    if (elem.type === 'currency') {
      return elem.value
    }
  }
  return ''
}

function getDefaultOption (balance: number, options: number[]) {
  // Select the highest amount that is greater than or equal to the user's
  // balance, starting from the middle option.
  if (options.length > 0) {
    for (let i = Math.floor(options.length / 2); i >= 0; --i) {
      if (i === 0 || options[i] <= balance) {
        return optional(options[i])
      }
    }
  }
  return optional<number>()
}

interface Props {
  amountOptions: number[]
  userBalance: number
  exchangeRate: number
  exchangeCurrency: string
  onAmountUpdated: (amount: number) => void
}

export function AmountInput (props: Props) {
  const { getString } = useLocaleContext()

  const [selectedOption, setSelectedOption] =
    React.useState(getDefaultOption(props.userBalance, props.amountOptions))
  const [customAmount, setCustomAmount] = React.useState(0)
  const [customAmountText, setCustomAmountText] = React.useState('')
  const [exchangePrimary, setExchangePrimary] = React.useState(false)

  React.useEffect(() => {
    if (selectedOption.hasValue()) {
      props.onAmountUpdated(selectedOption.value())
    } else {
      props.onAmountUpdated(customAmount)
    }
  }, [selectedOption.valueOr(undefined), customAmount])

  function optionClassName (amount?: number) {
    return selectedOption.valueOr(undefined) === amount ? 'selected' : ''
  }

  function onCustomClick () {
    if (selectedOption.hasValue()) {
      let value = selectedOption.value()
      setCustomAmount(value)
      if (exchangePrimary) {
        value *= props.exchangeRate
        setCustomAmountText(exchangeAmountFormatter.format(value))
      } else {
        setCustomAmountText(batAmountFormatter.format(value))
      }
      setSelectedOption(optional())
    }
  }

  function onCustomAmountChange (event: React.FormEvent<HTMLInputElement>) {
    const textValue = event.currentTarget.value
    if (/[^\d\.]|(\..*\.)/.test(textValue)) {
      return
    }
    setCustomAmountText(textValue)
    let customAmount = parseFloat(textValue || '0')
    if (isNaN(customAmount)) {
      return
    }
    if (exchangePrimary) {
      customAmount /= props.exchangeRate
    }
    customAmount = Math.min(maximumAmount, customAmount)
    setCustomAmount(customAmount)
  }

  function onCustomAmountBlur () {
    let value = amountStep > 0
      ? Math.round(customAmount / amountStep) * amountStep
      : customAmount

    if (value < minimumAmount) {
      value = minimumAmount
    }

    setCustomAmount(value)

    if (exchangePrimary) {
      value *= props.exchangeRate
      setCustomAmountText(exchangeAmountFormatter.format(value))
    } else {
      setCustomAmountText(batAmountFormatter.format(value))
    }
  }

  function onSwapClick () {
    const newExchangePrimary = !exchangePrimary
    if (!selectedOption.hasValue()) {
      const value = customAmount
      if (newExchangePrimary) {
        const exchangeValue = value * props.exchangeRate
        setCustomAmountText(exchangeAmountFormatter.format(exchangeValue))
      } else {
        setCustomAmountText(batAmountFormatter.format(value))
      }
    }
    setExchangePrimary(newExchangePrimary)
  }

  function renderPrimaryAmount () {
    if (!selectedOption.hasValue()) {
      return (
        <style.customInput>
          <input
            type='text'
            value={customAmountText}
            onChange={onCustomAmountChange}
            onBlur={onCustomAmountBlur}
            autoFocus={true}
          />
        </style.customInput>
      )
    }
    if (exchangePrimary) {
      const value = selectedOption.value() * props.exchangeRate
      return (
        <style.primaryAmount>
          {exchangeAmountFormatter.format(value)}
        </style.primaryAmount>
      )
    }
    return (
      <style.primaryAmount>
        {batAmountFormatter.format(selectedOption.value())}
      </style.primaryAmount>
    )
  }

  function renderPrimary () {
    if (exchangePrimary) {
      return (
        <style.primary>
          <style.primarySymbol>
            {currencySymbol(props.exchangeCurrency)}
          </style.primarySymbol>
          {renderPrimaryAmount()}
          <style.primaryLabel>
            {props.exchangeCurrency}
          </style.primaryLabel>
        </style.primary>
      )
    }
    return (
      <style.primary>
        {renderPrimaryAmount()}
        <style.primaryLabel>BAT</style.primaryLabel>
      </style.primary>
    )
  }

  function renderSecondaryAmount () {
    let value = selectedOption.valueOr(customAmount)
    if (exchangePrimary) {
      return (
        <style.secondaryAmount>
          {batAmountFormatter.format(value)} BAT
        </style.secondaryAmount>
      )
    }
    value *= props.exchangeRate
    return (
      <style.secondaryAmount>
        {currencyFormatter(props.exchangeCurrency).format(value)}
        &nbsp;{props.exchangeCurrency}
      </style.secondaryAmount>
    )
  }

  return (
    <style.root>
      <style.selector>
        {
          props.amountOptions.map((amount) => {
            const onClick = () => setSelectedOption(optional(amount))
            return (
              <button
                key={amount}
                onClick={onClick}
                className={optionClassName(amount)}
              >
                {batAmountFormatter.format(amount)}
              </button>
            )
          })
        }
        <button onClick={onCustomClick} className={optionClassName()}>
          {getString('customAmountLabel')}
        </button>
      </style.selector>
      <style.amount>
        {renderPrimary()}
        <style.secondary>
          {renderSecondaryAmount()}
          <style.swap>
            <button onClick={onSwapClick}>
              <SwapIcon />
            </button>
          </style.swap>
        </style.secondary>
      </style.amount>
    </style.root>
  )
}

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

const minimumAmount = 0.002
const maximumAmount = 100
const amountStep = 0.001

function quantizeAmount (amount: number) {
  if (amount === 0) {
    return amount
  }
  const value = amountStep > 0
    ? Math.floor(amount / amountStep) * amountStep
    : amount
  return Math.min(Math.max(value, minimumAmount), maximumAmount)
}

const exchangeAmountFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2,
  maximumFractionDigits: 2
})

function currencyFormatter (currency: string) {
  return new Intl.NumberFormat(undefined, {
    style: 'currency',
    currency,
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
    minimumSignificantDigits: 1,
    maximumSignificantDigits: 1,
    // @ts-expect-error: "roudingPriority" not yet recognized
    roundingPriority: 'morePrecision',
    roundingMode: 'ceil'
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

interface Props {
  amountOptions: number[]
  userBalance: number
  exchangeRate: number
  exchangeCurrency: string
  onAmountUpdated: (amount: number) => void
}

export function AmountInput (props: Props) {
  const { getString } = useLocaleContext()

  const [selectedOption, setSelectedOption] = React.useState(optional(0))
  const [customAmount, setCustomAmount] = React.useState(0)
  const [customAmountText, setCustomAmountText] = React.useState('')
  const [exchangePrimary, setExchangePrimary] = React.useState(false)

  React.useEffect(() => {
    if (selectedOption.hasValue()) {
      props.onAmountUpdated(selectedOption.value())
    } else {
      props.onAmountUpdated(quantizeAmount(customAmount))
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

  function updateCustomAmount (textValue: string) {
    if (/[^\d\.]|(\..*\.)/.test(textValue)) {
      return customAmount
    }
    setCustomAmountText(textValue)
    let updatedAmount = parseFloat(textValue || '0')
    if (isNaN(updatedAmount)) {
      return customAmount
    }
    if (exchangePrimary) {
      updatedAmount /= props.exchangeRate
    }
    setCustomAmount(updatedAmount)
    return updatedAmount
  }

  function onCustomInputMounted (elem: HTMLElement | null) {
    // Expose a programmatic way to update the value in browser tests.
    if (elem) {
      elem[Symbol.for('updateCustomAmountForTesting')] = updateCustomAmount
    }
  }

  function onCustomAmountChange (event: React.FormEvent<HTMLInputElement>) {
    updateCustomAmount(event.currentTarget.value)
  }

  function onCustomAmountBlur (event: React.FormEvent<HTMLInputElement>) {
    const amount = updateCustomAmount(event.currentTarget.value)
    let value = quantizeAmount(amount)

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
      const onFocus = (event: React.FormEvent<HTMLInputElement>) => {
        event.currentTarget.select()
      }
      return (
        <style.customInput>
          <input
            type='text'
            value={customAmountText}
            ref={onCustomInputMounted}
            onChange={onCustomAmountChange}
            onBlur={onCustomAmountBlur}
            onFocus={onFocus}
            autoFocus={true}
            data-test-id='custom-amount-input'
          />
        </style.customInput>
      )
    }

    if (exchangePrimary) {
      const value = selectedOption.value() * props.exchangeRate
      return (
        <style.primaryAmount onClick={onCustomClick}>
          {exchangeAmountFormatter.format(value)}
        </style.primaryAmount>
      )
    }
    return (
      <style.primaryAmount onClick={onCustomClick}>
        {batAmountFormatter.format(selectedOption.value())}
      </style.primaryAmount>
    )
  }

  function renderPrimary () {
    return (
      <style.primary>
        {
          exchangePrimary &&
            <style.primarySymbol>
              {currencySymbol(props.exchangeCurrency)}
            </style.primarySymbol>
        }
        {renderPrimaryAmount()}
        <style.primaryLabel>
          {exchangePrimary ? props.exchangeCurrency : 'BAT'}
        </style.primaryLabel>
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
      <style.selector data-test-id='tip-amount-options'>
        {
          props.amountOptions.map((amount, index) => {
            const onClick = () => setSelectedOption(optional(amount))
            return (
              <button
                key={amount}
                onClick={onClick}
                className={optionClassName(amount)}
                data-option-index={index}
                data-option-value={amount}
              >
                {batAmountFormatter.format(amount)}
              </button>
            )
          })
        }
        <button
          onClick={onCustomClick}
          className={optionClassName()}
          data-test-id='custom-tip-button'
        >
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

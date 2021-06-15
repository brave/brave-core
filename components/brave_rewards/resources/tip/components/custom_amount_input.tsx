/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { getExchangeCurrency } from './exchange_amount'
import { ExchangeIcon } from './icons/exchange_icon'
import { CaretIcon } from '../../shared/components/icons/caret_icon'

import * as style from './custom_amount_input.style'

const amountFormat = new Intl.NumberFormat(undefined, {
  minimumIntegerDigits: 1,
  minimumFractionDigits: 2,
  maximumFractionDigits: 2
})

type InputMode = 'bat' | 'exchange'

interface Props {
  amount: number
  exchangeRate: number
  amountStep: number
  maximumAmount: number
  onAmountChange: (amount: number) => void
  onHideInput: () => void
}

export function CustomAmountInput (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  const [controlValue, setControlValue] = React.useState(
    amountFormat.format(props.amount))

  const [inputMode, setInputMode] = React.useState<InputMode>('bat')

  const onInputMounted = React.useCallback((input: HTMLInputElement | null) => {
    if (input) {
      input.focus()
      input.select()
    }
  }, [inputMode])

  const getBatValue = (inputValue: number) => {
    inputValue = inputValue || 0
    return Math.min(props.maximumAmount, inputMode === 'bat'
      ? inputValue
      : inputValue / props.exchangeRate)
  }

  const roundExchangeUp = (value: number) => {
    return Math.ceil(value * 100) / 100
  }

  const onBlur = (evt: React.FocusEvent<HTMLInputElement>) => {
    const rawValue = getBatValue(parseFloat(evt.target.value))
    const value = props.amountStep > 0
      ? Math.floor(rawValue / props.amountStep) * props.amountStep
      : rawValue

    if (isNaN(value) || value < props.amountStep) {
      return
    }

    setControlValue(amountFormat.format(inputMode === 'bat'
      ? value
      : roundExchangeUp(value * props.exchangeRate)))

    if (props.amount !== value) {
      props.onAmountChange(value)
    }
  }

  const onChange = (evt: React.ChangeEvent<HTMLInputElement>) => {
    const textValue = evt.target.value
    if (/[^\d\.]|(\..*\.)/.test(textValue)) {
      return
    }

    setControlValue(textValue)

    const value = getBatValue(parseFloat(textValue))
    if (props.amount !== value) {
      props.onAmountChange(value)
    }
  }

  const dependentAmount = inputMode === 'bat'
    ? roundExchangeUp(props.amount * props.exchangeRate)
    : props.amount

  const toggleMode = () => {
    setControlValue(amountFormat.format(dependentAmount))
    setInputMode(inputMode === 'bat' ? 'exchange' : 'bat')
  }

  return (
    <style.root>
      <style.header>
        {getString('customAmount')}
        <button onClick={props.onHideInput}>
          <CaretIcon direction='left' />
        </button>
      </style.header>
      <style.form>
        <style.amountSection>
          <style.amountBox>
            <input
              ref={onInputMounted}
              type='text'
              value={controlValue}
              onChange={onChange}
              onBlur={onBlur}
              data-test-id='custom-amount-input'
            />
            <span className='currency'>
              {inputMode === 'bat' ? 'BAT' : getExchangeCurrency()}
            </span>
          </style.amountBox>
          {
            inputMode === 'bat' &&
              <style.example>
                {formatMessage(getString('exampleTipAmount'), ['12.75'])} BAT
              </style.example>
          }
        </style.amountSection>
        <style.swap>
          <button onClick={toggleMode}>
            <ExchangeIcon />
          </button>
        </style.swap>
        <style.exhangeBox>
          <span>{amountFormat.format(dependentAmount)}</span>
          <span className='currency'>
            {inputMode === 'exchange' ? 'BAT' : getExchangeCurrency()}
          </span>
        </style.exhangeBox>
      </style.form>
    </style.root>
  )
}

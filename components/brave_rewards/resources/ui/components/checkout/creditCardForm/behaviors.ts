/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { CreditCardDetails, CreditCardErrorType, CreditCardError } from './types'
import { CardType } from './cardType'
import { ExpiryFormat } from './expiryFormat'

interface InputDictionary {
  cardNumber: HTMLInputElement
  expiry: HTMLInputElement
  securityCode: HTMLInputElement
}

interface BehaviorsInit {

  // A dictionary containing references to credit card <input>
  // elements.
  inputs: InputDictionary

  // Called when the inferred credit card type has changed.
  onCardTypeChange?: (cardType: CardType | null) => void

  // Called when validation is performed on an input. If the
  // input is valid, then the `error.type` property will equal
  // the empty string.
  onInputValidation?: (error: CreditCardError) => void
}

// Attaches dynamic behavior to credit card input elements and
// provides composition of functionality over those input elements.
// An instance of this class is designed to be created only
// once for any given set of credit card input elements. This
// class is framework-agnostic and uses only DOM interfaces.
export class Behaviors {
  inputs: InputDictionary
  cardType: CardType | null
  expiryFormat: ExpiryFormat
  onCardTypeChange?: (cardType: CardType | null) => void
  onInputValidation?: (error: CreditCardError) => void

  constructor (init: BehaviorsInit) {
    this.inputs = init.inputs
    this.expiryFormat = new ExpiryFormat()
    this.cardType = null
    this.onCardTypeChange = init.onCardTypeChange
    this.onInputValidation = init.onInputValidation

    this._attachCardNumberHandlers()
    this._attachExpiryHandlers()
    this._attachSecurityCodeHandlers()
  }

  focus () {
    this.inputs.cardNumber.focus()
  }

  get details (): CreditCardDetails {
    const { cardNumber, expiry, securityCode } = this.inputs

    const [
      expiryMonth,
      expiryYear
    ] = this.expiryFormat.parse(expiry.value, { fullYear: true })

    let cardNumberValue = cardNumber.value
    if (this.cardType) {
      cardNumberValue = this.cardType.removeCardNumberFormatting(cardNumberValue)
    }

    return {
      cardNumber: cardNumberValue,
      expiryMonth,
      expiryYear,
      securityCode: securityCode.value
    }
  }

  validate (): CreditCardError[] {
    return [
      ...this._validateCardNumber(),
      ...this._validateExpiry(),
      ...this._validateSecurityCode()
    ]
  }

  _attachCardNumberHandlers () {
    const { cardNumber } = this.inputs

    const fixCaretPositionAfterDelay = () => {
      let position = cardNumber.selectionStart || 0
      requestAnimationFrame(() => {
        if (document.activeElement === cardNumber) {
          // Advance past formatting characters
          if (cardNumber.value[position - 1] === ' ') {
            position += 1
          }
          cardNumber.setSelectionRange(position, position)
        }
      })
    }

    cardNumber.addEventListener('input', () => {
      let { value } = cardNumber
      value = value.replace(/[^\d ]/g, '')
      const prevType = this.cardType
      this.cardType = CardType.fromCardNumber(value)
      if (this.cardType) {
        fixCaretPositionAfterDelay()
        value = this.cardType.formatCardNumber(value)
        const error = this.cardType.validateCardNumber(value)
        if (!error) {
          this.inputs.expiry.focus()
        }
      }
      cardNumber.value = value
      if (this.cardType !== prevType && this.onCardTypeChange) {
        this.onCardTypeChange.call(undefined, this.cardType)
      }
    })

    cardNumber.addEventListener('blur', () => {
      if (cardNumber.value) {
        this._validateCardNumber()
      }
    })
  }

  _attachExpiryHandlers () {
    const { expiry } = this.inputs

    expiry.addEventListener('input', () => {
      const value = this.expiryFormat.format(expiry.value)
      expiry.value = value
      const error = this.expiryFormat.validate(value)
      if (!error) {
        this.inputs.securityCode.focus()
      }
    })

    expiry.addEventListener('blur', () => {
      if (expiry.value) {
        this._validateExpiry()
      }
    })
  }

  _attachSecurityCodeHandlers () {
    const { securityCode } = this.inputs

    securityCode.addEventListener('input', () => {
      const maxLength = this.cardType
        ? this.cardType.securityCodeLength
        : 3

      let { value } = securityCode
      value = value.replace(/\D/g, '')
      value = value.slice(0, maxLength)
      securityCode.value = value
    })

    securityCode.addEventListener('blur', () => {
      if (securityCode.value) {
        this._validateSecurityCode()
      }
    })
  }

  _validateCardNumber () {
    return this._runValidator(this.inputs.cardNumber, (value) => {
      if (!value) {
        return 'required-input'
      }
      if (this.cardType) {
        const error = this.cardType.validateCardNumber(value)
        if (error) {
          return 'invalid-card-number'
        }
      }
      return ''
    })
  }

  _validateExpiry () {
    return this._runValidator(this.inputs.expiry, (value) => {
      if (!value) {
        return 'required-input'
      }
      const error = this.expiryFormat.validate(value)
      if (error) {
        return 'invalid-expiry'
      }
      return ''
    })
  }

  _validateSecurityCode () {
    return this._runValidator(this.inputs.securityCode, (value) => {
      if (!value) {
        return 'required-input'
      }
      if (this.cardType) {
        const error = this.cardType.validateSecurityCode(value)
        if (error) {
          return 'invalid-security-code'
        }
      }
      return ''
    })
  }

  _runValidator (
    element: HTMLInputElement,
    fn: (value: string) => CreditCardErrorType
  ): CreditCardError[] {
    const error = { type: fn(element.value), element }
    const { onInputValidation } = this
    if (onInputValidation) {
      onInputValidation(error)
    }
    return error.type ? [error] : []
  }
}

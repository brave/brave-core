/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

type CardNumberError =
  '' |
  'invalid-length' |
  'pattern-mismatch' |
  'bad-luhn'

type SecurityCodeError =
  '' |
  'invalid-security-code'

function checkLuhn (digits: string) {
  let sum = 0
  let parity = digits.length % 2
  for (let i = 0; i < digits.length; ++i) {
    const n = parseInt(digits[i], 10)
    if (isNaN(n)) {
      return false
    }
    sum += i % 2 === parity ? n * 2 : n
  }
  return sum % 10 === 0
}

function removeCardNumberFormatting (cardNumber: string) {
  return cardNumber.replace(/\s+/g, '')
}

const cardTypes = new Map<string, CardType>()

export class CardType {
  id: string
  name: string
  pattern: RegExp
  validation: string = 'luhn'
  gaps: Array<number> = [4, 8, 12]
  lengths: Array<number> = [16]
  securityCodeLength: number = 3

  constructor (init: object) {
    Object.assign(this, init)
  }

  formatCardNumber (cardNumber: string) {
    cardNumber = removeCardNumberFormatting(cardNumber)
    const maxLength = this.lengths.reduce((prev, current) => Math.max(prev, current), 0)
    if (cardNumber.length > maxLength) {
      cardNumber = cardNumber.slice(0, maxLength)
    }
    let formatted = ''
    let start = 0
    for (const end of this.gaps) {
      if (end >= cardNumber.length) {
        break
      }
      formatted += cardNumber.slice(start, end) + ' '
      start = end
    }
    formatted += cardNumber.slice(start)
    return formatted
  }

  validateCardNumber (cardNumber: string): CardNumberError {
    cardNumber = removeCardNumberFormatting(cardNumber)
    if (!this.lengths.includes(cardNumber.length)) {
      return 'invalid-length'
    }
    if (!this.pattern.test(cardNumber)) {
      return 'pattern-mismatch'
    }
    if (this.validation === 'luhn' && !checkLuhn(cardNumber)) {
      return 'bad-luhn'
    }
    return ''
  }

  validateSecurityCode (code: string): SecurityCodeError {
    if (code.length !== this.securityCodeLength || /^\D$/.test(code)) {
      return 'invalid-security-code'
    }
    return ''
  }

  removeCardNumberFormatting (cardNumber: string) {
    return removeCardNumberFormatting(cardNumber)
  }

  static fromIdentifier (id: string) {
    return cardTypes.get(id) || null
  }

  static fromCardNumber (cardNumber: string) {
    cardNumber = removeCardNumberFormatting(cardNumber)
    for (const [, cardType] of cardTypes) {
      if (cardType.pattern.test(cardNumber)) {
        return cardType
      }
    }
    return null
  }
}

function registerTypes (initList: Array<object>) {
  for (const init of initList) {
    const cardType = new CardType(init)
    cardTypes.set(cardType.id, cardType)
  }
}

registerTypes([
  {
    id: 'visa',
    name: 'Visa',
    pattern: /^4/,
    lengths: [16, 18, 19]
  },
  {
    id: 'mastercard',
    name: 'MasterCard',
    pattern: /^(5[1-5]|677189)|^(222[1-9]|2[3-6]\d{2}|27[0-1]\d|2720)/
  },
  {
    id: 'amex',
    name: 'American Express',
    pattern: /^3[47]/,
    gaps: [4, 10],
    lengths: [15],
    securityCodeLength: 4
  },
  {
    id: 'dinersclub',
    name: 'Diners Club',
    pattern: /^(36|38|30[0-5])/,
    gaps: [4, 10],
    lengths: [14, 16, 19]
  },
  {
    id: 'discover',
    name: 'Discover',
    pattern: /^(6011|65|64[4-9]|622)/,
    lengths: [16, 19]
  },
  {
    id: 'jcb',
    name: 'JCB',
    pattern: /^35/,
    lengths: [16, 17, 18, 19]
  },
  {
    id: 'unionpay',
    name: 'UnionPay',
    pattern: /^62/,
    lengths: [14, 15, 16, 17, 18, 19]
  }
])

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type CreditCardErrorType =
  '' |
  'required-input' |
  'invalid-card-number' |
  'invalid-expiry' |
  'invalid-security-code'

export interface CreditCardError {
  type: CreditCardErrorType
  element: HTMLInputElement
}

export interface CreditCardDetails {
  cardNumber: string
  expiryMonth: string
  expiryYear: string
  securityCode: string
}

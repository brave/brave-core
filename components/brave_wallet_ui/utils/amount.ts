// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import BigNumber from 'bignumber.js'

import { CurrencySymbols } from './currency-symbols'
import { AbbreviationOptions } from '../constants/types'

type BigNumberIsh =
  | BigNumber
  | string
  | number

type AmountLike =
  | Amount
  | BigNumberIsh

export default class Amount {
  public readonly value?: BigNumber

  public constructor (value: BigNumberIsh) {
    this.value = value === ''
      ? undefined
      : new BigNumber(value)
  }

  static zero (): Amount {
    return new Amount('0')
  }

  static empty (): Amount {
    return new Amount('')
  }

  plus (value: AmountLike): Amount {
    if (value instanceof Amount) {
      return this.plus(value.value || '')
    }

    if (value === '') {
      return this
    } else if (this.value === undefined) {
      return new Amount(value)
    }

    return new Amount(this.value.plus(value))
  }

  minus (value: AmountLike): Amount {
    if (value instanceof Amount) {
      return this.minus(value.value || '')
    }

    if (value === '') {
      return this
    } else if (this.value === undefined) {
      return new Amount(value)
    }

    return new Amount(this.value.minus(value))
  }

  times (value: AmountLike): Amount {
    if (value instanceof Amount) {
      return this.times(value.value || '')
    }

    if (value === '' || this.value === undefined) {
      return Amount.empty()
    }

    return new Amount(this.value.times(value))
  }

  div (value: AmountLike): Amount {
    if (value instanceof Amount) {
      return this.div(value.value || '')
    }

    if (value === '' || this.value === undefined) {
      return Amount.empty()
    }

    if (new BigNumber(value).isZero()) {
      return Amount.empty()
    }

    return new Amount(this.value.div(value))
  }

  divideByDecimals (decimals: number): Amount {
    if (this.value === undefined) {
      return Amount.empty()
    }

    return new Amount(this.value.dividedBy(10 ** decimals))
  }

  multiplyByDecimals (decimals: number): Amount {
    if (this.value === undefined) {
      return Amount.empty()
    }

    return new Amount(this.value.multipliedBy(10 ** decimals))
  }

  gt (amount: AmountLike): boolean {
    if (this.value === undefined) {
      return false
    }

    if (amount instanceof Amount) {
      return this.gt(amount.value || '')
    }

    if (amount === '') {
      return false
    }

    return this.value.gt(amount)
  }

  gte (amount: AmountLike): boolean {
    return this.gt(amount) || this.eq(amount)
  }

  lt (amount: AmountLike): boolean {
    if (amount === '') {
      return false
    }

    if (amount instanceof Amount) {
      return this.lt(amount.value || '')
    }

    if (this.value === undefined) {
      return false
    }

    return this.value.lt(amount)
  }

  lte (amount: AmountLike): boolean {
    return this.lt(amount) || this.eq(amount)
  }

  eq (amount: AmountLike): boolean {
    if (amount instanceof Amount) {
      return this.eq(amount.value || '')
    }

    if (this.value === undefined && amount === '') {
      return true
    }

    if (this.value !== undefined && amount !== '') {
      return this.value.eq(amount)
    }

    return false
  }

  /**
   * Returns the normalized string of the given numeric value.
   *
   * This function is typically used for converting a hex value to
   * numeric value.
   *
   * Invalid values return an empty string.
   *
   * @param value Numeric value to normalize.
   */
  static normalize (value: string): string {
    if (value === '') {
      return ''
    }

    const amount = new Amount(value)
    if (amount.value !== undefined && amount.value.isNaN()) {
      return ''
    }

    if (amount.isZero()) {
      return '0'
    }

    return amount.format()
  }

  private static formatAmountWithCommas (value: string, commas: boolean): string {
    // Remove trailing zeros, including the decimal separator if necessary.
    // Example: 1.0000000000 becomes 1.
    const trimmedResult = value.replace(/\.0*$|(\.\d*[1-9])0+$/, '$1')

    // Format number with commas.
    // https://stackoverflow.com/a/2901298/1946230
    return commas
      ? trimmedResult.replace(/\B(?<!\.\d*)(?=(\d{3})+(?!\d))/g, ',')
      : trimmedResult
  }

  format (significantDigits?: number, commas: boolean = false): string {
    if (this.value === undefined || this.value.isNaN()) {
      return ''
    }

    if (significantDigits === undefined) {
      return Amount.formatAmountWithCommas(
        this.value.toFixed(),
        commas
      )
    }

    // Handle the case where the value is large enough that formatting with
    // significant figures will result in an undesirable loss of precision.
    const desiredDecimalPlaces = 2
    if (this.value.isGreaterThanOrEqualTo(10 ** (significantDigits - desiredDecimalPlaces))) {
      return Amount.formatAmountWithCommas(
        this.value.toFixed(desiredDecimalPlaces),
        commas
      )
    }

    return Amount.formatAmountWithCommas(
      this.value.precision(significantDigits).toFixed(),
      commas
    )
  }

  formatAsAsset (significantDigits?: number, symbol?: string): string {
    const result = this.format(significantDigits, true)
    if (!symbol) {
      return result
    }

    return result === ''
      ? ''
      : `${result} ${symbol}`
  }

  formatAsFiat (currency?: string): string {
    let decimals
    let value

    const valueDP = this.value && this.value.decimalPlaces()
    if (
        this.value === undefined ||
        this.value.isNaN() ||
        valueDP === null ||
        valueDP === undefined
    ) {
      return ''
    } else if (
        valueDP < 2 || this.value.isGreaterThanOrEqualTo(10)
    ) {
      decimals = 2
      value = this.value.toNumber()
    } else if (this.value.isGreaterThanOrEqualTo(1)) {
      decimals = 3
      value = this.value.toNumber()
    } else {
      value = new BigNumber(this.format(4)).toNumber()
    }

    const options: Intl.NumberFormatOptions = {
      style: 'decimal',
      minimumFractionDigits: decimals || 0,
      maximumFractionDigits: decimals || 20
    }

    // currency code must be upper-case
    const upperCaseCurrency = currency?.toUpperCase()

    if (upperCaseCurrency && CurrencySymbols[upperCaseCurrency]) {
      options.style = 'currency'
      options.currency = currency
      options.currencyDisplay = 'narrowSymbol'
    }

    return Intl.NumberFormat(navigator.language, options).format(value)
  }

  toHex (): string {
    if (this.value === undefined) {
      return ''
    }

    if (this.value.isNaN()) {
      return '0x0'
    }

    return `0x${this.value.toString(16)}`
  }

  toNumber (): number {
    if (this.value === undefined || this.value.isNaN()) {
      return 0
    }

    return this.value.toNumber()
  }

  isUndefined (): boolean {
    return this.value === undefined
  }

  isNaN (): boolean {
    return this.value !== undefined && this.value.isNaN()
  }

  isZero (): boolean {
    return this.value !== undefined && this.value.isZero()
  }

  isPositive (): boolean {
    return this.value !== undefined && this.value.isPositive()
  }

  isNegative (): boolean {
    return this.value !== undefined && this.value.isNegative()
  }

  // Abbreviate number in units of 1000 e.g., 100000 becomes 100k
  abbreviate (decimals: number, currency?: string, forceAbbreviation?: AbbreviationOptions): string {
    const powers = {
      trillion: Math.pow(10, 12),
      billion: Math.pow(10, 9),
      million: Math.pow(10, 6),
      thousand: Math.pow(10, 3)
    }
    const abbreviations = {
      thousand: 'k',
      million: 'M',
      billion: 'B',
      trillion: 'T'
    }

    if (this.value === undefined) {
      return ''
    }

    const formatter = Intl.NumberFormat(navigator.language, {
      style: currency ? 'currency' : 'decimal',
      currency: currency,
      currencyDisplay: 'narrowSymbol',
      minimumFractionDigits: decimals,
      maximumFractionDigits: decimals
    })

    const abs = this.value.absoluteValue().toNumber()
    let value = this.value.toNumber()
    let abbreviation = ''

    if (forceAbbreviation && abbreviations[forceAbbreviation] && powers[forceAbbreviation]) {
      abbreviation = abbreviations[forceAbbreviation]
      value = value / powers[forceAbbreviation]

      return formatter.format(value) + abbreviation
    }

    if (abs >= powers.trillion || Math.round(abs / powers.trillion) === 1) {
      // trillion
      abbreviation = abbreviations.trillion
      value = value / powers.trillion
    } else if (abs < powers.trillion && abs >= powers.billion) {
      // billion
      abbreviation = abbreviations.billion
      value = value / powers.billion
    } else if (abs < powers.billion && abs >= powers.million) {
      // million
      abbreviation = abbreviations.million
      value = value / powers.million
    } else if (abs < powers.million && abs >= powers.thousand) {
      // thousand
      abbreviation = abbreviations.thousand
      value = value / powers.thousand
    }

    return formatter.format(value) + abbreviation
  }
}

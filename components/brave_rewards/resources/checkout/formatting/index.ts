/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

function createCurrencyFormatter (currency: string) {
  return new Intl.NumberFormat(undefined, {
    style: 'currency',
    currency,
    minimumFractionDigits: 2,
    maximumFractionDigits: 2
  })
}

export function getExchangeRate (
  exchangeRates: Record<string, number>,
  currency: string
) {
  const rate = +(exchangeRates[currency || ''])
  return isNaN(rate) ? 0 : rate
}

export function createExchangeFormatter (
  exchangeRates: Record<string, number>,
  currency: string
) {
  const formatter = createCurrencyFormatter(currency)
  const rate = getExchangeRate(exchangeRates, currency)
  return (value: number) => formatter.format(value * rate)
}

const lastUpdatedFormatter = new Intl.DateTimeFormat(undefined, {
  // @ts-ignore: TS does not recognize dateStyle yet
  dateStyle: 'long',
  timeStyle: 'short'
})

export function formatLastUpdatedDate (dateString: string) {
  const date = Date.parse(dateString)
  return lastUpdatedFormatter.format(date)
}

export function formatTokenValue (value: number) {
  return value.toFixed(1)
}

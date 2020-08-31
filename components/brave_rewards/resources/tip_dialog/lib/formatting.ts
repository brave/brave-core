/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export function formatTokenAmount (value: number) {
  return value.toFixed(3)
}

export function formatFiatAmount (value: number, rate: number) {
  return `${(value * rate).toFixed(2)} USD`
}

export function formatLocaleTemplate (
  localeString: string,
  data: Record<string, string>
) {
  for (const [key, value] of Object.entries(data)) {
    const pattern = new RegExp(`{{\\s*${key}\\s*}}`)
    localeString = localeString.replace(pattern, value)
  }
  return localeString
}

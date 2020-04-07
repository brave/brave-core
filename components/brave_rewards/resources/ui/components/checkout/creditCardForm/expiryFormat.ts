/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type ExpiryError =
  '' |
  'invalid-month' |
  'invalid-year' |
  'date-in-past'

export class ExpiryFormat {
  fullYear: boolean

  constructor (init: { fullYear?: boolean } = {}) {
    this.fullYear = init.fullYear || false
  }

  parse (value: string, opts: { fullYear?: boolean } = {}): [string, string] {
    let [month = '', year = ''] = value.replace(/[^\d\/]/g, '').split('/')
    const yearLength = this.fullYear ? 4 : 2
    if (year.length > yearLength) {
      year = year.slice(0, yearLength)
    }
    if (opts.fullYear) {
      year = `20${year}`
    }
    return [month, year]
  }

  format (value: string) {
    let [month, year] = this.parse(value)
    if (!month) {
      return ''
    }
    const hasSeparator = year || /\/\s*$/.test(value)
    if (month.length === 1 && (/[2-9]/.test(month) || hasSeparator)) {
      month = `0${month}`
    }
    if (/\d \/$/.test(value)) {
      return month
    }
    if (!hasSeparator) {
      return month.length > 1 ? `${month} / ` : month
    }
    return `${month} / ${year}`
  }

  removeFormatting (value: string) {
    return value.replace(/\s*\/\s*/, '')
  }

  validate (value: string): ExpiryError {
    const currentMonth = new Date().getMonth() + 1
    let currentYear = new Date().getFullYear()
    if (!this.fullYear) {
      currentYear -= 2000
    }
    const [month, year] = this.parse(value).map(
      (x) => parseInt(x, 10) || 0
    )
    if (month < 1 || month > 12) {
      return 'invalid-month'
    }
    if (year < 0) {
      return 'invalid-year'
    }
    if (year < currentYear || year === currentYear && month < currentMonth) {
      return 'date-in-past'
    }
    return ''
  }

}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

const countryNames = new Intl.DisplayNames(undefined, { type: 'region' })

export function getCountryName (code: string) {
  return countryNames.of(code)
}

function getCountryOptions (countries: string[]) {
  return countries
    .map((code) => ({ code, name: countryNames.of(code) || '' }))
    .filter((item) => Boolean(item.name))
    .sort((a, b) => a.name?.localeCompare(b.name))
}

interface Props {
  countries: string[]
  placeholderText: string
  value: string
  onChange: (country: string) => void
  autoFocus?: boolean
}

export function CountrySelect (props: Props) {
  const onCountryChange = (event: React.FormEvent<HTMLSelectElement>) => {
    const { value } = event.currentTarget
    if (value !== props.value) {
      props.onChange(value)
    }
  }

  return (
    <select
      className={!props.value ? 'empty' : ''}
      value={props.value}
      onChange={onCountryChange}
      autoFocus={props.autoFocus}
      data-test-id='country-select'
    >
      <option value=''>{props.placeholderText}</option>
      {
        getCountryOptions(props.countries).map((option) => (
          <option key={option.code} value={option.code}>
            {option.name}
          </option>
        ))
      }
    </select>
  )
}

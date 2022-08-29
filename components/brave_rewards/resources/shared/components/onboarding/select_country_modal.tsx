/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { Modal } from '../modal'
import { GeoPinIcon } from './icons/geo_pin_icon'
import { TermsOfService } from '../terms_of_service'
import { CountrySelect } from './country_select'

import * as style from './select_country_modal.style'

interface Props {
  availableCountries: string[]
  onSave: (country: string) => void
}

export function SelectCountryModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [countryCode, setCountryCode] = React.useState('')

  const onSaveClick = () => {
    props.onSave(countryCode)
  }

  return (
    <Modal>
      <style.root>
        <style.header>
          <style.headerIcon><GeoPinIcon /></style.headerIcon>
          {getString('onboardingGeoHeader')}
        </style.header>
        <style.text>
          {getString('onboardingGeoText')}
        </style.text>
        <style.selectCountry>
          <CountrySelect
            countries={props.availableCountries}
            placeholderText={getString('onboardingSelectCountry')}
            value={countryCode}
            onChange={setCountryCode}
            autoFocus
          />
        </style.selectCountry>
        <style.enable>
          <button onClick={onSaveClick} disabled={!countryCode}>
            {getString('onboardingSave')}
          </button>
        </style.enable>
        <style.terms>
          <TermsOfService />
        </style.terms>
      </style.root>
    </Modal>
  )
}

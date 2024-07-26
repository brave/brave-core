/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useLocaleContext } from '../../lib/locale_strings'
import { Modal } from '../modal'
import { CountrySelect } from './country_select'

import { style } from './country_select_modal.style'

interface Props {
  loading: boolean
  countries: string[]
  defaultCountry: string
  onClose: () => void
  onCountrySelected: (countryCode: string) => void
}

export function CountrySelectModal(props: Props) {
  const { getString } = useLocaleContext()
  const [countryCode, setCountryCode] = React.useState('')

  function onContinueClick() {
    if (countryCode) {
      props.onCountrySelected(countryCode)
    }
  }

  return (
    <Modal
      className='country-select-modal'
      onEscape={props.loading ? undefined : props.onClose}
    >
      <Modal.Header onClose={props.onClose} closeDisabled={props.loading} />
      <div {...style}>
        <div className='graphic' />
        <div className='title'>
          {getString('countrySelectTitle')}
        </div>
        <div className='text'>
          {getString('countrySelectText')}
        </div>
        <div className='selector'>
          <CountrySelect
            countries={props.countries}
            defaultCountry={props.defaultCountry}
            placeholderText={getString('countrySelectPlaceholder')}
            value={countryCode}
            onChange={setCountryCode}
          />
        </div>
        <div className='actions'>
          <Button
            className='continue-button'
            isDisabled={!countryCode}
            isLoading={props.loading}
            onClick={onContinueClick}
          >
            {getString('continueButtonLabel')}
          </Button>
          <Button
            kind='plain'
            isDisabled={props.loading}
            onClick={props.onClose}
          >
            {getString('cancelButtonLabel')}
          </Button>
        </div>
      </div>
    </Modal>
  )
}

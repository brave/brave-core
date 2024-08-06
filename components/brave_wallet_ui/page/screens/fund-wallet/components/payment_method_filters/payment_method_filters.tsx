// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { DialogProps } from '@brave/leo/react/dialog'
import * as leo from '@brave/leo/tokens/css/variables'
import Checkbox from '@brave/leo/react/checkbox'

// types
import { MeldCountry, MeldPaymentMethod } from '../../../../../constants/types'

// utils
import { isComponentInStorybook } from '../../../../../utils/string-utils'

// styles
import { Dialog, Dropdown, DialogTitle } from '../shared/style'
import { Column, Row } from '../../../../../components/shared/style'

interface Props extends DialogProps {
  isLoading: boolean
  selectedCountryCode: string
  countries: MeldCountry[]
  paymentMethods: MeldPaymentMethod[]
  onSelectPaymentMethods: (paymentMethods: MeldPaymentMethod[]) => void
  onSelectCountry: (countryCode: string) => void
}

const PaymentMethodItem = ({
  isChecked,
  paymentMethod,
  onSelect
}: {
  isChecked: boolean
  paymentMethod: MeldPaymentMethod
  onSelect: (paymentMethod: MeldPaymentMethod) => void
}) => {
  const isDarkMode = window.matchMedia('(prefers-color-scheme: dark)').matches
  const logoUrl = isDarkMode
    ? paymentMethod.logoImages?.darkUrl
    : paymentMethod.logoImages?.lightUrl

  const isStorybook = isComponentInStorybook()

  return (
    <Row
      justifyContent='flex-start'
      gap={leo.spacing.s}
      alignItems='center'
    >
      <Checkbox
        checked={isChecked}
        size='normal'
        onChange={() => onSelect(paymentMethod)}
      >
        <Row
          gap={leo.spacing.s}
          alignItems='center'
        >
          <img
            width='19px'
            height='19px'
            src={isStorybook ? logoUrl : `chrome://image?${logoUrl}`}
          />

          {paymentMethod.name}
        </Row>
      </Checkbox>
    </Row>
  )
}

export const PaymentMethodFilters = (props: Props) => {
  const {
    isLoading,
    selectedCountryCode,
    countries,
    paymentMethods,
    onSelectCountry,
    onSelectPaymentMethods,
    ...rest
  } = props

  // state
  const [selectedPaymentMethods, setSelectedPaymentMethods] = React.useState<
    MeldPaymentMethod[]
  >(paymentMethods || [])

  // computed
  const selectedCountry = countries.find(
    (country) =>
      country.countryCode.toLowerCase() === selectedCountryCode.toLowerCase()
  )

  // methods
  const isPaymentMethodSelected = (paymentMethod: MeldPaymentMethod) =>
    selectedPaymentMethods.findIndex(
      (selectedPaymentMethod) =>
        selectedPaymentMethod.paymentMethod === paymentMethod.paymentMethod
    ) > -1

  const onSelectPaymentMethod = (paymentMethod: MeldPaymentMethod) => {
    const newSelectedPaymentMethods = isPaymentMethodSelected(paymentMethod)
      ? selectedPaymentMethods.filter(
          (selectedPaymentMethod) =>
            selectedPaymentMethod.paymentMethod !== paymentMethod.paymentMethod
        )
      : [...selectedPaymentMethods, paymentMethod]
    setSelectedPaymentMethods(newSelectedPaymentMethods)
    onSelectPaymentMethods(newSelectedPaymentMethods)
  }

  return (
    <Dialog
      showClose
      {...rest}
    >
      <DialogTitle slot='title'>Filters</DialogTitle>
      <Dropdown
        value={selectedCountryCode}
        onChange={(detail) => onSelectCountry(detail.value as string)}
      >
        <div slot='label'>Countries</div>
        <div slot='value'>{selectedCountry?.name}</div>
        {countries.map((country) => {
          return (
            <leo-option
              key={country.countryCode}
              value={country.countryCode}
            >
              {country.name}
            </leo-option>
          )
        })}
      </Dropdown>

      <Column
        justifyContent='flex-start'
        alignItems='flex-start'
        width='100%'
        gap={leo.spacing.l}
        margin={`${leo.spacing['3Xl']} 0 0 0`}
      >
        {paymentMethods.map((paymentMethod) => (
          <PaymentMethodItem
            key={paymentMethod.paymentMethod}
            isChecked={isPaymentMethodSelected(paymentMethod)}
            paymentMethod={paymentMethod}
            onSelect={onSelectPaymentMethod}
          />
        ))}
      </Column>
    </Dialog>
  )
}

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import WalletPageStory from '../../../../../stories/wrappers/wallet-page-story-wrapper'
import { PaymentMethodFilters } from './payment_method_filters'
import {
  mockMeldCountries,
  mockMeldPaymentMethods
} from '../../../../../common/constants/mocks'

export const _PaymentMethodFilters = () => {
  const [selectedCountryCode, setSelectedCountryCode] = React.useState('AS')
  return (
    <WalletPageStory>
      <PaymentMethodFilters
        isOpen={true}
        selectedCountryCode={selectedCountryCode}
        countries={mockMeldCountries}
        onSelectCountry={(countryCode: string) =>
          setSelectedCountryCode(countryCode)
        }
        isLoading={false}
        paymentMethods={mockMeldPaymentMethods}
        onSelectPaymentMethods={(paymentMethods) => {}}
      />
    </WalletPageStory>
  )
}

export default {
  component: _PaymentMethodFilters,
  title: 'Payment Method Filters'
}

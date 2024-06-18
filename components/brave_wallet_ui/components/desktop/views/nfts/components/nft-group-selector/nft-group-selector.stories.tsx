// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { NftDropdownOption, NftDropdown } from './nft-group-selector'

const options: NftDropdownOption[] = [
  { id: 'collected', label: 'Collected', labelSummary: '1' }
]

export const NftGroupSelector = {
  render: () => <NftDropdown
    selectedOptionId={options[0].id}
    options={options}
    onSelect={(optionId) => console.log(optionId)}
  />
}

export default {
  title: 'Nft Dropdown'
}

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { DropdownOption, NftGroupSelector } from './nft-group-selector'

const options: DropdownOption[] = [{ id: 'collected', label: 'Collected' }]

export const _NftGroupSelector = () => {
  return <NftGroupSelector selectedOption={options[0]} options={options} onSelect={(optionId) => console.log(optionId)}/>
}

_NftGroupSelector.storyName = 'NftGroupSelector'

export default _NftGroupSelector

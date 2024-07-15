// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styles
import { FilterButton, FilterLabel, FilterClose } from './explore_web3.style'

interface Props {
  label: string
  onClick: () => void
}

export const DappFilter = ({ label, onClick }: Props) => {
  return (
    <FilterButton onClick={onClick}>
      <FilterLabel>
        {label}
        <div slot='icon-after'>
          <FilterClose />
        </div>
      </FilterLabel>
    </FilterButton>
  )
}

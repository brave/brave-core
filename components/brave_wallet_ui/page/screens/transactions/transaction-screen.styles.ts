// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Icons
import {
  NoTransactionsIconDark,
  NoTransactionsIconLight
} from '../../../assets/svg-icons/empty-state-icons'

export const SearchAndFiltersRow = styled.div`
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  width: 100%;
  gap: 14px;
`

export const EmptyTransactionsIcon = styled.div`
  width: 100px;
  height: 100px;
  background-repeat: no-repeat;
  background-size: 100%;
  background-position: center;
  background-image: url(${NoTransactionsIconLight});
  @media (prefers-color-scheme: dark) {
    background-image: url(${NoTransactionsIconDark});
  }
`

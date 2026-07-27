// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Icons
import {
  NoAccountsIconDark,
  NoAccountsIconLight,
  NoTransactionsIconDark,
  NoTransactionsIconLight,
} from '../../../../../assets/svg-icons/empty-state-icons'

// Shared Styles
import { WalletButton } from '../../../../../components/shared/style'

export const EmptyStateIcon = styled.div`
  width: 100px;
  height: 100px;
  background-repeat: no-repeat;
  background-size: 100%;
  background-position: center;
  margin-bottom: 16px;
`

export const EmptyTransactionsIcon = styled(EmptyStateIcon)`
  background-image: url(${NoTransactionsIconLight});
  @media (prefers-color-scheme: dark) {
    background-image: url(${NoTransactionsIconDark});
  }
`

export const EmptyAccountsIcon = styled(EmptyStateIcon)`
  background-image: url(${NoAccountsIconLight});
  @media (prefers-color-scheme: dark) {
    background-image: url(${NoAccountsIconDark});
  }
`

export const ToggleVisibilityButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  pointer-events: auto;
  border: none;
`

export const EyeIcon = styled(Icon)`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`

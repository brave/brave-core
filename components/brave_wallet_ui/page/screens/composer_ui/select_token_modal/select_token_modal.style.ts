// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Row, Column, Text } from '../../../../components/shared/style'
import {
  layoutSmallWidth //
} from '../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const AccountSection = styled(Row)`
  border-bottom: 1px solid ${leo.color.divider.subtle};
`

export const ScrollContainer = styled(Column)<{
  noPadding: boolean
}>`
  flex: 1;
  overflow-x: hidden;
  overflow-y: auto;
  padding: ${(p) => (p.noPadding ? '0px' : '0px 40px')};
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: ${(p) => (p.noPadding ? '0px' : '0px 8px')};
  }
`

export const SendOptionsRow = styled(Row)`
  padding: 0px 40px 16px 40px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: 0px 16px 16px 16px;
  }
`

export const SearchBarRow = styled(Row)`
  padding: 0px 40px 24px 40px;
  @media screen and (max-width: ${layoutSmallWidth}px) {
    padding: 0px 16px 16px 16px;
  }
`

export const AccountNameText = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 14px;
  margin-right: 4px;
  font-weight: 500;
`

export const AccountAddressText = styled(AccountNameText)`
  color: ${leo.color.text.tertiary};
  margin-right: 0px;
`

export const BalanceText = styled(AccountNameText)`
  color: ${leo.color.text.secondary};
  margin-right: 0px;
`

export const NoAssetsText = styled(Text)`
  color: ${leo.color.text.tertiary};
  line-height: 22px;
`

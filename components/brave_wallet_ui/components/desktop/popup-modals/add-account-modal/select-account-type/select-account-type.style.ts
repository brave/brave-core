// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column, Text, WalletButton } from '../../../../shared/style'
import { layoutPanelWidth } from '../../../wallet-page-wrapper/wallet-page-wrapper.style'

export const StyledWrapper = styled(Column)`
  padding: 0px 32px 32px 32px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 20px 20px 20px;
  }
`

export const SectionWrapper = styled(Column)`
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: ${leo.radius.l};
  overflow: hidden;
`

export const SelectAccountItemWrapper = styled(Column)`
  border-bottom: 1px solid ${leo.color.divider.subtle};
  &:last-child {
    border-bottom: none;
  }
`

export const SelectAccountTitle = styled(Text)`
  font: ${leo.font.heading.h4};
  letter-spacing: ${leo.typography.letterSpacing.large};
`

export const TestAccountsButton = styled(WalletButton)<{ isOpen: boolean }>`
  width: 100%;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 16px 24px;
  font: ${leo.font.default.semibold};
  letter-spacing: ${leo.typography.letterSpacing.default};
  color: ${leo.color.text.secondary};
  border-bottom: ${({ isOpen }) =>
    isOpen ? `1px solid ${leo.color.divider.subtle}` : 'none'};
`

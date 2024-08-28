// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'

// Shared Styles
import {
  layoutPanelWidth //
} from '../../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const AccountName = styled.h3`
  color: ${color.text.primary};
  font: ${font.heading.h3};
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  max-width: 185px;
  margin: 0;
  padding: 0;
`

export const AccountAddress = styled.span`
  display: flex;
  padding: ${spacing.s} ${spacing.m};
  margin-top: ${spacing.m} 0;
  align-items: flex-start;
  background-color: ${color.page.background};
  border-radius: ${spacing.m};
  color: ${color.text.secondary};
  font: ${font.xSmall.semibold};
  line-height: 16px;
  @media (max-width: ${layoutPanelWidth}) {
    background-color: transparent;
  }
`

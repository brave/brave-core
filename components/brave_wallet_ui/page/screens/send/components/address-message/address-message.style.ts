// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { Icon } from '../../shared.styles'
import { WalletButton } from '../../../../../components/shared/style'

export const LearnMoreLink = styled.a`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 20px;
  color: ${(p) => p.theme.color.interactive05};
  margin: 0px;
  padding: 0px;
  text-decoration: none;
  cursor: pointer;
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.color.interactive06};
  }
`

export const HowToSolveButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 20px;
  color: ${(p) => p.theme.color.interactive05};
  margin: 0px;
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.color.interactive06};
  }
`

export const ErrorIcon = styled(Icon) <{ type: 'error' | 'warning' }>`
  background-color: ${(p) => p.type === 'error' ? p.theme.color.errorBorder : p.theme.color.warningBorder};
  margin-right: 10px;
`

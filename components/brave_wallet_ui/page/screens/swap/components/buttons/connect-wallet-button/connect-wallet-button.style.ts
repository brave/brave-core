// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { StyledButton, StyledDiv, Icon } from '../../shared-swap.styles'

export const Button = styled(StyledButton) <{ isConnected: boolean }>`
  --background-color-disconnected: ${p => p.theme.color.interactive04};
  @media (prefers-color-scheme: dark) {
    --background-color-disconnected: ${p => p.theme.color.interactive05};
  }
  background-color: ${p =>
    p.isConnected
      ? p.theme.color.background01
      : 'var(--background-color-disconnected)'
  };
  border-radius: 48px;
  color: ${p => (p.isConnected ? p.theme.color.text01 : p.theme.palette.white)};
  font-size: 14px;
  padding: ${p => (p.isConnected ? '8px 16px' : '10px 22px')};
  box-shadow: ${p =>
    p.isConnected
      ? '0px 0px 10px rgba(0, 0, 0, 0.05)'
      : 'none'
  };
  @media screen and (max-width: 570px) {
    font-size: 12px;
    padding: ${p => (p.isConnected ? '4px 8px' : '6px 16px')};
  }
`

export const AccountCircle = styled(StyledDiv) <{ orb: string }>`
  width: 24px;
  height: 24px;
  border-radius: 100%;
  background-image: url(${p => p.orb});
  background-size: cover;
  margin-right: 8px;
`

export const ButtonIcon = styled(Icon)`
  display: none;
  color: ${p => p.theme.color.text01};
  @media screen and (max-width: 570px) {
    display: block;
  }
`

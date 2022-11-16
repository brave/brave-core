// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { StyledDiv, Icon, StyledButton } from '../../shared.styles'

export const Wrapper = styled.div`
  display: flex;
  position: relative;
  height: 100%;
  align-items: center;
  width: 40px;
  justify-content: flex-end;
`

export const Tip = styled(StyledDiv)`
  position: absolute;
  border-radius: 16px;
  padding: 16px;
  z-index: 10;
  right: 8px;
  top: 42px;
  width: 220px;
  background-color: ${(p) => p.theme.color.background01};
  border: 1px solid ${(p) => p.theme.color.divider01};
  box-shadow: 0px 4px 20px rgba(0, 0, 0, 0.1);
  white-space: normal; 
`

export const TipIcon = styled(Icon)`
  background-color: ${(p) => p.theme.color.text02};
  margin-right: 8px;
`

export const AddressLink = styled(StyledButton)`
  --text-color: ${(p) => p.theme.color.interactive05};
  @media (prefers-color-scheme: dark) {
    --text-color: ${(p) => p.theme.palette.blurple500};
  }
  color: var(--text-color);
  font-size: 14px;
  padding: 0px;
`

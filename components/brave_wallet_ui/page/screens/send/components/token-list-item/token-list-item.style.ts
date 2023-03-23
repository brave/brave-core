// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// Shared Styles
import { StyledButton, StyledDiv, Row } from '../../shared.styles'

import {
  AssetIconProps,
  AssetIconFactory
} from '../../../../../components/shared/style'

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const NetworkIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  bottom: -3px;
  right: -3px;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 100%;
  padding: 2px;
  z-index: 3;
`

export const ButtonWrapper = styled(StyledDiv)`
  --background-hover: ${(p) => p.theme.palette.white};
  --shadow-hover: 0px 0px 16px rgba(99, 105, 110, 0.18);
  @media (prefers-color-scheme: dark) {
    --background-hover: transparent;
    --shadow-hover: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
  flex-direction: row;
  background-color: transparent;
  border-radius: 8px;
  justify-content: space-between;
  white-space: nowrap;
  width: 100%;
  margin-bottom: 8px;
  &:hover {
    background-color: var(--background-hover);
    box-shadow: var(--shadow-hover);
  }
`

export const Button = styled(StyledButton)`
  background-color: transparent;
  justify-content: space-between;
  padding: 8px;
  white-space: nowrap;
  width: 100%;
`

export const IconsWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
  margin-right: 16px;
`

export const IconAndName = styled(Row)`
  width: 80%;
  white-space: pre-line;
`

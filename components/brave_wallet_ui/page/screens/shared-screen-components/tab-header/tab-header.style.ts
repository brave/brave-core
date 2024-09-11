// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Assets
import BraveLogoLight from '../../../../assets/svg-icons/wallet_logo_light.svg'
import BraveLogoDark from '../../../../assets/svg-icons/wallet_logo_dark.svg'

// Shared Styles
import { StyledDiv, StyledButton } from '../../send/shared.styles'

export const HeaderWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: space-between;
  padding: 16px 32px 0px 32px;
  margin-bottom: 45px;
  top: 0;
  left: 0;
  right: 0;
  width: 100vw;
  box-sizing: border-box;
  position: absolute;
  z-index: 10;
`

export const BraveLogo = styled(StyledDiv)`
  height: 30px;
  width: 100px;
  background-image: url(${BraveLogoLight});
  background-size: cover;
  margin: 0px 12px 4px 0px;
  @media (prefers-color-scheme: dark) {
    background-image: url(${BraveLogoDark});
  }
`

export const SettingsWrapper = styled(StyledDiv)`
  position: relative;
`

export const SettingsButton = styled(StyledButton)`
  background-color: ${leo.color.container.background};
  border-radius: 12px;
  box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.05);
  height: 40px;
  width: 40px;
  &:hover {
    background-color: ${leo.color.container.interactive};
  }
  @media (prefers-color-scheme: dark) {
    box-shadow: 0px 0px 16px rgba(0, 0, 0, 0.36);
  }
`

export const SettingsIcon = styled(Icon)`
  --leo-icon-size: 20px;
  color: ${leo.color.icon.default};
`

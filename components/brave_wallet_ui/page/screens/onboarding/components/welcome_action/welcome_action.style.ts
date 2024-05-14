// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

import { WalletButton } from '../../../../../components/shared/style'

export const WelcomeActionWrapper = styled(WalletButton)`
  display: flex;
  padding: ${leo.spacing['3Xl']};
  flex-direction: column;
  width: 100%;
  min-height: 167px;
  justify-content: flex-start;
  align-items: flex-start;
  background: ${leo.color.container.background};
  border-radius: ${leo.spacing.xl};
  box-shadow: 0px 4px 16px -2px rgba(0, 0, 0, 0.08);
  border: none;
  cursor: pointer;
`

export const Title = styled.h6`
  width: 100%;
  font: ${leo.font.heading.h3};
  margin: 0;
  color: ${leo.color.text.primary};
  text-align: left;
`

export const Description = styled.p`
  width: 100%;
  font: ${leo.font.default.regular};
  color: ${leo.color.text.secondary};
  text-align: left;
  margin: 0;
`

export const TitleIcon = styled(Icon)`
  display: flex;
  align-items: center;
  justify-content: center;
  --leo-icon-size: 24px;
  color: ${leo.color.icon.interactive};
  flex-shrink: 0;
  flex: 1;
`

export const IconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 48px;
  height: 48px;
  background-color: ${leo.color.container.interactive};
  border-radius: 50%;
  z-index: 0;
`

export const WalletIcon = styled(Icon)`
  --leo-icon-size: 24px;
  border-radius: 50%;
  z-index: 1;
`

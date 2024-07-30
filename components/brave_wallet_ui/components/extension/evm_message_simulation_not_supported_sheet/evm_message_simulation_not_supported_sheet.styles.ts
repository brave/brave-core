// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// styles
import { Column, WalletButton } from '../../shared/style'

export const alertItemGap = leo.spacing.m

export const TitleText = styled.h4`
  color: ${leo.color.text.primary};
  text-align: center;
  font: ${leo.font.heading.h4};
  padding: 8px 32px 0px 32px;
`

export const AlertTextContainer = styled.div`
  color: ${leo.color.systemfeedback.infoText};
  text-align: center;
  font: ${leo.font.small.regular};
`

export const CollapseTitle = styled.div`
  display: inline-flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  color: ${leo.color.text.interactive};
  text-align: center;
  font: ${leo.font.small.semibold};
  text-decoration: none;
  cursor: pointer;
  width: 100%;
`

export const CollapseIcon = styled(Icon)`
  max-width: 16px;
  --leo-icon-size: 16px;
  --leo-icon-color: ${leo.color.icon.interactive};
  margin-left: 4px;
`

export const CollapseTitleRow = styled(WalletButton)`
  cursor: pointer;
  font-family: 'Poppins';
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  width: 100%;
  position: relative;
  box-sizing: border-box;
  background: none;
  border: none;
`

export const FullWidthChildrenColumn = styled(Column)`
  & * {
    width: 100%;
  }
`

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: ${color.container.background};
  overflow: hidden;
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-itmes: flex-start;
  align-self: stretch;
`

export const SettingsList = styled.div`
  display: flex;
  padding: ${spacing.m} ${spacing.s};
  flex-direction: column;
  align-items: center;
  gap: ${spacing.s};
  align-self: stretch;
`

export const Setting = styled.div`
  display: flex;
  padding: ${spacing.m};
  align-items: center;
  gap: ${spacing.m};
  align-self: stretch;
  cursor: pointer;
`

export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background: ${color.divider.subtle};
`

export const SettingLabelBox = styled.div`
  display: flex;
  padding: 0px ${spacing.m};
  flex-direction: column;
  align-items: flex-start;
  flex: 1 0 0;
`

export const SettingLabel = styled.div`
  color: ${color.text.primary};
  font: ${font.default.regular};
  align-self: stretch;
`

export const SettingDesc = styled.div`
  color: ${color.text.tertiary};
  font: ${font.small.regular};
  align-self: stretch;
`

export const StyledIcon = styled(Icon)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${color.icon.default};
`

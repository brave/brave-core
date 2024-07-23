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
  flex-direction: column;
  align-items: center;
`

export const Setting = styled.div`
  display: flex;
  padding: ${spacing.l} ${spacing['2Xl']};
  align-items: center;
  gap: ${spacing.xl};
  justify-content: space-between;
  width: 100%;
  align-self: stretch;
  cursor: pointer;

  &:focus-visible {
    outline: 1.5px solid ${color.systemfeedback.focusDefault};
    outline-offset: -6px;
  }
`
export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background: ${color.divider.subtle};
`

export const SettingLabel = styled.div`
  color: ${color.text.primary};
  font: ${font.default.regular};
`

export const StyledIcon = styled(Icon)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${color.icon.default};
  padding: 8px 0px;
`

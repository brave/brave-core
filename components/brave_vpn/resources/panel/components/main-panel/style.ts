// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Alert from '@brave/leo/react/alert'
import Icon from '@brave/leo/react/icon'
import { color, effect, gradient, font, radius, spacing } from '@brave/leo/tokens/css/variables'

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 24px;
  position: relative;
  z-index: 2;
`

export const StyledIcon = styled(Icon)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${color.icon.default};
`

export const VpnLogo = styled(Icon)`
  --leo-icon-size: 24px;
  --leo-icon-color: ${gradient.iconsActive};
`

export const SettingsButton = styled.button`
  position: absolute;
  right: 16px;
  top: 16px;
  border: 0;
  padding: 0;
  background: transparent;
  cursor: pointer;
`

export const PanelHeader = styled.section`
  display: flex;
  padding: ${spacing.m} 0;
  align-items: center;
  gap: ${spacing.m};
`

export const PanelTitle = styled.span`
  color: ${color.text.primary};
  font: ${font.heading.h3};
`

export const RegionSelectorButton = styled.button`
  --border-color: transparent;
  display: flex;
  align-items: center;
  align-self: stretch;
  gap: ${spacing.xl};
  border-radius: ${radius.m};
  border: 2px solid var(--border-color);
  box-shadow: ${effect.elevation['01']};
  background-color: ${color.container.background};
  padding: ${spacing.m} ${spacing.xl};
  cursor: pointer;

  &:hover {
    background: ${color.container.highlight};
  }

  &:focus-visible {
    --border-color: ${color.primary[40]};
    outline: 0;
  }
`


export const StyledAlert = styled(Alert)`
  --leo-alert-padding: ${spacing.xl};
  margin-bottom: ${spacing.m};
  align-self: stretch;
`

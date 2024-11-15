// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Input from '@brave/leo/react/input'
import Label from '@brave/leo/react/label'
import { font, color } from '@brave/leo/tokens/css/variables'

// Shared Styles
import {
  layoutPanelWidth //
} from '../../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const SearchInput = styled(Input).attrs({
  mode: 'filled',
  size: window.innerWidth <= layoutPanelWidth ? 'small' : 'normal'
})`
  margin-top: 2px;
  width: 100%;
  padding-bottom: 8px;
  @media (max-width: ${layoutPanelWidth}px) {
    size: small;
  }
`
export const Wrapper = styled.div`
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  z-index: 999;
  background-color: white;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
`

export const CurrencyImage = styled.img`
  width: 32px;
  height: 32px;
  border-radius: 50%;
`
export const CurrencySymbol = styled.span`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border-radius: 50%;
  font: ${font.default.semibold};
  color: ${color.container.background};
  background-color: ${color.primary[50]};
`

export const CurrencyName = styled.span`
  color: ${color.text.primary};
  font: ${font.default.semibold};
  text-overflow: ellipsis;
  text-align: left;
  white-space: nowrap;
  overflow: hidden;
`

export const CurrencyCode = styled.span`
  display: flex;
  color: ${color.text.primary};
  font: ${font.default.regular};
  text-transform: uppercase;
`

export const SelectedLabel = styled(Label).attrs({
  mode: 'default',
  color: 'purple'
})`
  text-transform: uppercase;
`

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import { color, font } from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column } from '../../../../../components/shared/style'
import { AmountInput as Input } from '../../../composer_ui/shared_composer.style'
import {
  layoutPanelWidth //
} from '../../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const Wrapper = styled(Column)`
  align-items: flex-end;
  @media (max-width: ${layoutPanelWidth}px) {
    align-items: flex-start;
    width: 100%;
  }
`

export const ButtonWrapper = styled(Column)`
  align-items: flex-end;
  @media (max-width: ${layoutPanelWidth}px) {
    flex-direction: row;
    justify-content: space-between;
    align-items: center;
    width: 100%;
  }
`

export const CurrencyCode = styled.span`
  color: ${color.text.primary};
  font: ${font.heading.h1};
  width: 70px;
  text-transform: uppercase;
`

export const AmountInput = styled(Input).attrs({
  hasError: false,
  type: 'number'
})`
  color: ${color.text.primary};
  font: ${font.heading.h1};
  text-align: left;
  width: 60px;
`

export const AmountEstimate = styled.span`
  color: ${color.text.interactive};
  font: ${font.default.semibold};
  @media (max-width: ${layoutPanelWidth}px) {
    font: ${font.default.regular};
  }
`

export const SwapVerticalIcon = styled(Icon).attrs({ name: 'swap-vertical' })`
  --leo-icon-color: ${color.icon.interactive};
  --leo-icon-size: 20px;
`

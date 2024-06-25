// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, font } from '@brave/leo/tokens/css/variables'
import { AmountInput as Input } from '../../../composer_ui/shared_composer.style'
import { layoutPanelWidth } from '../../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import Icon from '@brave/leo/react/icon'

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
`

export const LabelWrapper = styled.div`
  display: flex;
  width: 100%;
  gap: 8px;
  justify-content: flex-end;

  @media (max-width: ${layoutPanelWidth}px) {
    justify-content: flex-start;
  }
`

export const AmountWrapper = styled.div`
  display: grid;
  grid-template-columns: 1fr;
  width: 100%;
  align-items: center;
  gap: 8px;

  @media (max-width: ${layoutPanelWidth}px) {
    grid-template-columns: 2fr 1fr;
    gap: 16px;
  }
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

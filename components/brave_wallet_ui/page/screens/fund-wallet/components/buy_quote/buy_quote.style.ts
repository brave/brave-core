// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import Label from '@brave/leo/react/label'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'

// Shared Styles
import {
  layoutPanelWidth //
} from '../../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'
import { Column } from '../../../../../components/shared/style'

export const StyledWrapper = styled.div<{ isOpen?: boolean }>`
  display: flex;
  flex-direction: column;
  padding: ${spacing.l} ${spacing.xl};
  border-radius: 8px;
  border: 1px solid
    ${(p) => (p.isOpen ? color.button.background : color.divider.subtle)};
  background-color: ${color.container.background};
  width: 100%;
`

export const ProviderName = styled.p`
  color: ${color.text.primary};
  font: ${font.default.semibold};
  text-transform: capitalize;
  margin: 0;
  padding: 0;
`

export const Estimate = styled.p`
  color: ${color.text.secondary};
  font: ${font.xSmall.regular};
  padding: 0;
  margin: 0;
`

export const ProviderImage = styled.img`
  width: 40px;
  height: 40px;
  margin-right: ${spacing.xl};
`

export const PaymentMethodsWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;

  gap: ${spacing.l};
  padding: 4px 8px;
  border-radius: 4px;
  background-color: ${color.container.highlight};
`

export const PaymentMethodIcon = styled(Icon)`
  --leo-icon-size: 20px;
  --leo-icon-color: ${color.icon.default};
`

export const CaratIcon = styled(Icon)<{
  isOpen?: boolean
}>`
  --leo-icon-size: 24px;
  --leo-icon-color: ${color.button.background};
  transform: rotate(${(p) => (p.isOpen ? '180deg' : '0deg')});
  transition: transform 0.5s;
  margin-left: 8px;
`

export const WrapperForPadding = styled(Column)`
  margin-top: ${spacing.xl};
  padding-left: 56px;
  @media (max-width: ${layoutPanelWidth}px) {
    padding-left: 0px;
  }
`

export const QuoteDetailsWrapper = styled(Column)`
  padding: ${spacing.xl} ${spacing['2Xl']};
  border-radius: 12px;
  background-color: ${color.page.background};
`

export const QuoteDetailsLabel = styled.p`
  color: ${color.text.secondary};
  font: ${font.small.regular};
  color: ${color.text.primary};
  margin: 0;
  padding: 0;
`

export const QuoteDetailsValue = styled.p`
  color: ${color.text.primary};
  font: ${font.small.regular};
  color: ${color.text.primary};
  margin: 0;
  padding: 0;
`

export const Divider = styled.div`
  height: 1px;
  background-color: ${color.divider.subtle};
`

export const QuoteTotal = styled.p`
  color: ${color.text.primary};
  font: ${font.small.semibold};
  margin: 0;
  padding: 0;
`

export const BuyButton = styled(Button).attrs({
  kind: 'filled'
})`
  @media (max-width: ${layoutPanelWidth}px) {
    width: 100%;
  }
`

export const BestOptionLabel = styled(Label).attrs({
  mode: 'default',
  color: 'green'
})`
  --leo-label-padding: 12px;
  color: ${color.text.primary};
  text-transform: capitalize;
`

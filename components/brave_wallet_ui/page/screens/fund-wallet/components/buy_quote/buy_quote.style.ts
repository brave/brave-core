// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

export const StyledWrapper = styled.div<{ isOpen?: boolean }>`
  display: flex;
  flex-direction: column;
  padding: ${spacing.l} ${spacing.xl};
  border-radius: 8px;
  border: 1px solid
    ${(p) => (p.isOpen ? color.button.background : color.divider.subtle)};
  background-color: ${color.container.background};
`

export const ProviderName = styled.p`
  color: ${color.text.primary};
  font: ${font.default.semibold};
  margin: 0;
  padding: 0;
`

export const Estimate = styled.p`
  color: ${color.text.secondary};
  font: ${font.xSmall.regular};
`

export const ProviderImage = styled.img`
  width: 40px;
  height: 40px;
  margin-right: ${spacing.xl};
`

export const PaymentMethodsWrapper = styled.div`
  display: flex;
  gap: ${spacing.l};
  padding: 4px 8px;
  border-radius: 4px;
  background-color: ${color.container.highlight};
`

export const PaymentMethodIcon = styled(Icon)`
    --leo-icon-size: 12px;
`

export const CaratIcon = styled(Icon)<{
    isOpen?: boolean
}>`
    --leo-icon-size: 24px;
    transform: rotate(${(p) => (p.isOpen ? '180deg' : '0deg')});
    transition: transform 0.5s;
    margin-left: 8px;
`


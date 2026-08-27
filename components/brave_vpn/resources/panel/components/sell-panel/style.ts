// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import guardianLogoUrl from '../../assets/svg-icons/guardian-logo.svg'
import { color, effect, font, gradient, icon, radius, spacing } from '@brave/leo/tokens/css/variables'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: linear-gradient(
    180deg,
    ${color.primitive.neutral[15]} 0%,
    ${color.primitive.neutral[5]} 100%
  );
  border-radius: ${radius.xl};
  box-shadow: ${effect.elevation['02']};
  overflow: hidden;
  font: ${font.default.regular};
`

export const PanelContent = styled.section`
  width: 100%;
  box-sizing: border-box;
  display: flex;
  padding: ${spacing['3Xl']} ${spacing['2Xl']};
  flex-direction: column;
  align-items: center;
  gap: ${spacing['2Xl']};
`

export const PanelHeader = styled.section`
  display: flex;
  align-items: center;
  justify-content: center;
  gap: ${spacing.m};
`

export const ProductTitle = styled.h2`
  color: ${color.white};
  font: ${font.heading.h2};
  margin: ${spacing.none};
`

export const PoweredBy = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  gap: ${spacing.m};

  span {
    color: ${color.white};
    text-align: center;
  }
`

export const MainLogo = styled(Icon)`
  --leo-icon-size: 40px;
  --leo-icon-color: ${gradient.hero};
`

export const GuardianLogo = styled.i`
  width: 91px;
  height: 20px;
  background-image: url(${guardianLogoUrl});
  user-select: none;
  pointer-events: none;
  display: inline-block;
`

export const SellingPoints = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  align-self: stretch;
  text-wrap: pretty;
`

export const SellingPoint = styled.div`
  display: flex;
  align-items: center;
  gap: ${spacing.m};
  padding: ${spacing.s} ${spacing.none};
  align-self: stretch;
`

export const SellingPointIcon = styled(Icon)`
  --leo-icon-size: ${icon.s};
  --leo-icon-color: ${color.primitive.neutral[70]};
`

export const SellingPointLabel = styled.span`
  flex: 1;
  color: ${color.primitive.neutral[95]};
  font: ${font.default.regular};
`

export const ActionArea = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: ${spacing.xl};
  align-self: stretch;
`

export const ActionButton = styled(Button)`
  align-self: stretch;
`

export const ActionLink = styled.a`
  color: ${color.white};
  font: ${font.small.link};
  text-decoration-line: underline;

  &:focus-visible {
    outline: none;
    box-shadow: ${effect.focusState};
  }
`

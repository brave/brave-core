// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import sellGraphicUrl from '../../assets/svg-icons/sell-graphic.svg'
import guardianLogoUrl from '../../assets/svg-icons/guardian-logo.svg'
import { color, font, gradient, icon, spacing } from '@brave/leo/tokens/css/variables'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: #0F0663;
  position: relative;
  font: ${font.default.regular};
`

export const ProductTitle = styled.h3`
  color: #FFF;
  font: ${font.heading.h3};
  margin: 0 0 ${spacing.xl} 0;
`

export const PoweredBy = styled.div`
  display: flex;
  align-items: center;

  span {
    color: #FFF;
    text-align: center;
  }
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
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
`

export const SellGraphic = styled.div`
  width: 336px;
  height: 132px;
  background-image: url(${sellGraphicUrl});
  position: absolute;
  top: 0;
  left: 0px;
  user-select: none;
  pointer-events: none;
`

export const MainLogo = styled(Icon)`
  --leo-icon-size: 48px;
  --leo-icon-color: ${gradient.iconsActive};
`

export const GuardianLogo = styled.i`
  width: 91px;
  height: 20px;
  background-image: url(${guardianLogoUrl});
  user-select: none;
  pointer-events: none;
  display: inline-block;
  margin-left: 8px;
`

export const SellingPoints = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: ${spacing.m};
  align-self: stretch;
`

export const SellingPoint = styled.div`
  display: flex;
  align-items: center;
  gap: ${spacing.m};
  align-self: stretch;
`

export const SellingPointIcon = styled(Icon)`
  align-self: start;
  margin: 1px;
  --leo-icon-size: ${icon.m};
  --leo-icon-color: ${gradient.toolbarBackground};
`

export const SellingPointLabel = styled.span`
  color: #FFF;
  font: ${font.default.regular};
`

export const ActionArea = styled.div`
  display: flex;
  padding: 0px;
  flex-direction: column;
  align-items: center;
  gap: ${spacing.xl};
  align-self: stretch;
`

export const ActionButton = styled(Button)`
  --leo-button-color: rgba(255, 255, 255, 0.20);
  align-self: stretch;
`

export const ActionLink = styled.a`
  color: #FFF;
  font: ${font.small.link};
  text-decoration-line: underline;

  &:focus-visible {
    outline: 1.5px solid ${color.systemfeedback.focusDefault};
  }
`

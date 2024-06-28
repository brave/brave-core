/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { color, font, gradient } from '@brave/leo/tokens/css/variables'

export const Content = styled.div.attrs({ 'data-theme': 'dark' })`
  color: ${color.white};
  font: ${font.small.regular};
`

export const WelcomeText = styled.div``

export const ActionsWrapper = styled.div`
  margin-top: 16px;
  text-align: center;
  display: flex;
  flex-direction: column;
  justify-content: stretch;
  gap: 16px;
`

export const BraveTalkIcon = styled.div`
  --leo-icon-size: 24px;
  --leo-icon-color: ${gradient.iconsActive};
`

export const StyledTitle = styled.div`
  margin-bottom: 24px;
  font: ${font.heading.h4};
  color: ${color.white};
  display: flex;
  align-items: center;
  gap: 8px;
`

export const Privacy = styled.div``

export const PrivacyLink = styled.a`
  color: ${color.white};
  text-decoration: none;
`

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
import { ComponentType } from 'react'
import styled from 'styled-components'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'

export const StyledContent = styled('div')<{}>`
  font-size: 14px;
  font-family: Poppins, sans-serif;
  letter-spacing: 0;
  color: ${p => p.theme.color.text};
  line-height: 26px;
  margin-bottom: 25px;
`

export const StyledDoneWrapper = styled('div')<{}>`
  display: flex;
  justify-content: center;
  margin-top: 40px;
`

export const StyledActionsWrapper = styled('div')<{}>`
  margin-top: 40px;
  display: flex;
  justify-content: center;
`

export const ActionButton = styled(Button as ComponentType<ButtonProps>)`
  margin: 0 8px;
`

export const StyledTitleWrapper = styled('div')<{}>`
  width: 100%;
  text-align: center;
  margin-bottom: 20px;
`

export const StyledTitle = styled('span')<{}>`
  font-size: 22px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 40px;
  font-family: ${p => p.theme.fontFamily.heading};
  text-transform: capitalize;
`

export const StyledControlWrapper = styled('div')<{}>`
  width: 100%;
  margin-bottom: 30px;
`

export const StyledText = styled('p')<{}>`
  font-size: 15px;
  letter-spacing: 0;
  line-height: 26px;
  font-family: Poppins, sans-serif;
  color: ${p => p.theme.color.text};
`

export const StyledTextWrapper = styled('div')<{}>`
  margin-bottom: 25px;
`

export const StyledLink = styled('a')<{}>`
  color: ${p => p.theme.color.brandBatInteracting};
  font-weight: 600;
`

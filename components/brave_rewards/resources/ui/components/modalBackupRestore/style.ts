/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import { ComponentType } from 'react'
import styled from 'brave-ui/theme'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'

interface StyleProps {
  isError?: boolean
  error?: boolean
}

export const StyledContent = styled<{}, 'div'>('div')`
  font-size: 14px;
  font-family: Muli, sans-serif;
  letter-spacing: 0;
  color: ${p => p.theme.color.text};
  line-height: 26px;
  margin-bottom: 40px;
`

export const StyledImport = styled<{}, 'label'>('label')`
  color: #4c54d2;
  cursor: pointer;
`

export const StyledButtonWrapper = styled<{}, 'div'>('div')`
  display: flex;
  margin-top: 20px;
  margin-bottom: 40px;
`

export const GroupedButton = styled(Button as ComponentType<ButtonProps>)`
  margin: 0 4px;

  @media (max-width: 410px) {
    min-width: 74px;
    font-size: 9px;
  }
  @media (max-width: 370px) {
    min-width: 65px;
    font-size: 9px;
  }
`

export const StyledDoneWrapper = styled<{}, 'div'>('div')`
  display: flex;
  justify-content: center;
  margin-top: 40px;
`

export const StyledStatus = styled<StyleProps, 'div'>('div')`
  margin: ${p => p.isError ? 0 : -16}px 0 16px;
  border-radius: 6px;
  overflow: hidden;
`

export const StyledActionsWrapper = styled<{}, 'div'>('div')`
  margin-top: 40px;
  display: flex;
  justify-content: center;
`

export const ActionButton = styled(Button as ComponentType<ButtonProps>)`
  margin: 0 8px;
`

export const StyledTitleWrapper = styled<{}, 'div'>('div')`
  width: 100%;
  text-align: center;
  margin-bottom: 20px;
`

export const StyledTitle = styled<{}, 'span'>('span')`
  font-size: 22px;
  font-weight: normal;
  letter-spacing: 0;
  line-height: 40px;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const StyledSafe = styled<{}, 'span'>('span')`
  font-weight: 700;
  margin-right: 3px;
  color: ${p => p.theme.color.brandBatInteracting};
`

export const StyledControlWrapper = styled<{}, 'div'>('div')`
  width: 100%;
  margin-bottom: 30px;
`

export const StyledText = styled<{}, 'p'>('p')`
  font-size: 15px;
  letter-spacing: 0;
  line-height: 26px;
  font-family: Muli, sans-serif;
  color: ${p => p.theme.color.text};
`

export const StyledTextWrapper = styled<{}, 'div'>('div')`
  margin-bottom: 25px;
`

export const StyledLink = styled<{}, 'a'>('a')`
  color: ${p => p.theme.color.brandBatInteracting};
  cursor: pointer;
  display: inline-block;
  font-weight: 700;
  margin-left: 3px;
`

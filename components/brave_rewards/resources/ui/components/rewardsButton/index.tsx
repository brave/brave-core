/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'
import {
  StyledButtonWrapper,
  StyledButtonText,
  StyledIcon
} from './style'

export type Type = 'tip' | 'opt-in' | 'cta-opt-in'

export interface Props {
  text?: string
  testId?: string
  type?: Type
  disabled?: boolean
  onClick?: () => void
  icon?: React.ReactNode
}

export default class RewardsButton extends React.PureComponent<Props, {}> {
  render () {
    const { text, testId, type, disabled, onClick, icon } = this.props

    return (
      <StyledButtonWrapper
        buttonType={type}
        disabled={disabled}
        onClick={onClick}
        data-test-id={testId}
      >
        <StyledButtonText hasIcon={!!icon}>
          {text}
          {
            icon
            ? <StyledIcon>
                {icon}
              </StyledIcon>
            : null
          }
        </StyledButtonText>
      </StyledButtonWrapper>
    )
  }
}

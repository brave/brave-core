/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '../../../components/buttonsIndicators/button'
import { StyledWrapper, StyledText, StyledButton } from './style'

export interface Props {
  id?: string
  text?: string
  buttonText: string
  onButtonClick: () => void
}

export default class GrantError extends React.PureComponent<Props, {}> {
  render () {
    const { id, text, buttonText, onButtonClick } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledText>
          {text}
        </StyledText>
        <StyledButton>
          <Button
            text={buttonText}
            size={'call-to-action'}
            type={'accent'}
            onClick={onButtonClick}
          />
        </StyledButton>
      </StyledWrapper>
    )
  }
}

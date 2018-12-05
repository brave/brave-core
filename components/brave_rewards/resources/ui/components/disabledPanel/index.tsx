/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledOff,
  StyledText,
  StyledLink,
  StyledTitleWrapper
} from './style'

import { getLocale } from '../../../helpers'

export interface Props {
  testId?: string
  isPrivate?: boolean
  onLinkOpen: () => void
}

export default class DisabledPanel extends React.PureComponent<Props, {}> {
  render () {
    const { testId, isPrivate, onLinkOpen } = this.props

    return (
      <StyledWrapper data-test-id={testId}>
        <StyledTitleWrapper>
          <StyledTitle>
            {getLocale('disabledPanelTitle')}
          </StyledTitle>
          <StyledOff>
            {getLocale('disabledPanelOff')}
          </StyledOff>
        </StyledTitleWrapper>
        <StyledText>
          {isPrivate ? getLocale('disabledPanelPrivateText') : getLocale('disabledPanelText')}
          <StyledLink onClick={onLinkOpen}>
            {isPrivate ? getLocale('learnMore') : getLocale('disabledPanelSettings')}
          </StyledLink>
        </StyledText>
      </StyledWrapper>
    )
  }
}

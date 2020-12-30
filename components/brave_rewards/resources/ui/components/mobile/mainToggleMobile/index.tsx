/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledLogoWrapper
} from './style'
import { getLocale } from 'brave-ui/helpers'
import { BatColorIcon } from 'brave-ui/components/icons'

export interface Props {
  testId?: string
}

export default class MainToggleMobile extends React.PureComponent<Props, {}> {
  render () {
    const { testId } = this.props

    return (
      <StyledWrapper data-test-id={testId}>
          <StyledLogoWrapper>
            <BatColorIcon />
          </StyledLogoWrapper>
          <StyledTitle>
            {getLocale('braveRewards')}
          </StyledTitle>
      </StyledWrapper>
    )
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledTitle, StyledContent } from './style'
import { getLocale } from '../../../helpers'

interface Props {
  id?: string
}

export default class PanelOff extends React.PureComponent<Props, {}> {
  render () {
    return (
      <StyledWrapper id={this.props.id}>
        <StyledTitle>{getLocale('rewardsPanelOffText1')}</StyledTitle>
        <StyledContent>{getLocale('rewardsPanelOffText2')}</StyledContent>
      </StyledWrapper>
    )
  }
}

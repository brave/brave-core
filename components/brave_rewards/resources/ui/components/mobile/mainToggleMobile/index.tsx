/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledLeft,
  StyledRight,
  StyledLogoWrapper
} from './style'
import Toggle from '../../../../components/formControls/toggle/index'
import { getLocale } from '../../../../helpers'
import { BatColorIcon } from '../../../../components/icons'

export interface Props {
  enabled: boolean
  onToggle: () => void
  id?: string
  testId?: string
}

export default class MainToggleMobile extends React.PureComponent<Props, {}> {
  render () {
    const { id, enabled, onToggle, testId } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledLeft>
          <StyledLogoWrapper>
            <BatColorIcon />
          </StyledLogoWrapper>
          <StyledTitle>
            {getLocale('braveRewards')}
          </StyledTitle>
        </StyledLeft>
        <StyledRight>
          <Toggle checked={enabled} onToggle={onToggle} testId={testId} />
        </StyledRight>
      </StyledWrapper>
    )
  }
}

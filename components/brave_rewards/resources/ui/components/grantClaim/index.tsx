/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledIcon, StyledText, StyledClaim } from './style'
import { getLocale } from '../../../helpers'
import { GiftIcon } from '../../../components/icons'

export interface Props {
  id?: string
  onClaim: () => void
}

export default class GrantClaim extends React.PureComponent<Props, {}> {
  render () {
    const { id, onClaim } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledIcon>
          <GiftIcon />
        </StyledIcon>
        <StyledText>{getLocale('newGrant')}</StyledText>
        <StyledClaim onClick={onClaim}>{getLocale('claim')}</StyledClaim>
      </StyledWrapper>
    )
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledLink,
  StyledWrapper
} from './style'

import { getLocale } from 'brave-ui/helpers'

export interface Props {
  onAdsHistoryOpen?: () => void
}

export default class ShowAdsHistory extends React.PureComponent<Props, {}> {
  render () {
    const { onAdsHistoryOpen } = this.props

    return (
      <StyledWrapper>
        <StyledLink onClick={onAdsHistoryOpen}>
          {getLocale('openAdsHistory')}
        </StyledLink>
      </StyledWrapper>
    )
  }
}

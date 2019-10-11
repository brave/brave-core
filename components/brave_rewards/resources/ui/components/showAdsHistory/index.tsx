/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledLink,
  StyledWrapper,
  StyledDisabledLink
} from './style'

import { getLocale } from 'brave-ui/helpers'

export interface Props {
  onAdsHistoryOpen?: () => void
  notEmpty: boolean
}

export default class ShowAdsHistory extends React.PureComponent<Props, {}> {
  render () {
    const { onAdsHistoryOpen, notEmpty } = this.props

    return (
      <StyledWrapper>
        {
          notEmpty ?
          <StyledLink onClick={onAdsHistoryOpen}>
              {getLocale('openAdsHistory')}
          </StyledLink> :
          <StyledDisabledLink>
            {getLocale('openAdsHistory')}
          </StyledDisabledLink>
        }
      </StyledWrapper>
    )
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'
import {
  StyledWrapper,
  StyledCard,
  StyledTitle,
  StyledText
} from './style'
import { getLocale } from 'brave-ui/helpers'

export default class DisabledBox extends React.PureComponent<{}, {}> {
  render () {
    return (
      <StyledWrapper>
        <StyledCard>
          <StyledTitle>
            {getLocale('whyBraveRewards')}
          </StyledTitle>
          <StyledText>
            {getLocale('rewardsOffText5')}
          </StyledText>
          <StyledTitle>
            {getLocale('rewardsOffText3')}
          </StyledTitle>
          <StyledText>
            {getLocale('rewardsOffText4')}
          </StyledText>
        </StyledCard>
      </StyledWrapper>
    )
  }
}

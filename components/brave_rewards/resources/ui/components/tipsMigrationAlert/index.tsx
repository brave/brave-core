/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'
import { getLocale } from '../../../helpers'

import {
  StyledWrapper,
  StyledAlertIcon,
  StyledInfo,
  StyledMessage,
  StyledMonthlyTips,
  StyledReviewWrapper,
  StyledReviewList
} from './style'
import { AlertCircleIcon } from '../../../components/icons'

export interface Props {
  testId?: string
  onReview: () => void
}

export default class TipsMigrationAlert extends React.PureComponent<Props, {}> {
  render () {
    const { testId, onReview } = this.props

    return (
      <StyledWrapper data-test-id={testId}>
        <StyledAlertIcon>
          <AlertCircleIcon />
        </StyledAlertIcon>
        <StyledInfo>
          <StyledMessage>
            {getLocale('reviewSitesMsg')}
          </StyledMessage>
          <StyledMonthlyTips>
            {getLocale('monthlyTips')}
          </StyledMonthlyTips>
        </StyledInfo>
        <StyledReviewWrapper>
          <StyledReviewList onClick={onReview}>
            {getLocale('learnMore')}
          </StyledReviewList>
        </StyledReviewWrapper>
      </StyledWrapper>
    )
  }
}

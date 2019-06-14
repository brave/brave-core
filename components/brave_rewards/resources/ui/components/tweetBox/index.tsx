/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledTweetBox,
  StyledTweetText,
  StyledTweetTimestamp,
  StyledTwitterIcon
} from './style'

import {
  TwitterColorIcon
} from '../../../components/icons'

export interface Props {
  tweetText: string
  tweetTimestamp: number
}

export default class TweetBox extends React.Component<Props, {}> {
  formatTimestamp = (timestamp: Date) => {
    const dateOptions = { month: 'short', day: 'numeric' }
    if (new Date().getFullYear() !== timestamp.getFullYear()) {
      (dateOptions as any)['year'] = 'numeric'
    }
    return timestamp.toLocaleString(navigator.language, dateOptions)
  }

  render () {
    const tweetTimestamp = new Date(this.props.tweetTimestamp * 1000)
    return (
      <StyledTweetBox>
        <StyledTwitterIcon>
          <TwitterColorIcon />
        </StyledTwitterIcon>
        <StyledTweetTimestamp>
          {this.formatTimestamp(tweetTimestamp)}
        </StyledTweetTimestamp>
        <StyledTweetText>
          {this.props.tweetText}
        </StyledTweetText>
      </StyledTweetBox>
    )
  }
}

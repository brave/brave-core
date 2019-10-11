/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledMediaBox,
  StyledMediaText,
  StyledMediaTimestamp,
  StyledMediaIcon
} from './style'

import {
  TwitterColorIcon,
  RedditColorIcon
} from 'brave-ui/components/icons'

export interface Props {
  mediaType: 'twitter' | 'reddit'
  mediaText: string
  mediaTimestamp: number
  mediaTimetext?: string
}

export default class MediaBox extends React.Component<Props, {}> {
  formatTimestamp = (timestamp: Date) => {
    const dateOptions = { month: 'short', day: 'numeric' }
    if (new Date().getFullYear() !== timestamp.getFullYear()) {
      (dateOptions as any)['year'] = 'numeric'
    }
    return timestamp.toLocaleString(navigator.language, dateOptions)
  }

  getMediaIcon = () => {
    return this.props.mediaType === 'twitter'
    ? <TwitterColorIcon />
    : this.props.mediaType === 'reddit'
    ? <RedditColorIcon />
    : null
  }

  getMediaTimestamp = () => {
    const { mediaType, mediaTimestamp, mediaTimetext } = this.props
    return mediaType === 'twitter'
      ? this.formatTimestamp(new Date(mediaTimestamp * 1000))
      : mediaType === 'reddit'
      ? mediaTimetext
      : null
  }

  render () {
    return (
      <StyledMediaBox>
        <StyledMediaIcon>
          {this.getMediaIcon()}
        </StyledMediaIcon>
        <StyledMediaTimestamp>
          {this.getMediaTimestamp()}
        </StyledMediaTimestamp>
        <StyledMediaText>
          {this.props.mediaText}
        </StyledMediaText>
      </StyledMediaBox>
    )
  }
}

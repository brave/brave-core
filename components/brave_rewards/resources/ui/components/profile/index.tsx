/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledContent,
  StyledImageWrapper,
  StyledImage,
  StyledVerified,
  StyledTitleWrap,
  StyledTitle,
  StyledProvider,
  StyledProviderWrap,
  StyledInlineVerified,
  StyledVerifiedText
} from './style'
import { getLocale } from '../../../helpers'
import { VerifiedFillIcon } from '../../../components/icons'

export type Provider = 'twitter' | 'youtube' | 'twitch'

export interface Props {
  id?: string
  src?: string
  title: string
  type?: 'big' | 'small'
  provider?: Provider
  verified?: boolean
}

/*
  TODO
  - add fallback image
 */
export default class Profile extends React.PureComponent<Props, {}> {
  static defaultProps = {
    type: 'small'
  }

  getProviderName (provider: Provider) {
    switch (provider) {
      case 'youtube':
        return `${getLocale('on')} YouTube`
      case 'twitter':
        return `${getLocale('on')} Twitter`
      case 'twitch':
        return `${getLocale('on')} Twitch`
    }
  }

  getSrc (src?: string) {
    return src ? src : ''
  }

  render () {
    const { id, type, provider, src, title, verified } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledImageWrapper>
          <StyledImage src={this.getSrc(src)} type={type} />
          {
            verified && type === 'small'
            ? <StyledVerified><VerifiedFillIcon /></StyledVerified>
            : null
          }
        </StyledImageWrapper>
        <StyledContent type={type}>
          <StyledTitleWrap type={type}>
            <StyledTitle type={type}>{title}</StyledTitle>
            {
              provider
              ? <StyledProvider type={type}>{this.getProviderName(provider)}</StyledProvider>
              : null
            }
          </StyledTitleWrap>
          {
            verified && type === 'big'
            ? <StyledProviderWrap>
              <StyledInlineVerified><VerifiedFillIcon /></StyledInlineVerified> <StyledVerifiedText>{getLocale('verifiedPublisher')}</StyledVerifiedText>
            </StyledProviderWrap>
            : null
          }
        </StyledContent>
      </StyledWrapper>
    )
  }
}

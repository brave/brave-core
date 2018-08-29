/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../../helpers'

import {
  StyledEnableTipsSection,
  StyledEnableTipsInner,
  StyledText,
  StyledProviderImg,
  StyledEnableTips,
  StyledProviderName,
  StyledToggleOuter,
  StyledToggleInner,
  StyledThumbsUpIcon
} from './style'
import {
  ThumbsUpIcon,
  TwitchColorIcon,
  TwitterColorIcon,
  YoutubeColorIcon
} from '../../../components/icons'
import { Toggle } from '../../../components'

export type Provider = 'twitter' | 'youtube' | 'twitch'

export interface Props {
  id?: string
  provider?: Provider
  onToggleTips: () => void
  tipsEnabled?: boolean
}

export default class ToggleTips extends React.PureComponent<Props, {}> {
  getProviderImg (provider: Provider) {
    switch (provider) {
      case 'youtube':
        return <YoutubeColorIcon />
      case 'twitter':
        return <TwitterColorIcon />
      case 'twitch':
        return <TwitchColorIcon />
    }
  }

  getProviderName (provider: Provider) {
    return provider.toUpperCase()
  }

  getProviderText (provider: Provider) {
    return `${this.getProviderName(provider)} ${getLocale('for')}`
  }

  render () {
    const { id, provider, onToggleTips, tipsEnabled } = this.props

    if (!provider) {
      return null
    }

    return (
      <StyledEnableTipsSection id={id}>
        <StyledEnableTipsInner>
          <StyledEnableTips>
            {getLocale('enableTips')}
          </StyledEnableTips>
          <StyledText>
            {getLocale('on')}
          </StyledText>
          <StyledProviderImg>
            {this.getProviderImg(provider)}
          </StyledProviderImg>
          <StyledProviderName>
            {this.getProviderName(provider)}
          </StyledProviderName>
          <StyledText>
            {getLocale('for')}
          </StyledText>
          <StyledThumbsUpIcon>
            <ThumbsUpIcon/>
          </StyledThumbsUpIcon>
          <StyledToggleOuter>
            <StyledToggleInner>
              <Toggle
                size={'small'}
                onToggle={onToggleTips}
                checked={tipsEnabled}
              />
            </StyledToggleInner>
          </StyledToggleOuter>
        </StyledEnableTipsInner>
      </StyledEnableTipsSection>
    )
  }
}

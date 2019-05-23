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
  StyledProviderWrapRefreshing,
  StyledProviderWrapRefreshFinished,
  StyledInlineVerified,
  StyledVerifiedText,
  StyledInlineUnVerified,
  StyledVerifiedCheckLink,
  StyledVerifiedCheckNoLink,
  StyledRefreshCheckOverlayFinished,
  StyledRefresh,
  StyledRefreshLoaderFinished,
  StyledRefreshOverlay,
  StyledRefreshOverlayFinished,
  StyledRefreshFinished,
  StyledVerifiedDivider
} from './style'
import { getLocale } from '../../../helpers'
import { VerifiedSIcon, UnVerifiedSIcon, LoaderIcon, CheckmarkCircleS } from '../../../components/icons'

export type Provider = 'twitter' | 'youtube' | 'twitch'

export interface Props {
  id?: string
  src?: string
  title: string
  type?: 'big' | 'small'
  provider?: Provider
  verified?: boolean
  tableCell?: boolean
  showUnVerifiedHelpIcon?: boolean
  refreshingPublisher?: boolean
  publisherRefreshed?: boolean
  onRefreshPublisher?: () => void
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

  getVerifiedInfo = () => {
    return (
      <>
        <StyledInlineVerified>
          <VerifiedSIcon />
        </StyledInlineVerified>{' '}
        <StyledVerifiedText>
          {getLocale('verifiedPublisher')}
        </StyledVerifiedText>
      </>
    )
  }

  getDefaultVerifiedPanelWrap = () => {
    return (
      <StyledProviderWrap>
        {
          this.getVerifiedInfo()
        }
      </StyledProviderWrap>
    )
  }

  getVerifiedPanelWrapRefreshing = () => {
    return (
      <>
        <StyledProviderWrapRefreshing>
          {
            this.getVerifiedInfo()
          }
        </StyledProviderWrapRefreshing>
        <StyledRefreshOverlay>
          <StyledRefresh>
            <LoaderIcon />
          </StyledRefresh>
        </StyledRefreshOverlay>
      </>
    )
  }

  getVerifiedPanelWrapRefreshFinished = () => {
    return (
      <>
        <StyledProviderWrapRefreshFinished>
          {
            this.getVerifiedInfo()
          }
        </StyledProviderWrapRefreshFinished>
        <StyledRefreshOverlayFinished>
          <StyledRefreshLoaderFinished>
            <LoaderIcon />
          </StyledRefreshLoaderFinished>
        </StyledRefreshOverlayFinished>
        <StyledRefreshCheckOverlayFinished>
          <StyledRefreshFinished>
            <CheckmarkCircleS />
          </StyledRefreshFinished>
        </StyledRefreshCheckOverlayFinished>
      </>
    )
  }

  getVerifiedPanelWrapping = () => {
    const {
      refreshingPublisher,
      publisherRefreshed
    } = this.props

    return (
      !refreshingPublisher && !publisherRefreshed ?
        this.getDefaultVerifiedPanelWrap()
    : !publisherRefreshed && refreshingPublisher ?
        this.getVerifiedPanelWrapRefreshing()
    :
      this.getVerifiedPanelWrapRefreshFinished()
    )
  }

  getUnverifiedInfo = () => {
    return (
      <>
        <StyledInlineUnVerified>
          <UnVerifiedSIcon />
        </StyledInlineUnVerified>{' '}
        <StyledVerifiedText>
          {getLocale('unVerifiedPublisher')}
        </StyledVerifiedText>
        <StyledVerifiedDivider />
      </>
    )
  }

  getDefaultUnverifiedPanelWrap = () => {
    const { onRefreshPublisher } = this.props
    return (
      <StyledProviderWrap>
        {
          this.getUnverifiedInfo()
        }
        <StyledVerifiedCheckLink onClick={onRefreshPublisher}>
          {getLocale('unVerifiedCheck')}
        </StyledVerifiedCheckLink>
      </StyledProviderWrap>
    )
  }

  getUnverifiedPanelWrapRefreshing = () => {
    return (
      <>
        <StyledProviderWrapRefreshing>
          {
            this.getUnverifiedInfo()
          }
          <StyledVerifiedCheckLink>
            {getLocale('unVerifiedCheck')}
          </StyledVerifiedCheckLink>
        </StyledProviderWrapRefreshing>
        <StyledRefreshOverlay>
          <StyledRefresh>
            <LoaderIcon />
          </StyledRefresh>
        </StyledRefreshOverlay>
      </>
    )
  }

  getUnverifiedPanelWrapRefreshFinished = () => {
    return (
      <>
        <StyledProviderWrapRefreshFinished>
          {
            this.getUnverifiedInfo()
          }
          <StyledVerifiedCheckNoLink>
            {getLocale('unVerifiedCheck')}
          </StyledVerifiedCheckNoLink>
        </StyledProviderWrapRefreshFinished>
        <StyledRefreshOverlayFinished>
          <StyledRefreshLoaderFinished>
            <LoaderIcon />
          </StyledRefreshLoaderFinished>
        </StyledRefreshOverlayFinished>
        <StyledRefreshCheckOverlayFinished>
          <StyledRefreshFinished>
            <CheckmarkCircleS />
          </StyledRefreshFinished>
        </StyledRefreshCheckOverlayFinished>
      </>
    )
  }

  getUnverifiedPanelWrapping = () => {
    const {
      refreshingPublisher,
      publisherRefreshed
    } = this.props

    return (
      !publisherRefreshed && !refreshingPublisher ?
        this.getDefaultUnverifiedPanelWrap()
      : !publisherRefreshed && refreshingPublisher ?
        this.getUnverifiedPanelWrapRefreshing()
      :
        this.getUnverifiedPanelWrapRefreshFinished()
    )
  }

  render () {
    const {
      id,
      type,
      provider,
      src,
      title,
      verified,
      showUnVerifiedHelpIcon
    } = this.props

    return (
      <StyledWrapper id={id}>
        <StyledImageWrapper type={type}>
          <StyledImage src={this.getSrc(src)} />
          {verified && type === 'small' ? (
            <StyledVerified>
              <VerifiedSIcon />
            </StyledVerified>
          ) : null}
        </StyledImageWrapper>
        <StyledContent>
          <StyledTitleWrap>
            <StyledTitle type={type}>{title}</StyledTitle>
            {provider ? (
              <StyledProvider type={type}>
                {this.getProviderName(provider)}
              </StyledProvider>
            ) : null}
          </StyledTitleWrap>
          {verified && type === 'big' ? this.getVerifiedPanelWrapping()
          : showUnVerifiedHelpIcon ? this.getUnverifiedPanelWrapping()
          : null}
        </StyledContent>
      </StyledWrapper>
    )
  }
}

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
import { getLocale } from 'brave-ui/helpers'
import { VerifiedSIcon, UnVerifiedSIcon, LoaderIcon, CheckmarkCircleS } from 'brave-ui/components/icons'

export type Provider = 'twitter' | 'youtube' | 'twitch' | 'reddit' | 'vimeo' | 'github'

export interface Props {
  id?: string
  src?: string
  title: string
  type?: 'big' | 'small' | 'mobile'
  provider?: Provider
  verified?: boolean
  tableCell?: boolean
  showUnVerifiedHelpIcon?: boolean
  refreshingPublisher?: boolean
  publisherRefreshed?: boolean
  showUnVerified?: boolean
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
      case 'reddit':
        return `${getLocale('on')} Reddit`
      case 'vimeo':
        return `${getLocale('on')} Vimeo`
      case 'github':
        return `${getLocale('on')} GitHub`
    }
  }

  getSrc (src?: string) {
    return src || ''
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
    const { showUnVerified } = this.props

    return (
      <StyledProviderWrap>
        {
          this.getVerifiedInfo()
        }
        {
          showUnVerified
          ? <>
              <StyledVerifiedDivider />
              {this.getUnverifiedAction()}
            </>
          : null
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
    const { showUnVerified } = this.props

    return (
      <>
        <StyledProviderWrapRefreshFinished>
          {
            this.getVerifiedInfo()
          }
          {
            showUnVerified
            ? <>
                <StyledVerifiedDivider />
                <StyledVerifiedCheckNoLink>
                  {getLocale('unVerifiedCheck')}
                </StyledVerifiedCheckNoLink>
              </>
            : null
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
      !refreshingPublisher && !publisherRefreshed
        ? this.getDefaultVerifiedPanelWrap()
    : !publisherRefreshed && refreshingPublisher
        ? this.getVerifiedPanelWrapRefreshing()
    : this.getVerifiedPanelWrapRefreshFinished()
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
    return (
      <StyledProviderWrap>
        {
          this.getUnverifiedInfo()
        }
        {
          this.getUnverifiedAction()
        }
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

  getUnverifiedAction = () => {
    const { onRefreshPublisher } = this.props

    return (
      <StyledVerifiedCheckLink onClick={onRefreshPublisher} data-test-id={'unverified-check-button'}>
        {getLocale('unVerifiedCheck')}
      </StyledVerifiedCheckLink>
    )
  }

  getUnverifiedPanelWrapping = () => {
    const {
      refreshingPublisher,
      publisherRefreshed
    } = this.props

    return (
      !publisherRefreshed && !refreshingPublisher
        ? this.getDefaultUnverifiedPanelWrap()
      : !publisherRefreshed && refreshingPublisher
        ? this.getUnverifiedPanelWrapRefreshing()
      : this.getUnverifiedPanelWrapRefreshFinished()
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
      <StyledWrapper id={id} type={type}>
        <StyledImageWrapper type={type}>
          <StyledImage src={this.getSrc(src)} />
          {verified && type === 'small' ? (
            <StyledVerified>
              <VerifiedSIcon />
            </StyledVerified>
          ) : null}
        </StyledImageWrapper>
        <StyledContent type={type}>
          <StyledTitleWrap type={type}>
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

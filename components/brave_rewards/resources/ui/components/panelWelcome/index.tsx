/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from 'brave-ui/helpers'

import {
  StyledWrapper,
  StyledInnerWrapper,
  StyledHeaderText,
  StyledBatLogo,
  StyledTitle,
  StyledDescText,
  StyledFooterText,
  StyledTrademark,
  StyledErrorMessage,
  StyledJoinButton,
  StyledTOSWrapper,
  StyledServiceText,
  StyledServiceLink
} from './style'

import { BatColorIcon, LoaderIcon } from 'brave-ui/components/icons'

export type Variant = 'one' | 'two'

export interface Props {
  id?: string
  variant?: Variant
  creating?: boolean
  error?: boolean
  onlyAnonWallet?: boolean
  moreLink?: () => void
  optInAction: () => void
  optInErrorAction: () => void
  onTOSClick?: () => void
  onPrivacyClick?: () => void
}

export default class PanelWelcome extends React.PureComponent<Props, {}> {
  get locale () {
    const { onlyAnonWallet, variant } = this.props

    return {
      one: {
        header: 'welcomeHeaderOne',
        title: 'braveRewards',
        desc: 'welcomeDescOne',
        button: 'welcomeButtonTextOne',
        footer: 'welcomeFooterTextOne'
      },
      two: {
        header: 'welcomeHeaderTwo',
        title: 'braveRewards',
        desc: onlyAnonWallet ? 'welcomeDescPoints' : 'welcomeDescTwo',
        button: 'welcomeButtonTextTwo',
        footer: 'welcomeFooterTextTwo'
      }
    }[variant || 'one']
  }

  render () {
    const {
      id,
      optInAction,
      optInErrorAction,
      moreLink,
      onTOSClick,
      onPrivacyClick
    } = this.props

    let props = {}

    if (moreLink) {
      props = {
        onClick: moreLink
      }
    }

    return (
      <StyledWrapper id={id}>
        <StyledInnerWrapper>
          <StyledHeaderText>
            {getLocale(this.locale.header)}
          </StyledHeaderText>
          <StyledBatLogo>
            <BatColorIcon/>
          </StyledBatLogo>
          <StyledTitle level={4}>
            {getLocale(this.locale.title)}
          </StyledTitle>
          <StyledTrademark>TM</StyledTrademark>
          <StyledDescText>
            {getLocale(this.locale.desc)}
          </StyledDescText>
          {
            this.props.creating && !this.props.error
            ? <StyledJoinButton
              level='secondary'
              size='call-to-action'
              type='subtle'
              text={getLocale('braveRewardsCreatingText')}
              disabled={true}
              data-test-id='optInAction'
              icon={{
                image: <LoaderIcon />,
                position: 'after'
              }}
            />
            : this.props.error
              ? <>
                  <StyledErrorMessage>
                    {getLocale('walletFailedTitle')}
                  </StyledErrorMessage>
                  <StyledJoinButton
                    level='secondary'
                    size='call-to-action'
                    type='subtle'
                    text={getLocale('walletFailedButton')}
                    onClick={optInErrorAction}
                    data-test-id='optInErrorAction'
                  />
                </>
              : <StyledJoinButton
                size='call-to-action'
                type='subtle'
                level='secondary'
                onClick={optInAction}
                text={getLocale(this.locale.button)}
              />
          }
          {
            onTOSClick && onPrivacyClick
            ? <StyledTOSWrapper>
                <StyledServiceText>
                  {getLocale('serviceTextPanelWelcome')} <StyledServiceLink onClick={onTOSClick}>{getLocale('termsOfService')}</StyledServiceLink> {getLocale('and')} <StyledServiceLink onClick={onPrivacyClick}>{getLocale('privacyPolicy')}</StyledServiceLink>.
                </StyledServiceText>
              </StyledTOSWrapper>
            : null
          }
          <StyledFooterText {...props}>
            {getLocale(this.locale.footer)}
          </StyledFooterText>
        </StyledInnerWrapper>
      </StyledWrapper>
    )
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '../../../helpers'

import {
  StyledWrapper,
  StyledInnerWrapper,
  StyledHeaderText,
  StyledBatLogo,
  StyledTitle,
  StyledDescText,
  StyledFooterText,
  StyledButtonWrapper,
  StyledTrademark
} from './style'

import { BatColorIcon } from '../../../components/icons'
import Button from '../../../components/buttonsIndicators/button'

export type Variant = 'one' | 'two'

export interface Props {
  id?: string
  variant?: Variant
  moreLink?: () => void
  optInAction: () => void
}

export default class PanelWelcome extends React.PureComponent<Props, {}> {
  get locale () {
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
        desc: 'welcomeDescTwo',
        button: 'welcomeButtonTextTwo',
        footer: 'welcomeFooterTextTwo'
      }
    }[this.props.variant || 'one']
  }

  render () {
    const { id, optInAction, moreLink } = this.props

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
          <StyledButtonWrapper>
            <Button
              size='call-to-action'
              type='subtle'
              level='secondary'
              onClick={optInAction}
              text={getLocale(this.locale.button)}
            />
          </StyledButtonWrapper>
          <StyledFooterText {...props}>
            {getLocale(this.locale.footer)}
          </StyledFooterText>
        </StyledInnerWrapper>
      </StyledWrapper>
    )
  }
}

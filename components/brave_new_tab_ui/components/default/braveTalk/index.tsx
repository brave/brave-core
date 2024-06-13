/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import createWidget from '../widget/index'
import { getLocale } from '../../../../common/locale'

import {
  Content,
  WelcomeText,
  ActionsWrapper,
  BraveTalkIcon,
  StyledTitle,
  Privacy,
  PrivacyLink
} from './style'
import { StyledTitleTab, StyledCard } from '../widgetCard'
import { braveTalkWidgetUrl } from '../../../constants/new_tab_ui'

interface Props {
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
}

class BraveTalk extends React.PureComponent<Props, {}> {
  renderTitle () {
    return (
      <>
        <BraveTalkIcon><Icon name='product-brave-talk' /></BraveTalkIcon>
        {getLocale('braveTalkWidgetTitle')}
      </>
    )
  }

  renderTitleTab () {
    return (
      <StyledTitleTab onClick={this.props.onShowContent}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  shouldCreateCall = () => {
    window.open(braveTalkWidgetUrl, '_self', 'noopener')
  }

  render () {
    if (!this.props.showContent) {
      return this.renderTitleTab()
    }

    return (
      <StyledCard>
        <StyledTitle>
          {this.renderTitle()}
        </StyledTitle>
        <Content>
          <WelcomeText>
            {getLocale('braveTalkWidgetWelcomeTitle')}
          </WelcomeText>
          <ActionsWrapper>
            <Button onClick={this.shouldCreateCall}>
              {getLocale('braveTalkWidgetStartButton')}
            </Button>
            <Privacy>
              <PrivacyLink
                rel='noopener'
                target='_blank'
                href='https://brave.com/privacy/browser/#brave-talk-learn'
              >
                {getLocale('braveTalkWidgetAboutData')}
              </PrivacyLink>
            </Privacy>
          </ActionsWrapper>
        </Content>
      </StyledCard>
    )
  }
}

export const BraveTalkWidget = createWidget(BraveTalk)

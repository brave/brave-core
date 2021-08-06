/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'
import { getLocale } from '../../../../common/locale'

import {
  WidgetWrapper,
  Header,
  Content,
  WelcomeText,
  ActionsWrapper,
  CallButton,
  BraveTalkIcon,
  StyledTitle,
  Privacy,
  PrivacyLink
} from './style'
import { StyledTitleTab } from '../widgetTitleTab'
import BraveTalkSvg from './assets/brave-talk-svg'
import { braveTalkWidgetUrl } from '../../../constants/new_tab_ui'

interface Props {
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
}

class BraveTalk extends React.PureComponent<Props, {}> {

  getButtonText = () => {
    return getLocale('braveTalkWidgetStartButton')
  }

  renderTitle () {
    return (
      <Header>
        <StyledTitle>
          <BraveTalkIcon>
            <BraveTalkSvg />
          </BraveTalkIcon>
          <>
            {getLocale('braveTalkWidgetTitle')}
          </>
        </StyledTitle>
      </Header>
    )
  }

  renderTitleTab () {
    const { onShowContent, stackPosition } = this.props

    return (
      <StyledTitleTab onClick={onShowContent} stackPosition={stackPosition}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  shouldCreateCall = (event: any) => {
    event.preventDefault()
    window.open(braveTalkWidgetUrl, '_self', 'noopener')
  }

  render () {
    const {
      showContent
    } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper>
          {this.renderTitle()}
          <Content>
            <WelcomeText>
              {getLocale('braveTalkWidgetWelcomeTitle')}
            </WelcomeText>
            <ActionsWrapper>
              <CallButton onClick={this.shouldCreateCall}>
                {getLocale('braveTalkWidgetStartButton')}
              </CallButton>
              <Privacy>
                <PrivacyLink
                  rel={'noopener'}
                  target={'_blank'}
                  href={'https://brave.com/privacy/browser/#brave-talk-learn'}
                >
                  {getLocale('braveTalkWidgetAboutData')}
                </PrivacyLink>
              </Privacy>
            </ActionsWrapper>
          </Content>
      </WidgetWrapper>
    )
  }
}

export const BraveTalkWidget = createWidget(BraveTalk)

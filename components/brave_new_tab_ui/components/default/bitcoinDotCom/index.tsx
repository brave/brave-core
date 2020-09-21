/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'
import * as Styled from './style'
import { StyledTitleTab } from '../widgetTitleTab'
import BitcoinDotComLogo from './assets/logo.png'

interface State {
}

interface Props {
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
}

class BitcoinDotCom extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
  }

  renderTitle () {
    const { showContent } = this.props

    return (
      <Styled.Header>
        <Styled.Title contentShowing={showContent}>
          <Styled.BitcoinDotComIcon>
            <Styled.BitcoinDotComIconImg src={BitcoinDotComLogo} />
          </Styled.BitcoinDotComIcon>
          <Styled.TitleText>
            {'Bitcoin.com'}
          </Styled.TitleText>
        </Styled.Title>
      </Styled.Header>
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

  render () {
    if (!this.props.showContent) {
      return this.renderTitleTab()
    }

    return (
      <Styled.WidgetWrapper tabIndex={0}>
        {this.renderTitle()}
        {'Placeholder'}
      </Styled.WidgetWrapper>
    )
  }
}

export const BitcoinDotComWidget = createWidget(BitcoinDotCom)

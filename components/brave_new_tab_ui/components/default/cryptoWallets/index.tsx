/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import createWidget from '../widget/index'

import {
    WidgetWrapper,
    Header,
    IconWrapper,
    StyledTitle
  } from './style'
import { StyledTitleTab } from '../widgetTitleTab'
import { BatColorIcon } from 'brave-ui/components/icons'

interface Props {
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
}

class CryptoWallets extends React.PureComponent<Props, {}> {

  componentDidMount () {
    // The origin for the frame should be determined based on
    // whether the wallet is setup or not
    /*
    chrome.braveWallet.shouldPromptForSetup((prompt: boolean) => { })
    */
  }

  renderTitle () {
    return (
      <Header>
        <StyledTitle>
          <IconWrapper>
            <BatColorIcon />
          </IconWrapper>
          <>
            {'Crypto Wallets'}
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
          <div style={{ margin: '20px 0 5px 0' }}>
            <iframe style={{ border: 'none' }} width="284px" height="384px" src="chrome-extension://odbfpeeihdkbihmopkbjmoonfanlbfcl/popup.html"/>
          </div>
      </WidgetWrapper>
    )
  }
}

export const CryptoWalletsWidget = createWidget(CryptoWallets)

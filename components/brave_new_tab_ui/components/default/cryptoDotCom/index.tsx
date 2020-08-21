/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import {
  WidgetWrapper,
  Header,
  StyledTitle,
  GeminiIcon,
  StyledTitleText
} from './style'
// import {
//   SearchIcon,
//   ShowIcon,
//   HideIcon
// } from '../exchangeWidget/shared-assets'
import CryptoDotComLogo from './assets/cryptoDotCom-logo'
// import { CaratLeftIcon, CaratDownIcon } from 'brave-ui/components/icons'

// Utils
// import cryptoColors from '../exchangeWidget/colors'
// import { getLocale } from '../../../../common/locale'

interface State {
}

interface Props {
  showContent: boolean
  tickerPrices: Record<string, string>
  stackPosition: number
  onShowContent: () => void
  onDisableWidget: () => void
  onTotalPriceOptIn: () => void
  onBtcPriceOptIn: () => void
}

class CryptoDotCom extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {}
  }

  componentDidMount () {}

  componentDidUpdate (prevProps: Props) {}

  componentWillUnmount () {}

  renderIndexView () {
    return false
  }

  setSelectedView (view: string) {
  }

  renderTitle () {
    return (
      <Header>
        <StyledTitle>
          <GeminiIcon>
            <CryptoDotComLogo />
          </GeminiIcon>
          <StyledTitleText>
            {'Crypto.com'}
          </StyledTitleText>
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
    const { showContent } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper tabIndex={0}>
        {
          this.renderIndexView()
          ? this.renderIndexView()
          : <>
              {this.renderTitle()}
            </>
        }
      </WidgetWrapper>
    )
  }
}

export const CryptoDotComWidget = createWidget(CryptoDotCom)

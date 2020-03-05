/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledTitleTab,
  WidgetWrapper,
  Content,
  Copy,
  BuyPromptWrapper,
  FiatInputWrapper,
  FiatInputField,
  FiatDropdown,
  CaratDropdown,
  AssetDropdown,
  AssetDropdownLabel,
  ActionsWrapper,
  ConnectButton,
  Header,
  StyledTitle,
  StyledTitleText,
  ExchangeIcon
} from './style'
import ExchangeLogo from './assets/exchange-logo'
import { CaratDownIcon } from 'brave-ui/components/icons'

export interface ExchangeProps {
  showContent: boolean
  onShowContent: () => void
}

export default class Exchange extends React.PureComponent<ExchangeProps, {}> {

  renderTitle () {
    const { showContent } = this.props

    return (
      <Header isInTab={!showContent}>
        <StyledTitle>
          <ExchangeIcon>
            <ExchangeLogo />
          </ExchangeIcon>
          <StyledTitleText>
            {'Exchange'}
          </StyledTitleText>
        </StyledTitle>
      </Header>
    )
  }

  renderTitleTab () {
    const { showContent, onShowContent } = this.props

    return (
      <StyledTitleTab isInTab={!showContent} onClick={onShowContent}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  renderBuyView () {
    return (
      <>
        <Content>
          <Copy>
            {'Buy Crypto'}
          </Copy>
          <BuyPromptWrapper>
            <FiatInputWrapper>
              <FiatInputField
                type={'text'}
                placeholder={'I want to spend...'}
              />
              <FiatDropdown>
                {'USD'}
                <CaratDropdown>
                  <CaratDownIcon />
                </CaratDropdown>
              </FiatDropdown>
            </FiatInputWrapper>
            <AssetDropdown itemsShowing={false}>
              <AssetDropdownLabel>
                {'BTC'}
              </AssetDropdownLabel>
              <CaratDropdown>
                <CaratDownIcon />
              </CaratDropdown>
            </AssetDropdown>
          </BuyPromptWrapper>
          <ActionsWrapper>
            <ConnectButton>
              {'Buy BTC'}
            </ConnectButton>
          </ActionsWrapper>
        </Content>
      </>
    )
  }

  render () {
    const { showContent } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper>
        {this.renderTitle()}
        {this.renderBuyView()}
      </WidgetWrapper>
    )
  }
}

// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import CryptoBanner from './assets/crypto-banner'
import {
  StyledWrapper,
  StyledInner,
  StyledContent,
  StyledHeader,
  StyledSeparator,
  StyledDisclosure,
  StyledText,
  StyledRewards,
  StyledButtonWrapper,
  StyledButton
} from './style'

// Utils
import { getLocale } from '../../../../../../components/common/locale'

interface Props {
  onWalletOptIn: () => void
}

interface State {
  isLoading: boolean
}

export default class OptIn extends React.PureComponent<Props, State> {

  constructor (props: Props) {
    super(props)
    this.state = {
      isLoading: false
    }
  }

  openRewards = () => {
    window.open('chrome://rewards', '_blank')
  }

  onWalletOptIn = () => {
    if (!this.state.isLoading) {
      this.setState({ isLoading: true })
      this.props.onWalletOptIn()
    }
  }

  render () {
    return (
      <StyledWrapper>
        <StyledInner>
          <CryptoBanner />
          <StyledContent>
            <StyledHeader>
              {getLocale('cryptoWalletsWelcome')}
            </StyledHeader>
            <StyledSeparator />
            <StyledDisclosure>
              <StyledText>
                {getLocale('cryptoWalletsDisclosureOne')}
              </StyledText>
              <StyledText>
                {getLocale('cryptoWalletsDisclosureTwo')}
              </StyledText>
              <StyledText>
                {getLocale('cryptoWalletsDisclosureThree')} <StyledRewards onClick={this.openRewards}>{getLocale('cryptoWalletsBraveRewards')}</StyledRewards>. {getLocale('cryptoWalletsDisclosureFour')}
              </StyledText>
            </StyledDisclosure>
          </StyledContent>
          <StyledButtonWrapper>
            <StyledButton onClick={this.onWalletOptIn}>
              {
                this.state.isLoading
                ? getLocale('cryptoWalletsDownloading')
                : getLocale('cryptoWalletsDisclosureConfirm')
              }
            </StyledButton>
          </StyledButtonWrapper>
        </StyledInner>
      </StyledWrapper>
    )
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import GrantMobile from './grantMobile'
import AdsBoxMobile from './adsBoxMobile'
import ContributeBoxMobile from './contributeBoxMobile'
import DonationsBoxMobile from './donationsBoxMobile'
import { StyledDisabledContent, StyledHeading, StyledText } from './style'
import {
  MainToggleMobile,
  SettingsPageMobile,
  WalletInfoHeader
} from '../../components/mobile'

// Utils
import locale from './fakeLocale'

interface State {
  mainToggle: boolean
  walletShown: boolean
}

class SettingsMobile extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      mainToggle: true,
      walletShown: false
    }
  }

  onMainToggle = () => {
    this.setState({ mainToggle: !this.state.mainToggle })
  }

  onToggleWallet = () => {
    this.setState({ walletShown: !this.state.walletShown })
  }

  render () {
    return (
      <SettingsPageMobile>
        <MainToggleMobile
          onToggle={this.onMainToggle}
          enabled={this.state.mainToggle}
        />
        {
          !this.state.mainToggle
          ? <StyledDisabledContent>
              <StyledHeading>
                {locale.whyBraveRewards}
              </StyledHeading>
              <StyledText>
                {locale.rewardsDisabledDescOne}
              </StyledText>
              <StyledText>
                {locale.rewardsDisabledDescTwo}
              </StyledText>
            </StyledDisabledContent>
          : null
        }
        <GrantMobile type={'ugp'}/>
        <GrantMobile type={'ads'} amount={'30.0'}/>
        <WalletInfoHeader
          balance={'30.0'}
          id={'mobile-wallet'}
          onClick={this.onToggleWallet}
          converted={'7.00 USD'}
        />
        <AdsBoxMobile
          rewardsEnabled={this.state.mainToggle}
        />
        <ContributeBoxMobile
          rewardsEnabled={this.state.mainToggle}
        />
        <DonationsBoxMobile
          rewardsEnabled={this.state.mainToggle}
        />
      </SettingsPageMobile>
    )
  }
}

export default SettingsMobile

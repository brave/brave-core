/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import AdsBoxMobile from './adsBoxMobile'
import ContributeBoxMobile from './contributeBoxMobile'
import DonationsBoxMobile from './donationsBoxMobile'
import { StyledDisabledContent, StyledHeading, StyledText } from './style'
import {
  MainToggleMobile,
  SettingsPageMobile,
  WalletInfoHeader
} from '../../../../src/features/rewards/mobile'

// Assets
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

// Utils
import locale from './fakeLocale'

interface State {
  mainToggle: boolean
}

class SettingsMobile extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = {
      mainToggle: true
    }
  }

  onMainToggle = () => {
    this.setState({ mainToggle: !this.state.mainToggle })
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
        <WalletInfoHeader
          balance={'30.0'}
          id={'mobile-wallet'}
          converted={'163230.50 USD'}
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

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import DonationsBox from './donationsBox'
import Grant from './grant'
import PageWallet, { Props as WalletProps } from './pageWallet'
import { Grid, Column } from 'brave-ui/components'
import { DisabledBox, GrantTransitionBanner, MainToggle, SettingsPage } from '../../components'

export interface Props {
  walletProps: WalletProps,
  showBanner: boolean
}

interface State {
  mainToggle: boolean
}

class Settings extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      mainToggle: true
    }
  }

  doNothing = () => {
    console.log('nothing')
  }

  onMainToggle = () => {
    this.setState({ mainToggle: !this.state.mainToggle })
  }

  render () {
    const { walletProps, showBanner } = this.props
    return (
      <SettingsPage>
        {
          showBanner
          ? <GrantTransitionBanner
            onAction={this.doNothing}
            amount={'10'}
          />
          : null
        }
        <Grid columns={3} customStyle={{ gridGap: '24px' }}>
          <Column size={2} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            <MainToggle
              onTOSClick={this.doNothing}
              onPrivacyClick={this.doNothing}
              onToggle={this.onMainToggle}
              enabled={this.state.mainToggle}
            />
            {
              !this.state.mainToggle
                ? <DisabledBox />
                : null
            }
            <AdsBox />
            <ContributeBox />
            <DonationsBox />
          </Column>
          <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            <Grant type={'ugp'} />
            <Grant type={'ads'} amount={'30.0'} />
            <PageWallet {...walletProps} />
          </Column>
        </Grid>
      </SettingsPage>
    )
  }
}

export default Settings

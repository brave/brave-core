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
import { MainToggle, SettingsPage } from '../../components'

export interface Props {
  walletProps: WalletProps
}

class Settings extends React.PureComponent<Props> {
  doNothing = () => {
    console.log('nothing')
  }

  render () {
    const { walletProps } = this.props
    return (
      <SettingsPage>
        <Grid columns={3} customStyle={{ gridGap: '24px' }}>
          <Column size={2} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            <MainToggle
              onTOSClick={this.doNothing}
              onPrivacyClick={this.doNothing}
            />
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

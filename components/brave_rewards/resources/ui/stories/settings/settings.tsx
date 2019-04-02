/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import DonationsBox from './donationsBox'
import Grant from './grant'
import PageWallet from './pageWallet'
import { Grid, Column } from '../../../../src/components'
import { DisabledBox, MainToggle, SettingsPage } from '../../../../src/features/rewards'

// Assets
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'

interface State {
  mainToggle: boolean
}

class Settings extends React.PureComponent<{}, State> {
  constructor (props: {}) {
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
    return (
      <SettingsPage>
        <Grid columns={3} customStyle={{ gridGap: '32px' }}>
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
            <AdsBox/>
            <ContributeBox/>
            <DonationsBox/>
          </Column>
          <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            <Grant type={'ugp'}/>
            <Grant type={'ads'} amount={'30.0'}/>
            <PageWallet/>
          </Column>
        </Grid>
      </SettingsPage>
    )
  }
}

export default Settings

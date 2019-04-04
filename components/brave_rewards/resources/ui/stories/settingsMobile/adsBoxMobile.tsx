/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { StyledListContent } from './style'
import { Select } from '../../../../src/components'
import { Tokens, List } from '../../../../src/features/rewards'
import { BoxMobile } from '../../../../src/features/rewards/mobile'

// Utils
import locale from './fakeLocale'

interface Props {
  rewardsEnabled: boolean
}

interface State {
  adsToggle: boolean
}

class AdsBoxMobile extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      adsToggle: true
    }
  }

  onAdsToggle = () => {
    this.setState({
      adsToggle: !this.state.adsToggle
    })
  }

  render () {
    const checked = this.props.rewardsEnabled && this.state.adsToggle

    return (
      <BoxMobile
        title={locale.adsTitle}
        type={'ads'}
        description={locale.adsDesc}
        toggle={true}
        checked={checked}
        toggleAction={this.onAdsToggle}
        settingsChild={<div>Ads Settings content</div>}
      >
        <List title={<StyledListContent>{locale.adsEarnings}</StyledListContent>}>
          <StyledListContent>
            <Tokens value={'10.0'} converted={'4.00'} />
          </StyledListContent>
        </List>
        <List title={<StyledListContent>{locale.adsDisplayed}</StyledListContent>}>
          <StyledListContent>
            <Select
              title={locale.adsDisplayed}
            >
              <div data-value='0'>0</div>
              <div data-value='2'>2</div>
              <div data-value='4'>4</div>
              <div data-value='6'>6</div>
            </Select>
          </StyledListContent>
        </List>
      </BoxMobile>
    )
  }
}

export default AdsBoxMobile

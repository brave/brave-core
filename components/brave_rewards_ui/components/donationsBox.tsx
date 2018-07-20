/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  Box,
  Column,
  DonationTable,
  Grid,
  List,
  Tokens
} from 'brave-ui'
import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Utils
import { getLocale } from '../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import Checkbox from 'brave-ui/rewards/checkbox'
import DisabledContent from 'brave-ui/rewards/disabledContent'

// Assets
const donate = require('../../img/rewards/donate_disabled.svg')

// TODO temp, remove
const bartBaker = require('../../img/rewards/temp/bartBaker.jpeg')

interface Props extends Rewards.ComponentProps {
}

class DonationBox extends React.Component<Props, {}> {
  // TODO remove
  get donationRows () {
    return [
      {
        profile: {
          name: 'Bart Baker',
          verified: true,
          provider: 'youtube',
          src: bartBaker
        },
        type: 'recurring',
        contribute: {
          tokens: 2,
          converted: 0.2
        },
        onRemove: () => { console.log('Bar') }
      },
      {
        profile: {
          verified: false,
          name: 'theguardian.com',
          src: bartBaker
        },
        type: 'donation',
        contribute: {
          tokens: 12,
          converted: 6.2
        },
        text: 'May 7'
      },
      {
        profile: {
          verified: false,
          name: 'BrendanEich',
          provider: 'twitter',
          src: bartBaker
        },
        type: 'tip',
        contribute: {
          tokens: 7,
          converted: 3.2
        },
        text: 'May 2'
      }
    ] as any
  }

  get actions () {
    return this.props.actions
  }

  onCheckSettingChange = (key: string, selected: boolean) => {
    this.actions.onSettingSave(key, selected)
  }

  donationSettings = () => {
    return (
      <Grid columns={1} theme={{ maxWidth: '270px', margin: '0 auto' }}>
        <Column size={1} theme={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <Checkbox
            title={getLocale('donationAbility')}
            value={{
              donationAbilityYT: this.props.rewardsData.donationAbilityYT,
              donationAbilityTwitter: this.props.rewardsData.donationAbilityTwitter
            }}
            multiple={true}
            onChange={this.onCheckSettingChange}
          >
            <div data-key='donationAbilityYT'>{getLocale('donationAbilityYT')}</div>
            <div data-key='donationAbilityTwitter'>{getLocale('donationAbilityTwitter')}</div>
          </Checkbox>
        </Column>
      </Grid>
    )
  }

  disabledContent = () => {
    return (
      <DisabledContent
        image={donate}
        theme={{ color: '#AC9CCF', boldColor: '#696fdc' }}
      >
        • {getLocale('donationDisabledText1')}<br/>
        • {getLocale('donationDisabledText2')}
      </DisabledContent>
    )
  }

  render () {
    const { rewardsData } = this.props
    const showDisabled = rewardsData.firstLoad !== false || !rewardsData.enabledMain

    return (
      <Box
        title={getLocale('donationTitle')}
        theme={{ titleColor: '#696FDC' }}
        description={getLocale('donationDesc')}
        disabledContent={showDisabled ? this.disabledContent() : null}
        settingsChild={this.donationSettings()}
      >
        <List title={getLocale('donationTotalDonations')}>
          <Tokens value={21} converted={7} />
        </List>
        <List title={getLocale('donationList')}>
          {getLocale('donationTotal')} &nbsp;<Tokens value={3} hideText={true} />
        </List>
        <DonationTable
          rows={this.donationRows}
          allItems={true}
          theme={{
            headerColor: '#696FDC'
          }}
        >
          {getLocale('donationVisitSome')}
        </DonationTable>
      </Box>
    )
  }
}

const mapStateToProps = (state: Rewards.ApplicationState) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(DonationBox)

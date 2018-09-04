/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Checkbox, Column, Grid, ControlWrapper } from 'brave-ui/components'
import { DisabledContent, Box, TableDonation, List, Tokens } from 'brave-ui/features/rewards'

// Utils
import { getLocale } from '../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

// Assets
const donate = require('../../../img/rewards/donate_disabled.svg')

interface Props extends Rewards.ComponentProps {
}

class DonationBox extends React.Component<Props, {}> {
  get actions () {
    return this.props.actions
  }

  onCheckSettingChange = (key: string, selected: boolean) => {
    this.actions.onSettingSave(key, selected)
  }

  donationSettings = () => {
    const {
      donationAbilityYT,
      donationAbilityTwitter,
      enabledMain
    } = this.props.rewardsData

    if (!enabledMain) {
      return null
    }

    return (
      <Grid columns={1} customStyle={{ maxWidth: '270px', margin: '0 auto' }}>
        <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <ControlWrapper text={getLocale('donationAbility')}>
            <Checkbox
              value={{
                donationAbilityYT,
                donationAbilityTwitter
              }}
              multiple={true}
              onChange={this.onCheckSettingChange}
            >
              <div data-key='donationAbilityYT'>{getLocale('donationAbilityYT')}</div>
              <div data-key='donationAbilityTwitter'>{getLocale('donationAbilityTwitter')}</div>
            </Checkbox>
          </ControlWrapper>
        </Column>
      </Grid>
    )
  }

  disabledContent = () => {
    return (
      <DisabledContent
        image={donate}
        type={'donation'}
      >
        • {getLocale('donationDisabledText1')}<br/>
        • {getLocale('donationDisabledText2')}
      </DisabledContent>
    )
  }

  getDonationRows = () => []

  render () {
    const { rewardsData } = this.props
    const showDisabled = rewardsData.firstLoad !== false || !rewardsData.enabledMain
    const donationRows = this.getDonationRows()
    const numRows = donationRows.length
    const allSites = !(numRows > 5)

    return (
      <Box
        title={getLocale('donationTitle')}
        type={'donation'}
        description={getLocale('donationDesc')}
        settingsChild={this.donationSettings()}
        disabledContent={showDisabled ? this.disabledContent() : null}
      >
        <List title={getLocale('donationTotalDonations')}>
          <Tokens value={0} converted={0} />
        </List>
        <List title={getLocale('donationList')}>
          {getLocale('total')} &nbsp;<Tokens value={numRows} hideText={true} toFixed={false} />
        </List>
        <TableDonation
          rows={donationRows}
          allItems={allSites}
          numItems={numRows}
          headerColor={true}
        >
          {getLocale('donationVisitSome')}
        </TableDonation>
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

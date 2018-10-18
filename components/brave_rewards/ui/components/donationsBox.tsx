/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Checkbox, Column, Grid, ControlWrapper } from 'brave-ui/components'
import { DisabledContent, Box, TableDonation, List, Tokens, ModalDonation } from 'brave-ui/features/rewards'
import { Provider } from 'brave-ui/features/rewards/profile'

// Utils
import { getLocale } from '../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import { DetailRow } from 'brave-ui/features/rewards/tableDonation'

// Assets
const donate = require('../../../img/rewards/donate_disabled.svg')

interface Props extends Rewards.ComponentProps {
}

interface State {
  modalShowAll: boolean
}

class DonationBox extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalShowAll: false
    }
  }

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

  getTotal = () => {
    const { reports } = this.props.rewardsData

    const currentTime = new Date()
    const reportKey = `${currentTime.getFullYear()}_${currentTime.getMonth() + 1}`
    const report: Rewards.Report = reports[reportKey]

    if (report) {
      return utils.donationTotal(report)
    }

    return '0.0'
  }

  getDonationRows = () => {
    const { walletInfo, recurringList, tipsList } = this.props.rewardsData

    // Recurring
    let recurring: DetailRow[] = []
    if (recurringList) {
      recurring = recurringList.map((item: Rewards.Publisher) => {
        let name = item.name
        if (item.provider) {
          name = `${name} ${getLocale('on')} ${item.provider}`
        }

        let faviconUrl = `chrome://favicon/size/48@1x/${item.url}`
        if (item.favIcon) {
          faviconUrl = `chrome://favicon/size/48@1x/${item.favIcon}`
        }

        return {
          profile: {
            name,
            verified: item.verified,
            provider: (item.provider ? item.provider : undefined) as Provider,
            src: faviconUrl
          },
          contribute: {
            tokens: item.percentage.toFixed(1),
            converted: utils.convertBalance(item.percentage.toString(), walletInfo.rates)
          },
          url: item.url,
          type: 'recurring' as any,
          onRemove: () => { this.actions.removeRecurring(item.id) }
        }
      })
    }

    // Tips
    let tips: DetailRow[] = []
    if (tipsList) {
      tips = tipsList.map((item: Rewards.Publisher) => {
        let name = item.name
        if (item.provider) {
          name = `${name} ${getLocale('on')} ${item.provider}`
        }

        let faviconUrl = `chrome://favicon/size/48@1x/${item.url}`
        if (item.favIcon) {
          faviconUrl = `chrome://favicon/size/48@1x/${item.favIcon}`
        }

        const token = utils.convertProbiToFixed(item.percentage.toString())

        return {
          profile: {
            name,
            verified: item.verified,
            provider: (item.provider ? item.provider : undefined) as Provider,
            src: faviconUrl
          },
          contribute: {
            tokens: token,
            converted: utils.convertBalance(token, walletInfo.rates)
          },
          url: item.url,
          text: item.tipDate ? new Date(item.tipDate * 1000).toLocaleDateString() : undefined,
          type: 'donation' as any,
          onRemove: () => { this.actions.removeRecurring(item.id) }
        }
      })
    }

    return recurring.concat(tips)
  }

  onModalToggle = () => {
    this.setState({
      modalShowAll: !this.state.modalShowAll
    })
  }

  render () {
    const { walletInfo, firstLoad, enabledMain } = this.props.rewardsData
    const showDisabled = firstLoad !== false || !enabledMain
    const donationRows = this.getDonationRows()
    const topRows = donationRows.slice(0, 5)
    const numRows = donationRows && donationRows.length
    const allSites = !(numRows > 5)
    const total = this.getTotal()
    const converted = utils.convertBalance(total, walletInfo.rates)

    return (
      <Box
        title={getLocale('donationTitle')}
        type={'donation'}
        description={getLocale('donationDesc')}
        settingsChild={this.donationSettings()}
        disabledContent={showDisabled ? this.disabledContent() : null}
      >
        {
          this.state.modalShowAll
          ? <ModalDonation
            rows={donationRows}
            onClose={this.onModalToggle}
          />
          : null
        }
        <List title={getLocale('donationTotalDonations')}>
          <Tokens value={total} converted={converted} />
        </List>
        <TableDonation
          rows={topRows}
          allItems={allSites}
          numItems={numRows}
          headerColor={true}
          onShowAll={this.onModalToggle}
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

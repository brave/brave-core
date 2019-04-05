/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  DisabledContent,
  Box,
  BoxAlert,
  TableDonation,
  List,
  Tokens,
  ModalDonation,
  NextContribution
} from 'brave-ui/features/rewards'
import { Provider } from 'brave-ui/features/rewards/profile'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import { DetailRow } from 'brave-ui/features/rewards/tableDonation'

interface Props extends Rewards.ComponentProps {
}

interface State {
  modalShowAll: boolean
  settings: boolean
}

class DonationBox extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalShowAll: false,
      settings: false
    }
  }

  get actions () {
    return this.props.actions
  }

  disabledContent = () => {
    return (
      <DisabledContent
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
        let faviconUrl = `chrome://favicon/size/48@1x/${item.url}`
        if (item.favIcon && item.verified) {
          faviconUrl = `chrome://favicon/size/48@1x/${item.favIcon}`
        }

        return {
          profile: {
            name: item.name,
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
          onRemove: () => { this.actions.removeRecurringTip(item.id) }
        }
      })
    }

    // Tips
    let tips: DetailRow[] = []
    if (tipsList) {
      tips = tipsList.map((item: Rewards.Publisher) => {
        let faviconUrl = `chrome://favicon/size/48@1x/${item.url}`
        if (item.favIcon && item.verified) {
          faviconUrl = `chrome://favicon/size/48@1x/${item.favIcon}`
        }

        const token = utils.convertProbiToFixed(item.percentage.toString())

        return {
          profile: {
            name: item.name,
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
          onRemove: () => { this.actions.removeRecurringTip(item.id) }
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

  doNothing = () => {
    console.log('Alert closed')
  }

  importAlert = (walletImported: boolean) => {
    return (
      walletImported
      ? <BoxAlert type={'tips'} onReview={this.doNothing} />
      : null
    )
  }

  onSettingsToggle = () => {
    this.setState({ settings: !this.state.settings })
  }

  render () {
    const {
      walletInfo,
      firstLoad,
      enabledMain,
      ui,
      recurringList,
      reconcileStamp
    } = this.props.rewardsData
    const { walletImported } = ui
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
        disabledContent={showDisabled ? this.disabledContent() : null}
        attachedAlert={this.importAlert(walletImported)}
        settingsOpened={this.state.settings}
        onSettingsClick={this.onSettingsToggle}
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
        {
          recurringList && recurringList.length > 0
          ? <List title={getLocale('donationNextDate')}>
            <NextContribution>
              {new Date(reconcileStamp * 1000).toLocaleDateString()}
            </NextContribution>
          </List>
          : null
        }

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

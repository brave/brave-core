/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Checkbox, Grid, Column, ControlWrapper } from 'brave-ui/components'
import {
  DisabledContent,
  Box,
  TableDonation,
  List,
  Tokens,
  ModalDonation
} from '../../ui/components'
import { Provider } from '../../ui/components/profile'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'
import { DetailRow } from '../../ui/components/tableDonation'

interface Props extends Rewards.ComponentProps {
}

interface State {
  modalShowAll: boolean
  settings: boolean
}

class TipBox extends React.Component<Props, State> {
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
        {getLocale('donationDisabledText1')}<br/>
        {getLocale('donationDisabledText2')}
      </DisabledContent>
    )
  }

  getTipsRows = () => {
    const { parameters, tipsList } = this.props.rewardsData
    let tips: DetailRow[] = []

    if (!tipsList) {
      return tips
    }

    return tipsList.map((item: Rewards.Publisher) => {
      let faviconUrl = `chrome://favicon/size/64@1x/${item.url}`
      const verified = utils.isPublisherConnectedOrVerified(item.status)
      if (item.favIcon && verified) {
        faviconUrl = `chrome://favicon/size/64@1x/${item.favIcon}`
      }

      return {
        profile: {
          name: item.name,
          verified,
          provider: (item.provider ? item.provider : undefined) as Provider,
          src: faviconUrl
        },
        contribute: {
          tokens: item.percentage.toFixed(3),
          converted: utils.convertBalance(item.percentage, parameters.rate)
        },
        url: item.url,
        text: item.tipDate ? new Date(item.tipDate * 1000).toLocaleDateString() : undefined,
        type: 'donation' as any,
        onRemove: () => { this.actions.removeRecurringTip(item.id) }
      }
    })
  }

  onModalToggle = () => {
    this.setState({
      modalShowAll: !this.state.modalShowAll
    })
  }

  doNothing = () => {
    console.log('Alert closed')
  }

  onSettingsToggle = () => {
    this.setState({ settings: !this.state.settings })
  }

  onInlineTipSettingChange = (key: string, selected: boolean) => {
    this.actions.onInlineTipSettingChange(key, selected)
  }

  donationSettingsChild = () => {
    let value = this.props.rewardsData.inlineTip

    if (!value) {
      value = {
        twitter: true,
        reddit: true,
        github: true
      }
    }

    return (
      <>
        <Grid columns={1}>
          <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            <ControlWrapper text={getLocale('donationAbility')}>
              <Checkbox
                value={value}
                multiple={true}
                onChange={this.onInlineTipSettingChange}
              >
                <div data-key='reddit'>{getLocale('donationAbilityReddit')}</div>
              </Checkbox>
              <Checkbox
                value={value}
                multiple={true}
                onChange={this.onInlineTipSettingChange}
              >
                <div data-key='twitter'>{getLocale('donationAbilityTwitter')}</div>
              </Checkbox>

              <Checkbox
                value={value}
                multiple={true}
                onChange={this.onInlineTipSettingChange}
              >
                <div data-key='github'>{getLocale('donationAbilityGitHub')}</div>
              </Checkbox>
            </ControlWrapper>
          </Column>
        </Grid>
      </>
    )
  }

  render () {
    const {
      parameters,
      firstLoad,
      tipsList
    } = this.props.rewardsData
    const showDisabled = firstLoad !== false
    const tipRows = this.getTipsRows()
    const topRows = tipRows.slice(0, 5)
    const numRows = tipRows && tipRows.length
    const allSites = !(numRows > 5)
    const total = utils.tipsListTotal(tipsList)
    const converted = utils.convertBalance(total, parameters.rate)

    return (
      <Box
        title={getLocale('donationTitle')}
        type={'donation'}
        description={getLocale('donationDesc')}
        disabledContent={showDisabled ? this.disabledContent() : null}
        settingsChild={this.donationSettingsChild()}
        settingsOpened={this.state.settings}
        onSettingsClick={this.onSettingsToggle}
      >
        {
          this.state.modalShowAll
          ? <ModalDonation
            rows={tipRows}
            onClose={this.onModalToggle}
            title={getLocale('donationTips')}
          />
          : null
        }
        <List title={getLocale('donationTotalDonations')}>
          <Tokens id={'tip-box-total'} value={total.toFixed(3)} converted={converted} />
        </List>
        <TableDonation
          id={'tips-table'}
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
)(TipBox)

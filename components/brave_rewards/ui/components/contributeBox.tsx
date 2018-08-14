/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Checkbox, Grid, Column, Select } from 'brave-ui/components'
import { Box, TableContribute, DisabledContent, List, ModalContribute, Tokens, NextContribution } from 'brave-ui/features/rewards'

// Utils
import { getLocale } from '../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'

// Assets
const contributeDisabledIcon = require('../../../img/rewards/contribute_disabled.svg')
const bartBaker = require('../../../img/rewards/temp/bartBaker.jpeg') // TODO temp, remove

interface State {
  modalContribute: boolean
}

interface MonthlyChoice {
  tokens: number
  converted: number
}

interface Props extends Rewards.ComponentProps {
}

class ContributeBox extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalContribute: false
    }
  }

  // TODO remove
  getContributeRows = () => {
    return [
      {
        profile: {
          name: 'Bart Baker',
          verified: true,
          provider: 'youtube',
          src: bartBaker
        },
        attention: 40,
        onRemove: () => { console.log('Bar') }
      },
      {
        profile: {
          name: 'duckduckgo.com',
          verified: true,
          src: bartBaker
        },
        attention: 20,
        onRemove: () => { console.log('duckduckgo') }
      },
      {
        profile: {
          name: 'buzzfeed.com',
          verified: false,
          src: bartBaker
        },
        attention: 10,
        onRemove: () => { console.log('buzzfeed') }
      },
      {
        profile: {
          name: 'theguardian.com',
          verified: true,
          src: bartBaker
        },
        attention: 5,
        onRemove: () => { console.log('theguardian') }
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: bartBaker
        },
        attention: 4,
        onRemove: () => { console.log('wikipedia') }
      },
      {
        profile: {
          name: 'wikipedia.org',
          verified: false,
          src: bartBaker
        },
        attention: 1,
        onRemove: () => { console.log('wikipedia') }
      }
    ] as any
  }

  get actions () {
    return this.props.actions
  }

  contributeDisabled () {
    return (
      <DisabledContent
        image={contributeDisabledIcon}
        type={'contribute'}
      >
        • {getLocale('contributionDisabledText1')} <br />
        • {getLocale('contributionDisabledText2')}}
      </DisabledContent>
    )
  }

  onToggleContribution = () => {
    this.actions.onSettingSave('enabledContribute', !this.props.rewardsData.enabledContribute)
  }

  onModalContributeToggle = () => {
    this.setState({
      modalContribute: !this.state.modalContribute
    })
  }

  onSelectSettingChange = (key: string, value: string) => {
    this.actions.onSettingSave(key, +value)
  }

  onCheckSettingChange = (key: string, selected: boolean) => {
    this.actions.onSettingSave(key, selected)
  }

  contributeSettings = (monthlyList: MonthlyChoice[]) => {
    const {
      contributionMinTime,
      contributionMinVisits,
      contributionNonVerified,
      contributionVideos,
      contributionMonthly,
      enabledMain
    } = this.props.rewardsData

    if (!enabledMain) {
      return null
    }

    return (
      <Grid columns={1} theme={{ maxWidth: '270px', margin: '0 auto' }}>
        <Column size={1} theme={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <Select
            title={getLocale('contributionMonthly')}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
            value={(contributionMonthly || '').toString()}
          >
            {
              monthlyList.map((choice: MonthlyChoice) => {
                return <div key={`choice-${choice.tokens}`} data-value={choice.tokens.toString()}>
                  <Tokens value={choice.tokens} converted={choice.converted} />
                </div>
              })
            }
          </Select>
          <Select
            title={getLocale('contributionMinTime')}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMinTime')}
            value={(contributionMinTime || '').toString()}
          >
            <div data-value='5000'>{getLocale('contributionTime5')}</div>
            <div data-value='8000'>{getLocale('contributionTime8')}</div>
            <div data-value='60000'>{getLocale('contributionTime60')}</div>
          </Select>
          <Select
            title={getLocale('contributionMinVisits')}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMinVisits')}
            value={(contributionMinVisits || '').toString()}
          >
            <div data-value='1'>{getLocale('contributionVisit1')}</div>
            <div data-value='5'>{getLocale('contributionVisit5')}</div>
            <div data-value='10'>{getLocale('contributionVisit10')}</div>
          </Select>
          <Checkbox
            title={getLocale('contributionAllowed')}
            value={{
              contributionNonVerified: contributionNonVerified,
              contributionVideos: contributionVideos
            }}
            multiple={true}
            onChange={this.onCheckSettingChange}
          >
            <div data-key='contributionNonVerified'>{getLocale('contributionNonVerified')}</div>
            <div data-key='contributionVideos'>{getLocale('contributionVideos')}</div>
          </Checkbox>
        </Column>
      </Grid>
    )
  }

  render () {
    const {
      firstLoad,
      enabledMain,
      walletInfo,
      contributionMonthly,
      enabledContribute
    } = this.props.rewardsData
    const toggleOn = !(firstLoad !== false || !enabledMain)
    const monthlyList: MonthlyChoice[] = utils.generateContributionMonthly(walletInfo.choices, walletInfo.rates)
    const contributeRows = this.getContributeRows()
    const numRows = contributeRows.length
    const allSites = !(numRows > 5)

    return (
      <Box
        title={getLocale('contributionTitle')}
        type={'contribute'}
        description={getLocale('contributionDesc')}
        toggle={toggleOn}
        checked={toggleOn ? enabledContribute : false}
        disabledContent={this.contributeDisabled()}
        onToggle={this.onToggleContribution}
        settingsChild={this.contributeSettings(monthlyList)}
      >
        {
          this.state.modalContribute
          ? <ModalContribute
            rows={contributeRows}
            onClose={this.onModalContributeToggle}
          />
          : null
        }
        <List title={getLocale('contributionMonthly')}>
          <Select
            theme={{
              border: 'none',
              padding: '0',
              arrowPadding: '0',
              maxWidth: '100%'
            }}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
            value={(contributionMonthly || '').toString()}
          >
            {
              monthlyList.map((choice: MonthlyChoice) => {
                return <div key={`choice-${choice.tokens}`} data-value={choice.tokens.toString()}>
                  <Tokens value={choice.tokens} converted={choice.converted} />
                </div>
              })
            }
          </Select>
        </List>
        <List
          title={getLocale('contributionNextDate')}
        >
          <NextContribution>July 25th TODO</NextContribution>
        </List>
        <List title={getLocale('contributionSites')}>
          {getLocale('total')} &nbsp;<Tokens value={numRows} hideText={true} toFixed={false} />
        </List>
        <TableContribute
          header={[
            getLocale('contributionSiteVisited'),
            getLocale('contributionSiteAttention')
          ]}
          rows={contributeRows}
          allSites={allSites}
          numSites={numRows}
          onShowAll={this.onModalContributeToggle}
          headerColor={true}
        >
          {getLocale('contributionVisitSome')}
        </TableContribute>
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
)(ContributeBox)

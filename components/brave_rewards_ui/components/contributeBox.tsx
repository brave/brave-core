/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { Checkbox, Grid, Column, Select } from 'brave-ui/components'
import { Box, ContributeTable, DisabledContent, List, ModalContribute, Tokens } from 'brave-ui/features/rewards'

// Utils
import { getLocale } from '../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

// Assets
const contributeDisabledIcon = require('../../img/rewards/contribute_disabled.svg')

// TODO temp, remove
const bartBaker = require('../../img/rewards/temp/bartBaker.jpeg')

interface State {
  modalContribute: boolean
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
  get contributeRows () {
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
        theme={{ color: '#ce9ccf', boldColor: '#c16fc2' }}
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

  contributeSettings = () => {
    const data = this.props.rewardsData

    return (
      <Grid columns={1} theme={{ maxWidth: '270px', margin: '0 auto' }}>
        <Column size={1} theme={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <Select
            title={getLocale('contributionMonthly')}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
            value={(data.contributionMonthly || '').toString()}
          >
            <div data-value='10'><Tokens value={10} converted={'4'} /></div>
            <div data-value='20'><Tokens value={20} converted={'6'} /></div>
            <div data-value='40'><Tokens value={40} converted={'12'} /></div>
            <div data-value='100'><Tokens value={100} converted={'40'} /></div>
          </Select>
          <Select
            title={getLocale('contributionMinTime')}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMinTime')}
            value={(data.contributionMinTime || '').toString()}
          >
            <div data-value='5000'>{getLocale('contributionTime5')}</div>
            <div data-value='8000'>{getLocale('contributionTime8')}</div>
            <div data-value='60000'>{getLocale('contributionTime60')}</div>
          </Select>
          <Select
            title={getLocale('contributionMinVisits')}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMinVisits')}
            value={(data.contributionMinVisits || '').toString()}
          >
            <div data-value='1'>{getLocale('contributionVisit1')}</div>
            <div data-value='5'>{getLocale('contributionVisit5')}</div>
            <div data-value='10'>{getLocale('contributionVisit10')}</div>
          </Select>
          <Checkbox
            title={getLocale('contributionAllowed')}
            value={{
              contributionNonVerified: data.contributionNonVerified,
              contributionVideos: data.contributionVideos
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
    const { rewardsData } = this.props
    const toggleOn = !(rewardsData.firstLoad !== false || !rewardsData.enabledMain)

    return (
      <Box
        title={getLocale('contributionTitle')}
        theme={{ titleColor: '#9F22A1' }}
        description={getLocale('contributionDesc')}
        toggle={toggleOn}
        checked={toggleOn ? rewardsData.enabledContribute : false}
        disabledContent={this.contributeDisabled()}
        onToggle={this.onToggleContribution}
        settingsChild={this.contributeSettings()}
      >
        {
          this.state.modalContribute
          ? <ModalContribute
            rows={this.contributeRows}
            onClose={this.onModalContributeToggle}
          />
          : null
        }
        <List title={getLocale('contributionMonthly')}>
          <Select
            theme={{
              border: 'none',
              padding: '0 20px 0 0',
              arrowPadding: '0'
            }}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
            value={(rewardsData.contributionMonthly || '').toString()}
          >
            <div data-value='10'><Tokens value={10} converted={'4'} /></div>
            <div data-value='20'><Tokens value={20} converted={'6'} /></div>
            <div data-value='40'><Tokens value={40} converted={'12'} /></div>
            <div data-value='100'><Tokens value={100} converted={'40'} /></div>
          </Select>
        </List>
        <List
          title={getLocale('contributionNextDate')}
          theme={{
            'font-size': '16px',
            'font-weight': '600',
            'text-align': 'right',
            'border-radius': '6px',
            color: '#4b4c5c',
            background: '#e9f0ff',
            display: 'inline-block',
            padding: '9px 10px 9px 13px'
          }}
        >
          July 25th
        </List>
        <List title={getLocale('contributionSites')}>
          Total &nbsp;<Tokens value={55} hideText={true} />
        </List>
        <ContributeTable
          header={[
            getLocale('contributionSiteVisited'),
            getLocale('contributionSiteAttention')
          ]}
          rows={this.contributeRows}
          allSites={false}
          numSites={55}
          onShowAll={this.onModalContributeToggle}
          theme={{
            headerColor: '#9F22A1'
          }}
        >
          {getLocale('contributionVisitSome')}
        </ContributeTable>
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

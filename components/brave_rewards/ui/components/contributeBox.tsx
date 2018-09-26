/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Checkbox, Grid, Column, Select, ControlWrapper } from 'brave-ui/components'
import { Box, TableContribute, DisabledContent, List, ModalContribute, Tokens, NextContribution } from 'brave-ui/features/rewards'
import { Provider } from 'brave-ui/features/rewards/profile'

// Utils
import { getLocale } from '../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'

// Assets
const contributeDisabledIcon = require('../../../img/rewards/contribute_disabled.svg')

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

  getIncludedPublishers = (list: Rewards.Publisher[]) => {
    if (!list) {
      return []
    }

    return list.filter((item: Rewards.Publisher) => item.excluded !== 1)
  }

  getContributeRows = (list: Rewards.Publisher[]) => {
    return list.map((item: Rewards.Publisher) => {
      let name = item.name
      if (item.provider) {
        name = `${name} ${getLocale('on')} ${item.provider}`
      }

      return {
        profile: {
          name,
          verified: item.verified,
          provider: (item.provider ? item.provider : undefined) as Provider,
          src: `chrome://favicon/size/48@1x/${item.url}/`
        },
        url: item.url,
        attention: item.percentage,
        onRemove: () => { this.actions.excludePublisher(item.id) }
      }
    })
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
        • {getLocale('contributionDisabledText2')}
      </DisabledContent>
    )
  }

  onRestore = () => {
    this.actions.restorePublishers()
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
      <Grid columns={1} customStyle={{ maxWidth: '270px', margin: '0 auto' }}>
        <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <ControlWrapper text={getLocale('contributionMonthly')}>
            <Select
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
          </ControlWrapper>
          <ControlWrapper text={getLocale('contributionMinTime')}>
            <Select
              onChange={this.onSelectSettingChange.bind(this, 'contributionMinTime')}
              value={(contributionMinTime || '').toString()}
            >
              <div data-value='5'>{getLocale('contributionTime5')}</div>
              <div data-value='8'>{getLocale('contributionTime8')}</div>
              <div data-value='60'>{getLocale('contributionTime60')}</div>
            </Select>
          </ControlWrapper>
          <ControlWrapper text={getLocale('contributionMinVisits')}>
            <Select
              onChange={this.onSelectSettingChange.bind(this, 'contributionMinVisits')}
              value={(contributionMinVisits || '').toString()}
            >
              <div data-value='1'>{getLocale('contributionVisit1')}</div>
              <div data-value='5'>{getLocale('contributionVisit5')}</div>
              <div data-value='10'>{getLocale('contributionVisit10')}</div>
            </Select>
          </ControlWrapper>
          <ControlWrapper text={getLocale('contributionAllowed')}>
            <Checkbox
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
          </ControlWrapper>
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
      enabledContribute,
      reconcileStamp,
      autoContributeList
    } = this.props.rewardsData
    const includedPublishers = this.getIncludedPublishers(autoContributeList)
    const toggleOn = !(firstLoad !== false || !enabledMain)
    const monthlyList: MonthlyChoice[] = utils.generateContributionMonthly(walletInfo.choices, walletInfo.rates)
    const contributeRows = this.getContributeRows(includedPublishers)
    const topRows = contributeRows.slice(0, 5)
    const numRows = includedPublishers && includedPublishers.length
    const allSites = !(numRows > 5)
    const numExcludedSites = autoContributeList.length - includedPublishers.length

    return (
      <Box
        title={getLocale('contributionTitle')}
        type={'contribute'}
        description={getLocale('contributionDesc')}
        toggle={toggleOn}
        checked={toggleOn ? enabledContribute : false}
        settingsChild={this.contributeSettings(monthlyList)}
        disabledContent={this.contributeDisabled()}
        onToggle={this.onToggleContribution}
        testId={'autoContribution'}
      >
        {
          this.state.modalContribute
          ? <ModalContribute
            rows={contributeRows}
            onRestore={this.onRestore}
            numExcludedSites={numExcludedSites}
            onClose={this.onModalContributeToggle}
          />
          : null
        }
        <List title={getLocale('contributionMonthly')}>
          <Select
            floating={true}
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
        <List title={getLocale('contributionNextDate')}>
          <NextContribution>{new Date(reconcileStamp).toLocaleDateString()}</NextContribution>
        </List>
        <List title={getLocale('contributionSites')}>
          {getLocale('total')} &nbsp;<Tokens value={numRows} hideText={true} toFixed={false} />
        </List>
        <TableContribute
          header={[
            getLocale('site'),
            getLocale('rewardsContributeAttention')
          ]}
          rows={topRows}
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

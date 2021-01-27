/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Checkbox, Grid, Column, Select, ControlWrapper } from 'brave-ui/components'
import { Box, TableContribute, DisabledContent, List, ModalContribute, Tokens, NextContribution } from '../../ui/components'
import { Provider } from '../../ui/components/profile'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'

interface State {
  modalContribute: boolean
  settings: boolean
  activeTabId: number
}

interface MonthlyChoice {
  tokens: string
  converted: string
}

interface Props extends Rewards.ComponentProps {
}

class ContributeBox extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalContribute: false,
      settings: false,
      activeTabId: 0
    }
  }

  getContributeRows = (list: Rewards.Publisher[]) => {
    return list
      .sort((a, b) => b.percentage - a.percentage)
      .map((item: Rewards.Publisher) => {
        const verified = utils.isPublisherConnectedOrVerified(item.status)
        let faviconUrl = `chrome://favicon/size/64@1x/${item.url}`
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
          url: item.url,
          attention: item.percentage,
          onRemove: () => { this.actions.excludePublisher(item.id) }
        }
      })
  }

  getExcludedRows = (list?: Rewards.ExcludedPublisher[]) => {
    if (!list) {
      return []
    }

    return list.map((item: Rewards.ExcludedPublisher) => {
      const verified = utils.isPublisherConnectedOrVerified(item.status)
      let faviconUrl = `chrome://favicon/size/64@1x/${item.url}`
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
        url: item.url,
        attention: 0,
        onRemove: () => { this.actions.restorePublisher(item.id) }
      }
    })
  }

  onTabChange = () => {
    const newId = this.state.activeTabId === 0 ? 1 : 0

    this.setState({
      activeTabId: newId
    })
  }

  get actions () {
    return this.props.actions
  }

  contributeDisabled () {
    return (
      <DisabledContent
        type={'contribute'}
      >
        {getLocale('contributionDisabledText1')} <br />
        {getLocale('contributionDisabledText2')}
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
      ui
    } = this.props.rewardsData

    const { onlyAnonWallet } = ui

    return (
      <Grid columns={1} customStyle={{ margin: '0 auto' }}>
        <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <ControlWrapper text={getLocale('contributionMonthly')}>
            <Select
              onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
              value={parseFloat((contributionMonthly.toString() || '0')).toFixed(3)}
            >
              {
                monthlyList.map((choice: MonthlyChoice) => {
                  return <div key={`choice-setting-${choice.tokens}`} data-value={choice.tokens.toString()}>
                    {getLocale('contributionUpTo')} <Tokens value={choice.tokens} converted={choice.converted} onlyAnonWallet={onlyAnonWallet} />
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
          <ControlWrapper text={getLocale('contributionOther')}>
            <Checkbox
              value={{
                contributionNonVerified: contributionNonVerified,
                contributionVideos: contributionVideos
              }}
              multiple={true}
              onChange={this.onCheckSettingChange}
            >
              <div data-key='contributionNonVerified'>{getLocale('contributionShowNonVerified')}</div>
              <div data-key='contributionVideos'>{getLocale('contributionVideos')}</div>
            </Checkbox>
          </ControlWrapper>
        </Column>
      </Grid>
    )
  }

  onSettingsToggle = () => {
    this.setState({ settings: !this.state.settings })
  }

  render () {
    const {
      firstLoad,
      parameters,
      contributionMonthly,
      enabledContribute,
      reconcileStamp,
      autoContributeList,
      excludedList,
      externalWallet,
      ui
    } = this.props.rewardsData
    const monthlyList: MonthlyChoice[] = utils.generateContributionMonthly(parameters)
    const contributeRows = this.getContributeRows(autoContributeList)
    const excludedRows = this.getExcludedRows(excludedList)
    const topRows = contributeRows.slice(0, 5)
    const numRows = contributeRows && contributeRows.length
    const numExcludedRows = excludedRows && excludedRows.length
    const allSites = !(excludedRows.length > 0 || numRows > 5)
    const showDisabled = firstLoad !== false || !enabledContribute
    const { onlyAnonWallet } = ui

    // Hide AC options from bitFlyer wallet regions.
    if (externalWallet && externalWallet.type === 'bitflyer') {
      return null
    }

    return (
      <Box
        title={getLocale('contributionTitle')}
        type={'contribute'}
        description={getLocale('contributionDesc')}
        toggle={true}
        checked={enabledContribute}
        settingsChild={this.contributeSettings(monthlyList)}
        disabledContent={showDisabled ? this.contributeDisabled() : null}
        onToggle={this.onToggleContribution}
        testId={'autoContribution'}
        settingsOpened={this.state.settings}
        onSettingsClick={this.onSettingsToggle}
      >
        {
          this.state.modalContribute
          ? <ModalContribute
            rows={contributeRows}
            onRestore={this.onRestore}
            excludedRows={excludedRows}
            activeTabId={this.state.activeTabId}
            onTabChange={this.onTabChange}
            onClose={this.onModalContributeToggle}
          />
          : null
        }
        <List title={getLocale('contributionMonthly')}>
          <Select
            floating={true}
            onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
            value={parseFloat((contributionMonthly.toString() || '0')).toFixed(3)}
            showAllContents={true}
          >
            {
              monthlyList.map((choice: MonthlyChoice) => {
                return (
                  <div key={`choice-${choice.tokens}`} data-value={choice.tokens.toString()}>
                    {getLocale('contributionUpTo')} <Tokens onlyAnonWallet={onlyAnonWallet} value={choice.tokens} converted={choice.converted} />
                  </div>
                )
              })
            }
          </Select>
        </List>
        <List title={getLocale('contributionNextDate')}>
          <NextContribution>
            {new Intl.DateTimeFormat('default', { month: 'short', day: 'numeric' }).format(reconcileStamp * 1000)}
          </NextContribution>
        </List>
        <List title={getLocale('contributionSites')}>
          <Tokens value={numRows.toString()} hideText={true} />
        </List>
        <TableContribute
          header={[
            getLocale('site'),
            getLocale('rewardsContributeAttention')
          ]}
          testId={'autoContribute'}
          rows={topRows}
          allSites={allSites}
          numSites={numRows}
          onShowAll={this.onModalContributeToggle}
          headerColor={true}
          showRemove={true}
          numExcludedSites={numExcludedRows}
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

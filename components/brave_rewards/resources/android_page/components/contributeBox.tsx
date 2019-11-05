/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  StyledListContent,
  StyledSitesNum,
  StyledTotalContent,
  StyledSitesLink
} from './style'
import { List, NextContribution, TableContribute, Tokens } from '../../ui/components'
import { BoxMobile, SelectMobile } from '../../ui/components/mobile'
import { Column, Grid, ControlWrapper, Checkbox } from 'brave-ui/components'
import { Provider } from '../../ui/components/profile'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import * as utils from '../utils'

interface State {
  allSitesShown: boolean
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
      allSitesShown: false
    }
  }

  getContributeRows = (list: Rewards.Publisher[]) => {
    return list.map((item: Rewards.Publisher) => {
      const verified = utils.isPublisherConnectedOrVerified(item.status)
      let faviconUrl = `chrome://favicon/size/48@1x/${item.url}`
      if (item.favIcon && verified) {
        faviconUrl = `chrome://favicon/size/48@1x/${item.favIcon}`
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

  get actions () {
    return this.props.actions
  }

  onRestore = () => {
    this.actions.restorePublishers()
  }

  onToggleContribution = () => {
    this.actions.onSettingSave('enabledContribute', !this.props.rewardsData.enabledContribute)
  }

  onSelectSettingChange = (key: string, value: string) => {
    this.actions.onSettingSave(key, +value)
  }

  onCheckSettingChange = (key: string, selected: boolean) => {
    this.actions.onSettingSave(key, selected)
  }

  onSitesShownToggle = () => {
    this.setState({
      allSitesShown: !this.state.allSitesShown
    })
  }

  getAmountOptions = (monthlyList: MonthlyChoice[]) => {
    return monthlyList.map((choice: MonthlyChoice) => {
      return {
        value: choice.tokens,
        dataValue: choice.tokens.toString(),
        converted: choice.converted
      }
    })
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
            <SelectMobile
              amountOptions={this.getAmountOptions(monthlyList)}
              onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
              value={parseFloat((contributionMonthly.toString() || '0')).toFixed(1)}
            />
          </ControlWrapper>
          <ControlWrapper text={getLocale('contributionMinTime')}>
            <SelectMobile
              onChange={this.onSelectSettingChange.bind(this, 'contributionMinTime')}
              value={(contributionMinTime || '').toString()}
              options={[
                {
                  value: '5',
                  text: getLocale('contributionTime5')
                },
                {
                  value: '8',
                  text: getLocale('contributionTime8')
                },
                {
                  value: '60',
                  text: getLocale('contributionTime60')
                }
              ]}
            />
          </ControlWrapper>
          <ControlWrapper text={getLocale('contributionMinVisits')}>
            <SelectMobile
              onChange={this.onSelectSettingChange.bind(this, 'contributionMinVisits')}
              value={(contributionMinVisits || '').toString()}
              options={[
                {
                  value: '1',
                  text: getLocale('contributionVisit1')
                },
                {
                  value: '5',
                  text: getLocale('contributionVisit5')
                },
                {
                  value: '10',
                  text: getLocale('contributionVisit10')
                }
              ]}
            />
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

  render () {
    const {
      walletInfo,
      contributionMonthly,
      enabledMain,
      reconcileStamp,
      autoContributeList,
      enabledContribute,
      excludedList,
      balance
    } = this.props.rewardsData
    const prefix = this.state.allSitesShown ? 'Hide all' : 'Show all'
    const monthlyList: MonthlyChoice[] = utils.generateContributionMonthly(walletInfo.choices, balance.rates)
    const contributeRows = this.getContributeRows(autoContributeList)
    const shownRows = this.state.allSitesShown ? contributeRows : contributeRows.slice(0, 5)
    const numRows = contributeRows && contributeRows.length
    const numExcludedRows = excludedList && excludedList.length

    return (
      <BoxMobile
        title={getLocale('contributionTitle')}
        type={'contribute'}
        description={getLocale('contributionDesc')}
        toggle={enabledMain}
        checked={enabledMain && enabledContribute}
        settingsChild={this.contributeSettings(monthlyList)}
        toggleAction={this.onToggleContribution}
      >
        <List title={<StyledListContent>{getLocale('contributionMonthly')}</StyledListContent>}>
          <StyledListContent>
            <SelectMobile
              floating={true}
              amountOptions={this.getAmountOptions(monthlyList)}
              onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
              value={parseFloat((contributionMonthly.toString() || '0')).toFixed(1)}
            />
          </StyledListContent>
        </List>
        <List title={<StyledListContent>{getLocale('contributionNextDate')}</StyledListContent>}>
          <StyledListContent>
            <NextContribution>
              {new Intl.DateTimeFormat('default', { month: 'short', day: 'numeric' }).format(reconcileStamp * 1000)}
            </NextContribution>
          </StyledListContent>
        </List>
        <List title={<StyledListContent>{getLocale('contributionSites')}</StyledListContent>}>
          <StyledTotalContent>
            {getLocale('total')} &nbsp;<Tokens
              value={numRows.toString()}
              hideText={true}
            />
          </StyledTotalContent>
        </List>
        <StyledListContent>
          <TableContribute
            header={[
              getLocale('site'),
              getLocale('rewardsContributeAttention')
            ]}
            rows={shownRows}
            allSites={true}
            numSites={numRows}
            headerColor={true}
            showRemove={true}
            showRowAmount={true}
            isMobile={true}
            numExcludedSites={numExcludedRows}
          >
            {getLocale('contributionVisitSome')}
          </TableContribute>
        </StyledListContent>
        {
          numRows > 5
          ? <StyledSitesNum>
              <StyledSitesLink onClick={this.onSitesShownToggle}>
                {prefix} {numRows} sites
              </StyledSitesLink>
            </StyledSitesNum>
          : null
        }
      </BoxMobile>
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

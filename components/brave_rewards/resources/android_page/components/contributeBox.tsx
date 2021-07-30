/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  StyledListContent,
  StyledTotalContent
} from './style'
import { List, ModalContribute, NextContribution, TableContribute, Tokens } from '../../ui/components'
import { BoxMobile, SelectMobile } from '../../ui/components/mobile'
import { Column, Grid, ControlWrapper, Checkbox } from 'brave-ui/components'
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
    return list.map((item: Rewards.Publisher) => {
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
      contributionMonthly
    } = this.props.rewardsData

    return (
      <Grid columns={1} customStyle={{ maxWidth: '270px', margin: '0 auto' }}>
        <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <ControlWrapper text={getLocale('contributionMonthly')}>
            <SelectMobile
              onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
              value={parseFloat((contributionMonthly.toString() || '0')).toFixed(3)}
              amountOptions={this.getAmountOptions(monthlyList)}
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

  onSettingsToggle = () => {
    this.setState({ settings: !this.state.settings })
  }

  render () {
    const {
      parameters,
      contributionMonthly,
      enabledContribute,
      reconcileStamp,
      autoContributeList,
      excludedList
    } = this.props.rewardsData
    const monthlyList: MonthlyChoice[] = utils.generateContributionMonthly(parameters)
    const contributeRows = this.getContributeRows(autoContributeList)
    const excludedRows = this.getExcludedRows(excludedList)
    const topRows = contributeRows.slice(0, 5)
    const numRows = contributeRows && contributeRows.length
    const numExcludedRows = excludedRows && excludedRows.length
    const allSites = !(excludedRows.length > 0 || numRows > 5)

    return (
      <BoxMobile
        title={getLocale('contributionTitle')}
        type={'contribute'}
        description={getLocale('contributionDesc')}
        toggle={true}
        checked={enabledContribute}
        settingsChild={this.contributeSettings(monthlyList)}
        toggleAction={this.onToggleContribution}
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
        <List title={<StyledListContent>{getLocale('contributionMonthly')}</StyledListContent>}>
          <StyledListContent>
            <SelectMobile
              floating={true}
              onChange={this.onSelectSettingChange.bind(this, 'contributionMonthly')}
              value={parseFloat((contributionMonthly.toString() || '0')).toFixed(3)}
              amountOptions={this.getAmountOptions(monthlyList)}
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
            testId={'autoContribute'}
            rows={topRows}
            allSites={allSites}
            numSites={numRows}
            onShowAll={this.onModalContributeToggle}
            headerColor={true}
            showRemove={true}
            numExcludedSites={numExcludedRows}
            showRowAmount={true}
            isMobile={true}
          >
            {getLocale('contributionVisitSome')}
          </TableContribute>
        </StyledListContent>
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

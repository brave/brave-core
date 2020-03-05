/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  Box,
  BoxAlert,
  DisabledContent,
  List,
  NextContribution,
  ModalShowAdsHistory,
  ShowAdsHistory,
  Tokens
} from '../../ui/components'
import { Grid, Column, Select, ControlWrapper, Checkbox } from 'brave-ui/components'

// Utils
import * as utils from '../utils'
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

interface Props extends Rewards.ComponentProps {
}

interface State {
  settings: boolean
  modalShowAdsHistory: boolean
}

class AdsBox extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      settings: false,
      modalShowAdsHistory: false
    }
  }

  componentDidMount () {
    this.isShowAdsHistoryUrl()
    this.props.actions.getAdsHistory()
  }

  componentDidUpdate (prevProps: Props) {
    if (
      prevProps.rewardsData.enabledMain &&
      !this.props.rewardsData.enabledMain
    ) {
      this.setState({ settings: false })
    }
  }

  adsDisabled = () => {
    return (
      <DisabledContent
        type={'ads'}
      >
        {getLocale('adsDisabledTextOne')} <br />
        {getLocale('adsDisabledTextTwo')}
      </DisabledContent>
    )
  }

  adsNotSupportedAlert = (supported: boolean) => {
    if (supported) {
      return null
    }

    return (
      <BoxAlert type={'ads'} />
    )
  }

  onAdsSettingChange = (key: string, value: boolean) => {
    let newValue: any = value
    const { adsEnabled, shouldAllowAdConversionTracking, shouldShowPublisherAdsOnParticipatingSites } = this.props.rewardsData.adsData

    if (key === 'adsEnabled') {
      newValue = !adsEnabled
    } else if (key === 'shouldShowPublisherAdsOnParticipatingSites') {
      newValue = !shouldShowPublisherAdsOnParticipatingSites
    } else if (key === 'shouldAllowAdConversionTracking') {
      newValue = !shouldAllowAdConversionTracking
    }

    this.props.actions.onAdsSettingSave(key, newValue)
  }

  adsSettings = (enabled?: boolean) => {
    if (!enabled) {
      return null
    }

    const { adsPerHour, shouldAllowAdConversionTracking, shouldShowPublisherAdsOnParticipatingSites } = this.props.rewardsData.adsData

    return (
      <Grid columns={1} customStyle={{ margin: '0 auto' }}>
        <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <ControlWrapper text={getLocale('adsPerHour')}>
            <Select
              value={adsPerHour.toString()}
              onChange={this.onAdsSettingChange.bind(this, 'adsPerHour')}
            >
              {['1', '2', '3', '4', '5'].map((num: string) => {
                return (
                  <div key={`num-per-hour-${num}`} data-value={num}>
                   {getLocale(`adsPerHour${num}`)}
                  </div>
                )
              })}
            </Select>
          </ControlWrapper>
          <ControlWrapper text={getLocale('adsDeliveryPermissions')}>
            <Checkbox
              value={{
                shouldShowPublisherAdsOnParticipatingSites: shouldShowPublisherAdsOnParticipatingSites
              }}
              multiple={true}
              onChange={this.onAdsSettingChange}
            >
              <div data-key='shouldShowPublisherAdsOnParticipatingSites'>{getLocale('adsShowOnParticipatingPublisherSites')}</div>
            </Checkbox>
            <div>{getLocale('adsShowOnParticipatingPublisherSitesDescription')}</div>
          </ControlWrapper>
          <ControlWrapper text={getLocale('adsOtherSettings')}>
            <Checkbox
              value={{
                shouldAllowAdConversionTracking: shouldAllowAdConversionTracking
              }}
              multiple={true}
              onChange={this.onAdsSettingChange}
            >
              <div data-key='shouldAllowAdConversionTracking'>{getLocale('adsAllowConversionTracking')}</div>
            </Checkbox>
            <div>{getLocale('adsAllowConversionTrackingDescription')}</div>
          </ControlWrapper>
        </Column>
      </Grid>
    )
  }

  onSettingsToggle = () => {
    this.setState({ settings: !this.state.settings })
  }

  onAdsHistoryToggle = () => {
    if (!this.state.modalShowAdsHistory) {
      this.props.actions.getAdsHistory()
    }
    this.setState({
      modalShowAdsHistory: !this.state.modalShowAdsHistory
    })
  }

  closeShowAdsHistory = () => {
    if (this.urlHashIs('#ads-history')) {
      window.location.hash = ''
    }
    this.onAdsHistoryToggle()
  }

  urlHashIs = (hash: string) => {
    return (
      window &&
      window.location &&
      window.location.hash !== '' &&
      window.location.hash === hash
    )
  }

  isShowAdsHistoryUrl = () => {
    this.setState({
      modalShowAdsHistory: this.urlHashIs('#ads-history')
    })
  }

  onThumbUpPress = (creativeInstanceId: string, creativeSetId: string, action: number) => {
    this.props.actions.toggleAdThumbUp(creativeInstanceId, creativeSetId, action)
  }

  onThumbDownPress = (creativeInstanceId: string, creativeSetId: string, action: number) => {
    this.props.actions.toggleAdThumbDown(creativeInstanceId, creativeSetId, action)
  }

  onOptInAction = (category: string, action: number) => {
    this.props.actions.toggleAdOptInAction(category, action)
  }

  onOptOutAction = (category: string, action: number) => {
    this.props.actions.toggleAdOptOutAction(category, action)
  }

  onMenuSave = (creativeInstanceId: string, creativeSetId: string, saved: boolean) => {
    this.props.actions.toggleSaveAd(creativeInstanceId, creativeSetId, saved)
  }

  onMenuFlag = (creativeInstanceId: string, creativeSetId: string, flagged: boolean) => {
    this.props.actions.toggleFlagAd(creativeInstanceId, creativeSetId, flagged)
  }

  getGroupedAdsHistory = (adsHistory: Rewards.AdsHistory[], savedOnly: boolean) => {
    let groupedAdsHistory: Rewards.AdsHistory[] = []

    for (let i = 0; i < adsHistory.length; i++) {
      const adHistory = adsHistory[i]

      const uuid = adHistory.uuid

      let flooredDate = new Date(adHistory.timestampInMilliseconds)
      flooredDate.setHours(0, 0, 0, 0)
      const flooredDateString = flooredDate.toLocaleDateString()

      for (let j = 0; j < adHistory.adDetailRows.length; j++) {
        const adDetailRow = this.getAdDetailRow(adHistory.adDetailRows[j])

        const index = groupedAdsHistory.findIndex(item => item.date === flooredDateString)
        if (index === -1) {
          groupedAdsHistory.push({
            uuid: uuid,
            date: flooredDateString,
            adDetailRows: [adDetailRow]
          })
        } else {
          groupedAdsHistory[index].adDetailRows.push(adDetailRow)
        }
      }
    }

    return groupedAdsHistory
  }

  getAdDetailRow = (adHistory: Rewards.AdHistory) => {
    let brandInfo = adHistory.adContent.brandInfo
    if (brandInfo.length > 50) {
      brandInfo = brandInfo.substring(0, 50) + '...'
    }

    const adContent: Rewards.AdContent = {
      creativeInstanceId: adHistory.adContent.creativeInstanceId,
      creativeSetId: adHistory.adContent.creativeSetId,
      brand: adHistory.adContent.brand,
      brandInfo: brandInfo,
      brandLogo: adHistory.adContent.brandLogo,
      brandDisplayUrl: adHistory.adContent.brandDisplayUrl,
      brandUrl: adHistory.adContent.brandUrl,
      likeAction: adHistory.adContent.likeAction,
      adAction: adHistory.adContent.adAction,
      savedAd: adHistory.adContent.savedAd,
      flaggedAd: adHistory.adContent.flaggedAd,
      onThumbUpPress: () =>
        this.onThumbUpPress(adHistory.adContent.creativeInstanceId,
                            adHistory.adContent.creativeSetId,
                            adHistory.adContent.likeAction),
      onThumbDownPress: () =>
        this.onThumbDownPress(adHistory.adContent.creativeInstanceId,
                              adHistory.adContent.creativeSetId,
                              adHistory.adContent.likeAction),
      onMenuSave: () =>
        this.onMenuSave(adHistory.adContent.creativeInstanceId,
                        adHistory.adContent.creativeSetId,
                        adHistory.adContent.savedAd),
      onMenuFlag: () =>
        this.onMenuFlag(adHistory.adContent.creativeInstanceId,
                        adHistory.adContent.creativeSetId,
                        adHistory.adContent.flaggedAd)
    }

    const categoryContent: Rewards.CategoryContent = {
      category: adHistory.categoryContent.category,
      optAction: adHistory.categoryContent.optAction,
      onOptInAction: () =>
        this.onOptInAction(adHistory.categoryContent.category,
                           adHistory.categoryContent.optAction),
      onOptOutAction: () =>
        this.onOptOutAction(adHistory.categoryContent.category,
                            adHistory.categoryContent.optAction)
    }

    return {
      uuid: adHistory.uuid,
      adContent: adContent,
      categoryContent: categoryContent
    }
  }

  hasSavedEntries = (adHistory: Rewards.AdsHistory[]) => {
    for (let ix = 0; ix < adHistory.length; ix++) {
      for (let jx = 0; jx < adHistory[ix].adDetailRows.length; jx++) {
        if (adHistory[ix].adDetailRows[jx].adContent.savedAd) {
          return true
        }
      }
    }
    return false
  }

  render () {
    const savedOnly = false

    let adsEnabled = false
    let adsPerHour = 0
    let adsUIEnabled = false
    let adsIsSupported = false
    let estimatedPendingRewards = 0
    let nextPaymentDate = ''
    let adNotificationsReceivedThisMonth = 0

    const {
      adsData,
      adsHistory,
      enabledMain,
      firstLoad,
      balance,
      ui
    } = this.props.rewardsData
    const { onlyAnonWallet } = ui

    if (adsData) {
      adsEnabled = adsData.adsEnabled
      adsPerHour = adsData.adsPerHour
      adsUIEnabled = adsData.adsUIEnabled
      adsIsSupported = adsData.adsIsSupported
      estimatedPendingRewards = adsData.adsEstimatedPendingRewards || 0
      nextPaymentDate = adsData.adsNextPaymentDate
      adNotificationsReceivedThisMonth = adsData.adsAdNotificationsReceivedThisMonth || 0
    }

    const enabled = adsEnabled && adsIsSupported
    const toggle = !(!enabledMain || !adsUIEnabled || !adsIsSupported)
    const showDisabled = firstLoad !== false || !toggle || !adsEnabled || !adsIsSupported

    const historyEntries = adsHistory || []
    const rows = this.getGroupedAdsHistory(historyEntries, savedOnly)
    const notEmpty = rows && rows.length !== 0
    const tokenString = getLocale(onlyAnonWallet ? 'points' : 'tokens')

    return (
      <>
        <Box
          title={getLocale('adsTitle')}
          type={'ads'}
          description={getLocale('adsDesc', { currency: tokenString })}
          toggle={toggle}
          checked={enabled}
          settingsChild={this.adsSettings(enabled && enabledMain)}
          testId={'braveAdsSettings'}
          disabledContent={showDisabled ? this.adsDisabled() : null}
          onToggle={this.onAdsSettingChange.bind(this, 'adsEnabled', '')}
          settingsOpened={this.state.settings}
          onSettingsClick={this.onSettingsToggle}
          attachedAlert={this.adsNotSupportedAlert(adsIsSupported)}
        >
          <List title={getLocale('adsCurrentEarnings')}>
            <Tokens
              onlyAnonWallet={onlyAnonWallet}
              value={estimatedPendingRewards.toFixed(1)}
              converted={utils.convertBalance(estimatedPendingRewards, balance.rates)}
            />
          </List>
          <List title={getLocale('adsPaymentDate')}>
            <NextContribution>
              {nextPaymentDate}
            </NextContribution>
          </List>
          <List title={getLocale('adsNotificationsReceived')}>
            <Tokens
              value={adNotificationsReceivedThisMonth.toString()}
              hideText={true}
            />
          </List>
          {
            notEmpty
            ? <ShowAdsHistory
                onAdsHistoryOpen={this.onAdsHistoryToggle}
                notEmpty={notEmpty}
            />
            : null
          }
        </Box>
        {
          this.state.modalShowAdsHistory
          ? <ModalShowAdsHistory
              onClose={this.closeShowAdsHistory}
              adsPerHour={adsPerHour}
              rows={rows}
              hasSavedEntries={this.hasSavedEntries(rows)}
              totalDays={7}
          />
          : null
        }
      </>
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
)(AdsBox)

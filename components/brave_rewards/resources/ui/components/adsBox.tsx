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
} from 'brave-ui/features/rewards'
import { Grid, Column, Select, ControlWrapper } from 'brave-ui/components'

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

  onAdsSettingChange = (key: string, value: string) => {
    let newValue: any = value
    const { adsEnabled } = this.props.rewardsData.adsData

    if (key === 'adsEnabled') {
      newValue = !adsEnabled
    }

    this.props.actions.onAdsSettingSave(key, newValue)
  }

  adsSettings = (enabled?: boolean) => {
    if (!enabled) {
      return null
    }

    const { adsPerHour } = this.props.rewardsData.adsData

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

  onThumbUpPress = (uuid: string, creativeSetId: string, action: number) => {
    this.props.actions.toggleAdThumbUp(uuid, creativeSetId, action)
  }

  onThumbDownPress = (uuid: string, creativeSetId: string, action: number) => {
    this.props.actions.toggleAdThumbDown(uuid, creativeSetId, action)
  }

  onOptInAction = (category: string, action: number) => {
    this.props.actions.toggleAdOptInAction(category, action)
  }

  onOptOutAction = (category: string, action: number) => {
    this.props.actions.toggleAdOptOutAction(category, action)
  }

  onMenuSave = (uuid: string, creativeSetId: string, saved: boolean) => {
    this.props.actions.toggleSaveAd(uuid, creativeSetId, saved)
  }

  onMenuFlag = (uuid: string, creativeSetId: string, flagged: boolean) => {
    this.props.actions.toggleFlagAd(uuid, creativeSetId, flagged)
  }

  getAdHistoryData = (adHistoryData: Rewards.AdsHistoryData[], savedOnly: boolean) => {
    return adHistoryData.map((item: Rewards.AdsHistoryData) => {
      return {
        id: item.id,
        date: new Date(item.timestampInMilliseconds).toLocaleDateString(),
        adDetailRows: (
          item.adDetailRows.map((itemDetail: Rewards.AdHistoryDetail) => {
            let brandInfo = itemDetail.adContent.brandInfo
            if (brandInfo.length > 50) {
              brandInfo = brandInfo.substring(0, 50) + '...'
            }
            const adContent: Rewards.AdContent = {
              uuid: itemDetail.adContent.uuid,
              creativeSetId: itemDetail.adContent.creativeSetId,
              brand: itemDetail.adContent.brand,
              brandInfo: brandInfo,
              brandLogo: itemDetail.adContent.brandLogo,
              brandDisplayUrl: itemDetail.adContent.brandDisplayUrl,
              brandUrl: itemDetail.adContent.brandUrl,
              likeAction: itemDetail.adContent.likeAction,
              adAction: itemDetail.adContent.adAction,
              savedAd: itemDetail.adContent.savedAd,
              flaggedAd: itemDetail.adContent.flaggedAd,
              onThumbUpPress: () =>
                this.onThumbUpPress(itemDetail.adContent.uuid,
                                    itemDetail.adContent.creativeSetId,
                                    itemDetail.adContent.likeAction),
              onThumbDownPress: () =>
                this.onThumbDownPress(itemDetail.adContent.uuid,
                                      itemDetail.adContent.creativeSetId,
                                      itemDetail.adContent.likeAction),
              onMenuSave: () =>
                this.onMenuSave(itemDetail.adContent.uuid,
                                itemDetail.adContent.creativeSetId,
                                itemDetail.adContent.savedAd),
              onMenuFlag: () =>
                this.onMenuFlag(itemDetail.adContent.uuid,
                                itemDetail.adContent.creativeSetId,
                                itemDetail.adContent.flaggedAd)
            }
            const categoryContent: Rewards.CategoryContent = {
              category: itemDetail.categoryContent.category,
              optAction: itemDetail.categoryContent.optAction,
              onOptInAction: () =>
                this.onOptInAction(itemDetail.categoryContent.category,
                                   itemDetail.categoryContent.optAction),
              onOptOutAction: () =>
                this.onOptOutAction(itemDetail.categoryContent.category,
                                    itemDetail.categoryContent.optAction)
            }
            return {
              id: itemDetail.id,
              adContent: adContent,
              categoryContent: categoryContent
            }
          })
        )
      }
    })
  }

  hasSavedEntries = (adHistoryData: Rewards.AdsHistoryData[]) => {
    for (let ix = 0; ix < adHistoryData.length; ix++) {
      for (let jx = 0; jx < adHistoryData[ix].adDetailRows.length; jx++) {
        if (adHistoryData[ix].adDetailRows[jx].adContent.savedAd) {
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
    let estimatedPendingRewards = '0'
    let nextPaymentDate = ''
    let adNotificationsReceivedThisMonth = 0

    const {
      adsData,
      adsHistory,
      enabledMain,
      firstLoad,
      balance
    } = this.props.rewardsData

    if (adsData) {
      adsEnabled = adsData.adsEnabled
      adsPerHour = adsData.adsPerHour
      adsUIEnabled = adsData.adsUIEnabled
      adsIsSupported = adsData.adsIsSupported
      estimatedPendingRewards = (adsData.adsEstimatedPendingRewards || 0).toFixed(2)
      nextPaymentDate = adsData.adsNextPaymentDate
      adNotificationsReceivedThisMonth = adsData.adsAdNotificationsReceivedThisMonth || 0
    }

    const enabled = adsEnabled && adsIsSupported
    const toggle = !(!enabledMain || !adsUIEnabled || !adsIsSupported)
    const showDisabled = firstLoad !== false || !toggle || !adsEnabled || !adsIsSupported

    if (!adsHistory) {
      return null
    }

    const rows = this.getAdHistoryData(adsHistory, savedOnly)
    const notEmpty = rows && rows.length !== 0

    return (
      <>
        <Box
          title={getLocale('adsTitle')}
          type={'ads'}
          description={getLocale('adsDesc')}
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
              value={estimatedPendingRewards}
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

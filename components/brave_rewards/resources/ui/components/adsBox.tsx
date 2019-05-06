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
      window.location.hash &&
      window.location.hash === hash
    )
  }

  isShowAdsHistoryUrl = () => {
    if (this.urlHashIs('#ads-history')) {
      this.setState({
        modalShowAdsHistory: true
      })
    } else {
      this.setState({
        modalShowAdsHistory: false
      })
    }
  }

  onThumbUpPress = () => {
    console.log('onThumbUpPress')
  }

  onThumbDownPress = () => {
    console.log('onThumbDownPress')
  }

  onOptInAction = () => {
    console.log('onOptInAction')
  }

  onOptOutAction = () => {
    console.log('onOptOutAction')
  }

  onMenuSave = () => {
    console.log('onMenuSave')
  }

  onMenuFlag = () => {
    console.log('onMenuFlag')
  }

  getAdHistoryData = (adHistoryData: Rewards.AdsHistoryData[], savedOnly: boolean) => {
    return adHistoryData.map((item: Rewards.AdsHistoryData, ix: number) => {
      return {
        id: ix,
        date: item.date,
        adDetailRows: (
          item.adDetailRows.map((itemDetail: Rewards.AdHistoryDetail, jx: number) => {
            const adContent: Rewards.AdContent = {
              brand: itemDetail.adContent.brand,
              brandInfo: itemDetail.adContent.brandInfo,
              brandLogo: itemDetail.adContent.brandLogo,
              brandDisplayUrl: itemDetail.adContent.brandDisplayUrl,
              brandUrl: itemDetail.adContent.brandUrl,
              likeAction: itemDetail.adContent.likeAction,
              adAction: itemDetail.adContent.adAction,
              savedAd: itemDetail.adContent.savedAd,
              flaggedAd: itemDetail.adContent.flaggedAd,
              onThumbUpPress: () => this.onThumbUpPress(),
              onThumbDownPress: () => this.onThumbDownPress(),
              onMenuSave: () => this.onMenuSave(),
              onMenuFlag: () => this.onMenuFlag()
            }
            const categoryContent: Rewards.CategoryContent = {
              category: itemDetail.categoryContent.category,
              optAction: itemDetail.categoryContent.optAction,
              onOptInAction: () => this.onOptInAction(),
              onOptOutAction: () => this.onOptOutAction()
            }
            return {
              id: jx,
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
    const adsPerHour = 2
    const adId: number = 0
    const rowId: number = 0
    const savedOnly: boolean = false

    /******** ASSUMED DATA - REMOVE UPON IMPLEMENTATION */
    const adsHistory: Rewards.AdsHistoryData[] =
    [
      {
        id: rowId,
        date: '1/30',
        adDetailRows: [
          {
            id: adId,
            adContent: {
              brand: 'Pepsi',
              brandLogo: '',
              brandUrl: 'https://www.pepsi.com',
              brandDisplayUrl: 'pepsi.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'Viewed',
              likeAction: 1,
              savedAd: true,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Entertainment',
              optAction: 0
            }
          },
          {
            id: adId + 1,
            adContent: {
              brand: 'TESLA',
              brandLogo: '',
              brandDisplayUrl: 'tesla.com',
              brandUrl: 'https://www.tesla.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'Clicked',
              likeAction: 2,
              savedAd: false,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Auto',
              optAction: 0
            }
          },
          {
            id: adId + 2,
            adContent: {
              brand: 'Disney',
              brandLogo: '',
              brandDisplayUrl: 'disney.com',
              brandUrl: 'https://www.disney.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'Clicked',
              likeAction: 0,
              savedAd: false,
              flaggedAd: true
            },
            categoryContent: {
              category: 'Travel',
              optAction: 0
            }
          }
        ]
      },
      {
        id: rowId + 1,
        date: '1/29',
        adDetailRows: [
          {
            id: adId + 3,
            adContent: {
              brand: 'Puma',
              brandLogo: '',
              brandDisplayUrl: 'puma.com',
              brandUrl: 'https://www.puma.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'Viewed',
              likeAction: 0,
              savedAd: false,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Sports',
              optAction: 0
            }
          },
          {
            id: adId + 4,
            adContent: {
              brand: 'Expedia.com',
              brandLogo: '',
              brandDisplayUrl: 'expedia.com',
              brandUrl: 'https://www.expedia.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'Viewed',
              likeAction: 0,
              savedAd: true,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Travel',
              optAction: 2
            }
          },
          {
            id: adId + 5,
            adContent: {
              brand: 'H&M',
              brandLogo: '',
              brandUrl: 'hm.com',
              brandDisplayUrl: 'https://www.hm.com',
              brandInfo: 'Animation & VFX Degree - Degree in Animation |',
              adAction: 'Closed',
              likeAction: 0,
              savedAd: true,
              flaggedAd: false
            },
            categoryContent: {
              category: 'Fashion',
              optAction: 1
            }
          }
        ]
      }
    ]
    /* end */

    const rows = this.getAdHistoryData(adsHistory, savedOnly)
    let adsEnabled = false
    let adsUIEnabled = false
    let adsIsSupported = false
    let estimatedPendingRewards = '0'
    let nextPaymentDate = ''
    let adNotificationsReceivedThisMonth = 0

    const {
      adsData,
      enabledMain,
      firstLoad,
      balance
    } = this.props.rewardsData

    if (adsData) {
      adsEnabled = adsData.adsEnabled
      adsUIEnabled = adsData.adsUIEnabled
      adsIsSupported = adsData.adsIsSupported
      estimatedPendingRewards = (adsData.adsEstimatedPendingRewards || 0).toFixed(2)
      nextPaymentDate = adsData.adsNextPaymentDate
      adNotificationsReceivedThisMonth = adsData.adsAdNotificationsReceivedThisMonth || 0
    }

    const enabled = adsEnabled && adsIsSupported
    const toggle = !(!enabledMain || !adsUIEnabled || !adsIsSupported)
    const showDisabled = firstLoad !== false || !toggle || !adsEnabled || !adsIsSupported

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
              converted={utils.convertBalance(estimatedPendingRewards, walletInfo.rates)}
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
          <ShowAdsHistory onAdsHistoryOpen={this.onAdsHistoryToggle} notEmpty={rows && rows.length !== 0}/>
        </Box>
        {
          this.state.modalShowAdsHistory
          ? <ModalShowAdsHistory
            onClose={this.closeShowAdsHistory}
            adsPerHour={adsPerHour}
            rows={rows}
            hasSavedEntries={this.hasSavedEntries(rows)}
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

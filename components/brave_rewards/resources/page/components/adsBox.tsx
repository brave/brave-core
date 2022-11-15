/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

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

import { Grid, Column, ControlWrapper } from 'brave-ui/components'
import { AlertCircleIcon } from 'brave-ui/components/icons'

import { LayoutKind } from '../lib/layout_context'
import { externalWalletProviderFromString } from '../../shared/lib/external_wallet'
import { getProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { PaymentStatusView } from '../../shared/components/payment_status_view'

import * as style from './adsBox.style'

import { convertBalance } from './utils'
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'

const nextPaymentDateFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short',
  day: 'numeric'
})

interface Props extends Rewards.ComponentProps {
  layout: LayoutKind
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

  adsDisabled = () => {
    return (
      <DisabledContent
        type={'ads'}
      >
        {getLocale('adsDisabledTextOne')}&nbsp;
        {getLocale('adsDisabledTextTwo')}
      </DisabledContent>
    )
  }

  needsBrowserUpdateView = () => {
    return (
      <style.needsBrowserUpdateView>
        <style.needsBrowserUpdateIcon>
          <AlertCircleIcon />
        </style.needsBrowserUpdateIcon>
        <style.needsBrowserUpdateContent>
          <style.needsBrowserUpdateContentHeader>
            {getLocale('rewardsBrowserCannotReceiveAds')}
          </style.needsBrowserUpdateContentHeader>
          <style.needsBrowserUpdateContentBody>
            {getLocale('rewardsBrowserNeedsUpdateToSeeAds')}
          </style.needsBrowserUpdateContentBody>
        </style.needsBrowserUpdateContent>
      </style.needsBrowserUpdateView>
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
    const { adsEnabled } = this.props.rewardsData.adsData
    if (key === 'adsEnabled') {
      value = String(!adsEnabled)
    }
    this.props.actions.onAdsSettingSave(key, value)
  }

  adsSettings = (enabled?: boolean) => {
    if (!enabled) {
      return null
    }

    const {
      adsPerHour,
      shouldAllowAdsSubdivisionTargeting,
      adsSubdivisionTargeting
    } = this.props.rewardsData.adsData

    const selectChangeHandler = (setting: string) => {
      return (event: React.FormEvent<HTMLSelectElement>) => {
        this.onAdsSettingChange(setting, event.currentTarget.value)
      }
    }

    return (
      <Grid columns={1} customStyle={{ margin: '0 auto' }}>
        <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
          <ControlWrapper text={getLocale('adsPerHour')}>
            <select
              value={adsPerHour.toString()}
              onChange={selectChangeHandler('adsPerHour')}
            >
              {
                [0, 1, 2, 3, 4, 5, 10].map((n) => (
                  <option key={`num-per-hour-${n}`} value={n}>
                    {getLocale(`adsPerHour${n}`)}
                  </option>
                ))
              }
            </select>
          </ControlWrapper>
        </Column>
        { shouldAllowAdsSubdivisionTargeting
          ? <>
            <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
              <ControlWrapper text={getLocale('adsSubdivisionTargetingTitle')}>
                <select
                  value={adsSubdivisionTargeting || ''}
                  onChange={selectChangeHandler('adsSubdivisionTargeting')}
                >
                  {
                    this.getAdsSubdivisions().map((subdivision) => {
                      return (
                        <option key={subdivision.code} value={subdivision.code}>
                          {subdivision.name}
                        </option>
                      )
                    })
                  }
                </select>
              </ControlWrapper>
            </Column>
            <div>
              {getLocale('adsSubdivisionTargetingDescription')} <a href={'https://support.brave.com/hc/en-us/articles/360026361072-Brave-Ads-FAQ'} target={'_blank'}>{getLocale('adsSubdivisionTargetingLearn')}</a>
            </div>
          </> : null }
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

  onThumbUpPress = (adContent: Rewards.AdContent) => {
    this.props.actions.toggleAdThumbUp(adContent)
  }

  onThumbDownPress = (adContent: Rewards.AdContent) => {
    this.props.actions.toggleAdThumbDown(adContent)
  }

  onOptIn = (category: string, action: number) => {
    this.props.actions.toggleAdOptIn(category, action)
  }

  onOptOut = (category: string, action: number) => {
    this.props.actions.toggleAdOptOut(category, action)
  }

  onMenuSave = (adContent: Rewards.AdContent) => {
    this.props.actions.toggleSavedAd(adContent)
  }

  onMenuFlag = (adContent: Rewards.AdContent) => {
    this.props.actions.toggleFlaggedAd(adContent)
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

  getAdsSubdivisions = (): Rewards.Subdivision[] => {
    const {
      adsSubdivisionTargeting,
      automaticallyDetectedAdsSubdivisionTargeting,
      subdivisions
    } = this.props.rewardsData.adsData

    if (!subdivisions || !subdivisions.length) {
      return []
    }

    let adsSubdivisionsList: Rewards.Subdivision[] = subdivisions.map(val => ({ ...val }))

    if (adsSubdivisionTargeting === 'DISABLED') {
      adsSubdivisionsList.unshift({ code: 'DISABLED', name: getLocale('adsSubdivisionTargetingDisabled') })
    } else {
      adsSubdivisionsList.unshift({ code: 'DISABLED', name: getLocale('adsSubdivisionTargetingDisable') })
    }

    const subdivisionMap = new Map<string, string>(subdivisions.map(v => [v.code, v.name]))
    const subdivision = subdivisionMap.get(automaticallyDetectedAdsSubdivisionTargeting)
    if (subdivision && adsSubdivisionTargeting === 'AUTO') {
      adsSubdivisionsList.unshift({ code: 'AUTO', name: getLocale('adsSubdivisionTargetingAutoDetectedAs', { adsSubdivisionTarget: subdivision }) })
    } else {
      adsSubdivisionsList.unshift({ code: 'AUTO', name: getLocale('adsSubdivisionTargetingAutoDetect') })
    }

    return adsSubdivisionsList
  }

  getAdDetailRow = (adHistory: Rewards.AdHistory) => {
    let brand = adHistory.adContent.brand
    if (brand.length > 50) {
      brand = brand.substring(0, 50) + '...'
    }

    let brandInfo = adHistory.adContent.brandInfo
    if (brandInfo.length > 50) {
      brandInfo = brandInfo.substring(0, 50) + '...'
    }

    const adContent: Rewards.AdContent = {
      adType: adHistory.adContent.adType,
      creativeInstanceId: adHistory.adContent.creativeInstanceId,
      creativeSetId: adHistory.adContent.creativeSetId,
      brand: brand,
      brandInfo: brandInfo,
      brandDisplayUrl: adHistory.adContent.brandDisplayUrl,
      brandUrl: adHistory.adContent.brandUrl,
      likeAction: adHistory.adContent.likeAction,
      adAction: adHistory.adContent.adAction,
      savedAd: adHistory.adContent.savedAd,
      flaggedAd: adHistory.adContent.flaggedAd,
      onThumbUpPress: () =>
        this.onThumbUpPress(adHistory.adContent),
      onThumbDownPress: () =>
        this.onThumbDownPress(adHistory.adContent),
      onMenuSave: () =>
        this.onMenuSave(adHistory.adContent),
      onMenuFlag: () =>
        this.onMenuFlag(adHistory.adContent)
    }

    const categoryContent: Rewards.CategoryContent = {
      category: adHistory.categoryContent.category,
      optAction: adHistory.categoryContent.optAction,
      onOptIn: () =>
        this.onOptIn(adHistory.categoryContent.category,
                     adHistory.categoryContent.optAction),
      onOptOut: () =>
        this.onOptOut(adHistory.categoryContent.category,
                      adHistory.categoryContent.optAction)
    }

    return {
      uuid: adHistory.uuid,
      adContent: adContent,
      categoryContent: categoryContent
    }
  }

  hasSavedEntries = (adHistory: Rewards.AdsHistory[]) => {
    return adHistory.some((adHistoryItem) => {
      return adHistoryItem.adDetailRows.some((row) => {
        return row.adContent.savedAd
      })
    })
  }

  hasLikedEntries = (adHistory: Rewards.AdsHistory[]) => {
    return adHistory.some((adHistoryItem) => {
      return adHistoryItem.adDetailRows.some((row) => {
        return row.adContent.likeAction === 1
      })
    })
  }

  render () {
    const savedOnly = false

    let adsEnabled = false
    let adsPerHour = 0
    let adsUIEnabled = false
    let adsIsSupported = false
    let nextPaymentDate = 0
    let adsReceivedThisMonth = 0
    let earningsThisMonth = 0
    let earningsLastMonth = 0
    let needsBrowserUpgradeToServeAds = false

    const {
      adsData,
      adsHistory,
      externalWallet,
      parameters
    } = this.props.rewardsData

    if (adsData) {
      adsEnabled = adsData.adsEnabled
      adsPerHour = adsData.adsPerHour
      adsUIEnabled = adsData.adsUIEnabled
      adsIsSupported = adsData.adsIsSupported
      nextPaymentDate = adsData.adsNextPaymentDate
      adsReceivedThisMonth = adsData.adsReceivedThisMonth || 0
      earningsThisMonth = adsData.adsEarningsThisMonth || 0
      earningsLastMonth = adsData.adsEarningsLastMonth || 0
      needsBrowserUpgradeToServeAds = adsData.needsBrowserUpgradeToServeAds
    }

    const enabled = adsEnabled && adsIsSupported
    const toggle = !(!adsUIEnabled || !adsIsSupported)
    const showDisabled = !toggle || !adsEnabled || !adsIsSupported

    const historyEntries = adsHistory || []
    const rows = this.getGroupedAdsHistory(historyEntries, savedOnly)
    const tokenString = getLocale('tokens')

    const walletStatus = externalWallet ? externalWallet.status : null
    const walletProvider = externalWallet
      ? externalWalletProviderFromString(externalWallet.type) : null
    const providerPayoutStatus = getProviderPayoutStatus(
      parameters.payoutStatus,
      walletProvider && walletStatus ? walletProvider : null)

    return (
      <>
        <Box
          title={getLocale('adsTitle')}
          type={'ads'}
          description={getLocale('adsDesc', { currency: tokenString })}
          toggle={toggle}
          checked={enabled}
          settingsChild={this.adsSettings(enabled)}
          testId={'braveAdsSettings'}
          disabledContent={showDisabled ? this.adsDisabled() : null}
          onToggle={this.onAdsSettingChange.bind(this, 'adsEnabled', '')}
          settingsOpened={this.state.settings}
          onSettingsClick={this.onSettingsToggle}
          attachedAlert={this.adsNotSupportedAlert(adsIsSupported)}
          headerAlert={
            (needsBrowserUpgradeToServeAds && !this.state.settings)
              ? this.needsBrowserUpdateView()
              : null
          }
        >
          <style.paymentStatus>
            <PaymentStatusView
              earningsLastMonth={earningsLastMonth}
              nextPaymentDate={nextPaymentDate}
              providerPayoutStatus={providerPayoutStatus}
            />
          </style.paymentStatus>
          <List title={getLocale('adsCurrentEarnings')}>
            <Tokens
              value={new Intl.NumberFormat(undefined, {
                minimumFractionDigits: 3,
                maximumFractionDigits: 3
              }).format(earningsThisMonth)}
              converted={convertBalance(earningsThisMonth, parameters.rate)}
            />
          </List>
          <List title={getLocale('adsPaymentDate')}>
            <NextContribution>
              {
                nextPaymentDate
                  ? nextPaymentDateFormatter.format(new Date(nextPaymentDate))
                  : ''
              }
            </NextContribution>
          </List>
          <List title={getLocale('adsNotificationsReceived')}>
            <Tokens
              value={adsReceivedThisMonth.toString()}
              hideText={true}
            />
          </List>
          {
            this.props.layout === 'wide' &&
              <ShowAdsHistory onAdsHistoryOpen={this.onAdsHistoryToggle} />
          }
        </Box>
        {
          this.state.modalShowAdsHistory
          ? <ModalShowAdsHistory
              onClose={this.closeShowAdsHistory}
              adsPerHour={adsPerHour}
              rows={rows}
              hasSavedEntries={this.hasSavedEntries(rows)}
              hasLikedEntries={this.hasLikedEntries(rows)}
              totalDays={30}
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

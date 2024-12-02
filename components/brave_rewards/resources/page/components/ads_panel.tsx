/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AdsHistory } from '../lib/types'
import { useActions, useRewardsData } from '../lib/redux_hooks'
import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { getProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { externalWalletFromExtensionData } from '../../shared/lib/external_wallet'

import {
  SettingsPanel,
  PanelHeader,
  PanelItem,
  MonthDay,
  ConfigHeader
} from './settings_panel'

import { ModalShowAdsHistory } from '../../ui/components'
import { AdsControlView } from './ads_control_view'
import { PaymentStatusView } from '../../shared/components/payment_status_view'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { EarningsRange } from '../../shared/components/earnings_range'
import { ArrowNextIcon } from '../../shared/components/icons/arrow_next_icon'
import { AlertIcon } from './icons/alert_icon'

import * as urls from '../../shared/lib/rewards_urls'
import * as style from './ads_panel.style'

const adsFaqURL = 'https://support.brave.com/hc/en-us/articles/360026361072-Brave-Ads-FAQ'

export function AdsPanel () {
  const { getString } = React.useContext(LocaleContext)
  const actions = useActions()

  const data = useRewardsData((state) => ({
    adsData: state.adsData,
    adsHistory: state.adsHistory,
    currentCountryCode: state.currentCountryCode,
    externalWallet: state.externalWallet,
    externalWalletProviderList: state.externalWalletProviderList,
    parameters: state.parameters,
    userType: state.userType,
    modalAdsHistory: state.ui.modalAdsHistory,
    showAdsSettings: state.ui.adsSettings
  }))

  React.useEffect(() => {
    if (data.modalAdsHistory) {
      actions.getAdsHistory()
    }
  }, [data.modalAdsHistory])

  const { adsData } = data

  const externalWallet = externalWalletFromExtensionData(data.externalWallet)

  const toggleModal = () => {
    if (data.modalAdsHistory) {
      actions.onModalAdsHistoryClose()
    } else {
      actions.onModalAdsHistoryOpen()
    }
  }

  const onShowConfigChange = () => {
    if (!adsData.shouldAllowAdsSubdivisionTargeting) {
      return undefined
    }
    return (showConfig: boolean) => {
      if (showConfig) {
        actions.onAdsSettingsOpen()
      } else {
        actions.onAdsSettingsClose()
      }
    }
  }

  function renderDescription () {
    return (
      <style.description>
        {getString('adsDescription')}
        {
          data.userType !== 'unconnected' &&
            (' ' + getString('adsDescriptionEarn'))
        }
        {' '}
        <NewTabLink href={adsFaqURL}>
          {getString('learnMore')}
        </NewTabLink>
      </style.description>
    )
  }

  function renderSubdivisionSelect () {
    const {
      adsSubdivisionTargeting,
      automaticallyDetectedAdsSubdivisionTargeting,
      shouldAllowAdsSubdivisionTargeting,
      subdivisions
    } = adsData

    if (!shouldAllowAdsSubdivisionTargeting) {
      return null
    }

    const optionList = subdivisions.map(val => ({ ...val }))
    if (optionList.length > 0) {
      optionList.unshift({
        subdivision: 'DISABLED',
        name: adsSubdivisionTargeting === 'DISABLED'
          ? getString('adsSubdivisionTargetingDisabled')
          : getString('adsSubdivisionTargetingDisable')
      })

      let automaticallyDetectedName = ''
      for (const item of subdivisions) {
        if (item.subdivision === automaticallyDetectedAdsSubdivisionTargeting) {
          automaticallyDetectedName = item.name
          break
        }
      }

      optionList.unshift({
        subdivision: 'AUTO',
        name: automaticallyDetectedName && adsSubdivisionTargeting === 'AUTO'
          ? getString('adsSubdivisionTargetingAutoDetectedAs')
              .replace('$1', automaticallyDetectedName)
          : getString('adsSubdivisionTargetingAutoDetect')
      })
    }

    const onChange = (event: React.FormEvent<HTMLSelectElement>) => {
      const { value } = event.currentTarget
      actions.onAdsSettingSave('adsSubdivisionTargeting', value)
    }

    const description = (
      <>
        {getString('adsSubdivisionTargetingDescription')}&nbsp;
        <NewTabLink href={adsFaqURL}>
          {getString('adsSubdivisionTargetingLearn')}
        </NewTabLink>
      </>
    )

    return (
      <>
        <PanelItem
          label={getString('adsSubdivisionTargetingTitle')}
          details={description}
        >
          <select value={adsData.adsSubdivisionTargeting} onChange={onChange}>
            {
              optionList.map(({ subdivision, name }) => (
                <option key={subdivision} value={subdivision}>{name}</option>
              ))
            }
          </select>
        </PanelItem>
      </>
    )
  }

  function renderConfig () {
    return (
      <>
        <ConfigHeader />
        {renderSubdivisionSelect()}
      </>
    )
  }

  function renderNeedsUpdateNotice () {
    if (!adsData.needsBrowserUpgradeToServeAds) {
      return null
    }

    return (
      <style.needsUpdate>
        <style.needsUpdateIcon>
          <AlertIcon />
        </style.needsUpdateIcon>
        <style.needsUpdateContent>
          <style.needsUpdateHeader>
            {getString('rewardsBrowserCannotReceiveAds')}
          </style.needsUpdateHeader>
          <style.needsUpdateBody>
            {getString('rewardsBrowserNeedsUpdateToSeeAds')}
          </style.needsUpdateBody>
        </style.needsUpdateContent>
      </style.needsUpdate>
    )
  }

  function renderConnectAcount () {
    const onConnect = () => { actions.onModalConnectOpen() }

    return (
      <style.connect>
        {
          formatMessage(getString('connectAccountText'), {
            tags: {
              $1: (content) => <strong key='bold'>{content}</strong>
            }
          })
        }
        <style.connectAction>
          <button onClick={onConnect}>
            {getString('rewardsConnectAccount')}<ArrowNextIcon />
          </button>
        </style.connectAction>
      </style.connect>
    )
  }

  function renderEarnings () {
    if (!externalWallet) {
      return (
        <style.hiddenEarnings>
          -&nbsp;-
          <NewTabLink href={urls.rewardsChangesURL}>
            {getString('rewardsLearnMore')}
          </NewTabLink>
        </style.hiddenEarnings>
      )
    }

    return (
      <style.earnings>
        <EarningsRange
          minimum={adsData.adsMinEarningsThisMonth}
          maximum={adsData.adsMaxEarningsThisMonth}
          minimumFractionDigits={3}
        />
      </style.earnings>
    )
  }

  function renderPaymentItems () {
    const providerPayoutStatus = getProviderPayoutStatus(
      data.parameters.payoutStatus,
      externalWallet ? externalWallet.provider : null)

    return (
      <>
        <style.paymentStatus>
          {
            data.userType === 'connected' &&
              <PaymentStatusView
                minEarnings={adsData.adsMinEarningsLastMonth}
                maxEarnings={adsData.adsMaxEarningsLastMonth}
                nextPaymentDate={adsData.adsNextPaymentDate}
                providerPayoutStatus={providerPayoutStatus}
              />
          }
        </style.paymentStatus>
        <style.earningsRow>
          <PanelItem label={getString('adsCurrentEarnings')}>
            {renderEarnings()}
          </PanelItem>
        </style.earningsRow>
        <PanelItem label={getString('adsPaymentDate')}>
          <MonthDay date={new Date(adsData.adsNextPaymentDate)} />
        </PanelItem>
      </>
    )
  }

  function renderContent () {
    if (!adsData.adsIsSupported) {
      return (
        <>
          {renderDescription()}
          <style.notSupported>
            <style.notSupportedIcon>
              <AlertIcon />
            </style.notSupportedIcon>
            <div>
              {getString('adsNotSupportedRegion')}
            </div>
          </style.notSupported>
        </>
      )
    }

    return (
      <>
        {renderDescription()}
        {
          data.userType === 'unconnected'
            ? renderConnectAcount()
            : renderPaymentItems()
        }
        <PanelItem
          label={getString('adsTotalReceivedLabel')}
          details={<AdsControlView />}
        >
          <style.totalAdsCount>
            {
              Object.values(adsData.adTypesReceivedThisMonth)
                .reduce((total, count) => total + count, 0)
            }
          </style.totalAdsCount>
        </PanelItem>
        <style.showHistory>
          <button onClick={toggleModal}>
            {getString('openAdsHistory')}
          </button>
        </style.showHistory>
      </>
    )
  }

  function renderModal() {
    if (!data.modalAdsHistory) {
      return null
    }

    const groupedHistory: AdsHistory[] = []

    for (const history of data.adsHistory) {
      const flooredDate = new Date(history.timestampInMilliseconds)
      flooredDate.setHours(0, 0, 0, 0)
      const flooredDateString = flooredDate.toLocaleDateString()

      for (const { uuid, createdAt, adContent, categoryContent } of history.adDetailRows) {
        let { brand } = adContent
        if (brand.length > 50) {
          brand = brand.substring(0, 50) + '...'
        }

        let { brandInfo } = adContent
        if (brandInfo.length > 50) {
          brandInfo = brandInfo.substring(0, 50) + '...'
        }

        const detailRow = {
          uuid,
          createdAt,
          adContent: {
            ...adContent,
            brand,
            brandInfo,
            onThumbUpPress: () => actions.toggleAdThumbUp(detailRow),
            onThumbDownPress: () => actions.toggleAdThumbDown(detailRow),
            onMenuSave: () => actions.toggleSavedAd(detailRow),
            onMenuFlag: () => actions.toggleFlaggedAd(detailRow)
          },
          categoryContent: {
            ...categoryContent,
            onOptIn: () => actions.toggleAdOptIn(detailRow),
            onOptOut: () => actions.toggleAdOptOut(detailRow)
          }
        }

        const index = groupedHistory.findIndex(
          (item) => item.date === flooredDateString)

        if (index === -1) {
          groupedHistory.push({
            uuid: history.uuid,
            date: flooredDateString,
            adDetailRows: [detailRow]
          })
        } else {
          groupedHistory[index].adDetailRows.push(detailRow)
        }
      }
    }

    const hasSavedEntries = groupedHistory.some((adHistoryItem) => {
      return adHistoryItem.adDetailRows.some((row) => row.adContent.savedAd)
    })

    const hasLikedEntries = groupedHistory.some((adHistoryItem) => {
      return adHistoryItem.adDetailRows.some(
        (row) => row.adContent.likeAction === 1)
    })

    return (
      <ModalShowAdsHistory
        onClose={toggleModal}
        adsPerHour={adsData.adsPerHour}
        rows={groupedHistory}
        hasSavedEntries={hasSavedEntries}
        hasLikedEntries={hasLikedEntries}
        totalDays={30}
      />
    )
  }

  return (
    <SettingsPanel deeplinkId='ads'>
      <style.root>
        {renderModal()}
        {renderNeedsUpdateNotice()}
        <PanelHeader
          title={getString('adsTitle')}
          enabled={true}
          showConfig={data.showAdsSettings}
          onShowConfigChange={onShowConfigChange()}
        />
        {data.showAdsSettings ? renderConfig() : renderContent()}
      </style.root>
    </SettingsPanel>
  )
}

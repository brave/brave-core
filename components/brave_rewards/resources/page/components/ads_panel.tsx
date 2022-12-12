/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { getUserType } from '../../shared/lib/user_type'
import { getProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { adsPerHourOptions } from '../../shared/lib/ads_options'

import {
  externalWalletFromExtensionData,
  isExternalWalletProviderAllowed
} from '../../shared/lib/external_wallet'

import {
  SettingsPanel,
  PanelHeader,
  PanelItem,
  MonthDay,
  TokenAmountWithExchange,
  ConfigHeader
} from './settings_panel'

import { ModalShowAdsHistory } from '../../ui/components'
import { PaymentStatusView } from '../../shared/components/payment_status_view'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { ArrowNextIcon } from '../../shared/components/icons/arrow_next_icon'
import { AlertIcon } from './icons/alert_icon'

import * as urls from '../../shared/lib/rewards_urls'
import * as style from './ads_panel.style'

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
    userVersion: state.userVersion
  }))

  const [showModal, setShowModal] = React.useState(false)
  const [showConfig, setShowConfig] = React.useState(false)

  const { adsData } = data

  const canEnable = adsData.adsIsSupported && adsData.adsUIEnabled
  const externalWallet = externalWalletFromExtensionData(data.externalWallet)
  const userType = getUserType(data.userVersion, externalWallet)

  const canConnectAccount = data.externalWalletProviderList.some((provider) => {
    const regionInfo = data.parameters.walletProviderRegions[provider] || null
    return isExternalWalletProviderAllowed(data.currentCountryCode, regionInfo)
  })

  const toggleModal = () => {
    setShowModal(!showModal)
  }

  const onEnabledChange = (enabled: boolean) => {
    actions.onAdsSettingSave('adsEnabled', enabled)
  }

  const settingSelectHandler = (key: string) => {
    return (event: React.FormEvent<HTMLSelectElement>) => {
      actions.onAdsSettingSave(key, Number(event.currentTarget.value) || 0)
    }
  }

  function renderDescription () {
    return (
      <style.description>
        {
          formatMessage(getString('adsDesc'), {
            tags: {
              $1: (content) => (
                <NewTabLink key='link' href={urls.aboutBATURL}>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
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
        code: 'DISABLED',
        name: adsSubdivisionTargeting === 'DISABLED'
          ? getString('adsSubdivisionTargetingDisabled')
          : getString('adsSubdivisionTargetingDisable')
      })

      let automaticallyDetectedName = ''
      for (const item of subdivisions) {
        if (item.code === automaticallyDetectedAdsSubdivisionTargeting) {
          automaticallyDetectedName = item.name
          break
        }
      }

      optionList.unshift({
        code: 'AUTO',
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
        <NewTabLink href='https://support.brave.com/hc/en-us/articles/360026361072-Brave-Ads-FAQ'>
          {getString('adsSubdivisionTargetingLearn')}
        </NewTabLink>
      </>
    )

    return (
      <>
        <PanelItem
          label={getString('adsSubdivisionTargetingTitle')}
          description={description}
        >
          <select value={adsData.adsSubdivisionTargeting} onChange={onChange}>
            {
              optionList.map(({ code, name }) => (
                <option key={code} value={code}>{name}</option>
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
        <PanelItem label={getString('adsPerHour')}>
          <select
            value={adsData.adsPerHour}
            onChange={settingSelectHandler('adsPerHour')}
          >
            {
              adsPerHourOptions.map((n) => (
                <option key={`num-per-hour-${n}`} value={n}>
                  {getString(`adsPerHour${n}`)}
                </option>
              ))
            }
          </select>
        </PanelItem>
        {renderSubdivisionSelect()}
      </>
    )
  }

  function renderTerms () {
    if (!canEnable) {
      return null
    }

    return (
      <style.terms>
        {
          formatMessage(getString('tosAndPp'), {
            placeholders: {
              $1: getString('adsTitle')
            },
            tags: {
              $2: (content) => (
                <NewTabLink key='terms' href={urls.termsOfServiceURL}>
                  {content}
                </NewTabLink>
              ),
              $4: (content) => (
                <NewTabLink key='privacy' href={urls.privacyPolicyURL}>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
      </style.terms>
    )
  }

  function renderDisabled () {
    return (
      <>
        {renderTerms()}
        {renderDescription()}
        <style.disabled>
          {getString('adsDisabledTextOne')}&nbsp;
          {getString('adsDisabledTextTwo')}
        </style.disabled>
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

  function renderNotSupportedNotice () {
    if (adsData.adsIsSupported) {
      return null
    }

    return (
      <style.notSupported>
        <style.notSupportedIcon>
          <AlertIcon />
        </style.notSupportedIcon>
        <div>
          {getString('adsNotSupportedRegion')}
        </div>
      </style.notSupported>
    )
  }

  function renderLimited () {
    if (!canConnectAccount) {
      return (
        <>
          {renderDescription()}
          <style.connectUnavailable>
            <div>
              {getString('connectAccountNoProviders')}
            </div>
            <div>
              <NewTabLink href={urls.supportedWalletRegionsURL}>
                {getString('learnMore')}
              </NewTabLink>
            </div>
          </style.connectUnavailable>
        </>
      )
    }

    const onConnect = () => { actions.onModalConnectOpen() }

    return (
      <>
        {renderDescription()}
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
              {getString('connectAccount')}<ArrowNextIcon />
            </button>
          </style.connectAction>
        </style.connect>
        <style.showHistory>
          <button onClick={toggleModal}>
            {getString('openAdsHistory')}
          </button>
        </style.showHistory>
      </>
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
      <TokenAmountWithExchange
        amount={adsData.adsEarningsThisMonth}
        exchangeRate={data.parameters.rate}
        exchangeCurrency='USD'
      />
    )
  }

  function renderContent () {
    if (!adsData.adsEnabled || !canEnable) {
      return renderDisabled()
    }

    if (userType === 'unconnected') {
      return renderLimited()
    }

    const providerPayoutStatus = () => {
      if (!externalWallet) {
        return 'off'
      }
      return getProviderPayoutStatus(
        data.parameters.payoutStatus,
        externalWallet.provider)
    }

    return (
      <>
        {renderDescription()}
        <style.paymentStatus>
            <PaymentStatusView
              earningsLastMonth={adsData.adsEarningsLastMonth}
              nextPaymentDate={adsData.adsNextPaymentDate}
              providerPayoutStatus={providerPayoutStatus()}
            />
          </style.paymentStatus>
        <PanelItem label={getString('adsCurrentEarnings')}>
          {renderEarnings()}
        </PanelItem>
        <PanelItem label={getString('adsPaymentDate')}>
          <MonthDay date={new Date(adsData.adsNextPaymentDate)} />
        </PanelItem>
        <PanelItem label={getString('adsNotificationsReceived')}>
          {adsData.adsReceivedThisMonth}
        </PanelItem>
        <style.showHistory>
          <button onClick={toggleModal}>
            {getString('openAdsHistory')}
          </button>
        </style.showHistory>
      </>
    )
  }

  function renderModal () {
    if (!showModal) {
      return null
    }

    const groupedHistory: Rewards.AdsHistory[] = []

    for (const history of data.adsHistory) {
      const flooredDate = new Date(history.timestampInMilliseconds)
      flooredDate.setHours(0, 0, 0, 0)
      const flooredDateString = flooredDate.toLocaleDateString()

      for (const detailRow of history.adDetailRows) {
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
    <SettingsPanel>
      <style.root>
        {renderModal()}
        {renderNeedsUpdateNotice()}
        <PanelHeader
          title={getString('adsTitle')}
          enabled={canEnable && adsData.adsEnabled}
          showConfig={showConfig}
          onShowConfigChange={setShowConfig}
          onEnabledChange={canEnable ? onEnabledChange : undefined}
        />
        {showConfig ? renderConfig() : renderContent()}
        {renderNotSupportedNotice()}
      </style.root>
    </SettingsPanel>
  )
}

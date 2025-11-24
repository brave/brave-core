/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'

import { formatString } from '$web-common/formatString'
import { useAppState } from '../../lib/app_model_context'
import { useLocaleContext, usePluralString } from '../../lib/locale_strings'
import { useConnectAccountRouter } from '../../lib/connect_account_router'
import { shouldResetExternalWallet } from '../../../shared/lib/external_wallet'
import { PayoutStatusView } from './payout_status_view'
import { AdsSummary } from './ads_summary'
import { AdsSettingsModal } from './ads_settings_modal'
import { AdsHistoryModal } from './ads_history_modal'
import { ResetExternalWalletCard } from './reset_external_wallet_card'

import batCoinGray from '../../assets/bat_coin_gray_animated.svg'
import batCoinColor from '../../assets/bat_coin_color_animated.svg'

import { style } from './earning_card.style'

export function EarningCard() {
  const connectAccount = useConnectAccountRouter()
  const { getString } = useLocaleContext()

  const externalWallet = useAppState((state) => state.externalWallet)
  const adsInfo = useAppState((state) => state.adsInfo)
  const isBubble = useAppState((state) => state.embedder.isBubble)

  const [showAdDetails, setShowAdDetails] = React.useState(!isBubble)
  const [showAdsSettingsModal, setShowAdsSettingsModal] = React.useState(false)
  const [showAdsHistoryModal, setShowAdsHistoryModal] = React.useState(false)

  let adsReceivedThisMonth = 0
  if (adsInfo) {
    adsReceivedThisMonth = Object.values(
      adsInfo.adTypesReceivedThisMonth,
    ).reduce((prev, current) => prev + current, 0)
  }

  const unconnectedAdsViewedString = usePluralString(
    'unconnectedAdsViewedText',
    adsReceivedThisMonth,
  )

  const connectedAdsViewedString = usePluralString(
    'connectedAdsViewedText',
    adsReceivedThisMonth,
  )

  function toggleAdDetails() {
    setShowAdDetails(!showAdDetails)
  }

  function toggleAdsSettingsModal() {
    setShowAdsSettingsModal(!showAdsSettingsModal)
  }

  function toggleAdsHistoryModal() {
    setShowAdsHistoryModal(!showAdsHistoryModal)
  }

  function renderAdsNav() {
    return (
      <div className='ads-summary-nav'>
        <button onClick={toggleAdsHistoryModal}>
          <Icon name='history' />
          {getString('adsHistoryButtonLabel')}
        </button>
        <button onClick={toggleAdsSettingsModal}>
          <Icon name='settings' />
          {getString('adsSettingsButtonLabel')}
        </button>
      </div>
    )
  }

  function renderLimited() {
    return (
      <div
        className='content-card'
        data-css-scope={style.scope}
      >
        <div className='counter'>
          <img
            alt='BAT'
            src={batCoinGray}
          />
          <div className='counter-text'>
            {unconnectedAdsViewedString
              && formatString(unconnectedAdsViewedString, {
                $1: (content) => (
                  <div className='counter-value'>
                    {content}
                    {renderAdsViewedTooltip()}
                  </div>
                ),
              })}
          </div>
        </div>
        <section className='unconnected'>
          <div className='connect'>
            <div className='connect-text'>
              <div>{getString('connectAccountText')}</div>
              <div className='connect-subtext'>
                {getString('connectAccountSubtext')}
              </div>
            </div>
            <Button
              className='connect-button'
              size='small'
              onClick={connectAccount}
            >
              {getString('connectButtonLabel')}
            </Button>
          </div>
          {renderAdsNav()}
        </section>
      </div>
    )
  }

  function renderAdsViewedTooltip() {
    return (
      <Tooltip
        mode='default'
        className='info'
      >
        <Icon name='info-outline' />
        <div slot='content'>{getString('adsViewedTooltip')}</div>
      </Tooltip>
    )
  }

  function renderEarningsCounter() {
    if (!adsInfo || !connectedAdsViewedString) {
      return
    }
    return formatString(connectedAdsViewedString, {
      $1: (content) => (
        <div className='counter-value'>
          {content}
          {renderAdsViewedTooltip()}
        </div>
      ),
    })
  }

  function renderAdsSummary() {
    if (!adsInfo) {
      return null
    }

    if (!adsInfo.isSupportedRegion) {
      return (
        <div className='warning-box'>
          <Icon name='warning-circle-filled' />
          <div>{getString('adsRegionNotSupportedText')}</div>
        </div>
      )
    }

    if (adsInfo.browserUpgradeRequired) {
      return (
        <div className='warning-box'>
          <Icon name='warning-circle-filled' />
          <div>{getString('adsBrowserUpgradeRequiredText')}</div>
        </div>
      )
    }

    return (
      <section className='ads-summary'>
        <button
          className='ads-summary-title'
          onClick={toggleAdDetails}
        >
          <span>
            {adsInfo
              && formatString(getString('earningsAdsReceivedText'), [
                <span className='value'>{adsReceivedThisMonth}</span>,
              ])}
          </span>
          <Icon name={showAdDetails ? 'carat-up' : 'carat-down'} />
        </button>
        {showAdDetails && (
          <>
            <AdsSummary />
            {renderAdsNav()}
          </>
        )}
      </section>
    )
  }

  function renderConnected() {
    return (
      <div
        className='content-card'
        data-css-scope={style.scope}
      >
        <PayoutStatusView />
        <div className='counter'>
          <img
            alt='BAT'
            src={batCoinColor}
          />
          <div className='counter-text'>{renderEarningsCounter()}</div>
        </div>
        {renderAdsSummary()}
      </div>
    )
  }

  if (externalWallet && shouldResetExternalWallet(externalWallet.provider)) {
    return <ResetExternalWalletCard />
  }

  return (
    <>
      {externalWallet ? renderConnected() : renderLimited()}
      {showAdsHistoryModal ? (
        <AdsHistoryModal onClose={toggleAdsHistoryModal} />
      ) : showAdsSettingsModal ? (
        <AdsSettingsModal onClose={toggleAdsSettingsModal} />
      ) : null}
    </>
  )
}

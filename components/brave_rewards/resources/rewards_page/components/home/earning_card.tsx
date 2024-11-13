/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { formatMessage } from '../../../shared/lib/locale_context'
import { useAppState } from '../../lib/app_model_context'
import { useLocaleContext, usePluralString } from '../../lib/locale_strings'
import { useConnectAccountRouter } from '../../lib/connect_account_router'
import { PayoutStatusView } from './payout_status_view'
import { AdsSummary } from './ads_summary'
import { AdsSettingsModal } from './ads_settings_modal'
import { AdsHistoryModal } from './ads_history_modal'

import batCoinGray from '../../assets/bat_coin_gray_animated.svg'
import batCoinColor from '../../assets/bat_coin_color_animated.svg'

import { style } from './earning_card.style'

export function EarningCard() {
  const connectAccount = useConnectAccountRouter()
  const { getString } = useLocaleContext()

  const [externalWallet, adsInfo, isBubble] = useAppState((state) => [
    state.externalWallet,
    state.adsInfo,
    state.embedder.isBubble
  ])

  const [showAdDetails, setShowAdDetails] = React.useState(!isBubble)
  const [showAdsSettingsModal, setShowAdsSettingsModal] = React.useState(false)
  const [showAdsHistoryModal, setShowAdsHistoryModal] = React.useState(false)

  const unconnectedAdsViewedString = usePluralString(
    'unconnectedAdsViewedText',
    adsInfo?.adsReceivedThisMonth)

  const connectedAdsViewedString = usePluralString(
    'connectedAdsViewedText',
    adsInfo?.adsReceivedThisMonth)

  function toggleAdDetails() {
    setShowAdDetails(!showAdDetails)
  }

  function toggleAdsSettingsModal() {
    setShowAdsSettingsModal(!showAdsSettingsModal)
  }

  function toggleAdsHistoryModal() {
    setShowAdsHistoryModal(!showAdsHistoryModal)
  }

  function renderLimited() {
    return (
      <div className='content-card' {...style}>
        <div className='counter'>
          <img alt='BAT' src={batCoinGray} />
          <div className='counter-text'>
            {
              formatMessage(unconnectedAdsViewedString, {
                tags: {
                  $1: (content) => (
                    <div key='value' className='counter-value'>
                      {content}
                    </div>
                  )
                }
              })
            }
          </div>
        </div>
        <section className='connect'>
          <div className='connect-text'>
            <div>
              {getString('connectAccountText')}
            </div>
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
        </section>
      </div>
    )
  }

  function renderEarningsCounter() {
    if (!adsInfo) {
      return
    }
    return formatMessage(connectedAdsViewedString, {
      tags: {
        $1: (content) => (
          <div key='value' className='counter-value'>
            {content}
          </div>
        )
      }
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
        <button className='ads-summary-title' onClick={toggleAdDetails}>
          <span>
            {
              adsInfo && formatMessage(getString('earningsAdsReceivedText'), [
                <span key='value' className='value'>
                  {adsInfo.adsReceivedThisMonth}
                </span>
              ])
            }
          </span>
          <Icon name={showAdDetails ? 'carat-up' : 'carat-down'} />
        </button>
        {
          showAdDetails && <>
            <AdsSummary />
            <div className='ads-summary-nav'>
              <button onClick={toggleAdsHistoryModal}>
                <Icon name='history' />{getString('adsHistoryButtonLabel')}
              </button>
              <button onClick={toggleAdsSettingsModal}>
                <Icon name='settings' />{getString('adsSettingsButtonLabel')}
              </button>
            </div>
          </>
        }
      </section>
    )
  }

  if (!externalWallet) {
    return renderLimited()
  }

  return <>
    <div className='content-card' {...style}>
      <PayoutStatusView />
      <div className='counter'>
        <img alt='BAT' src={batCoinColor} />
        <div className='counter-text'>
          {renderEarningsCounter()}
        </div>
      </div>
      {renderAdsSummary()}
    </div>
    {
      showAdsHistoryModal ?
        <AdsHistoryModal onClose={toggleAdsHistoryModal} /> :
      showAdsSettingsModal ?
        <AdsSettingsModal onClose={toggleAdsSettingsModal} /> : null
    }
  </>
}

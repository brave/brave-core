/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { formatMessage } from '../../../shared/lib/locale_context'
import { useAppState } from '../../lib/app_model_context'
import { useLocaleContext, usePluralString } from '../../lib/locale_strings'
import batCoinGray from '../../assets/bat_coin_gray_animated.svg'

import { style } from './earning_card.style'

export function EarningCard() {
  const { getString } = useLocaleContext()

  const [externalWallet, adsInfo] = useAppState((state) => [
    state.externalWallet,
    state.adsInfo
  ])

  const adsViewedString = usePluralString(
    'unconnectedAdsViewedText',
    adsInfo?.adsReceivedThisMonth)

  function renderLimited() {
    return (
      <div className='content-card' {...style}>
        <div className='counter'>
          <img src={batCoinGray} />
          <div className='counter-text'>
            {
              formatMessage(adsViewedString, {
                tags: {
                  $1: (content) => (
                    <div key='ads-viewed' className='counter-value'>
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
          <Button size='small'>
            {getString('connectButtonLabel')}
          </Button>
        </section>
      </div>
    )
  }

  if (!externalWallet) {
    return renderLimited()
  }

  return (
    <></>
  )
}

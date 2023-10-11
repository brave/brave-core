/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import formatMessage from '$web-common/formatMessage'
import getPageHandlerInstance from '../../api/page_handler'

import styles from './style.module.scss'

interface PremiumSuggestionProps {
  title?: string
  description?: string
  secondaryActionButton?: React.ReactNode
  verbose?: boolean
}

function PremiumSuggestion(props: PremiumSuggestionProps) {
  const pricingInfo = formatMessage(getLocale('premiumPricing'), {
    placeholders: {
      $1: <data>15</data>
    }
  })

  const handlePremiumButtonClick = () => {
    // TODO(petemill): Build the url in the browser
    getPageHandlerInstance().pageHandler.openURL({url: 'https://account.brave.com/account/?intent=checkout&product=vpn' })
  }

  return (
    <div className={styles.boxPremium}>
      <h4>{props.title}</h4>
      <p>{props.description}</p>
      <ul className={styles.featuresListing}>
        <li>
          <Icon name='check-normal' />
          <span>
            {getLocale('premiumFeature_1')}
            {props?.verbose && <p>{getLocale('premiumFeature_1_desc')}</p>}
          </span>
        </li>
        <li>
          <Icon name='check-normal' />
          <span>
            {getLocale('premiumFeature_2')}
            {props?.verbose && (
              <p>{getLocale('premiumFeature_2_desc')}</p>
            )}
          </span>
        </li>
        <li className={styles.priceList}>
          <span>{getLocale('premiumLabel')}</span>
          <span className={styles.price}>
            {pricingInfo}
          </span>
        </li>
      </ul>
      <div className={styles.actionsBox}>
        <Button onClick={handlePremiumButtonClick}>
          {getLocale('premiumButtonLabel')}
        </Button>
        {props.secondaryActionButton}
      </div>
    </div>
  )
}

export default PremiumSuggestion

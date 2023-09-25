/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import styles from './style.module.scss'

interface ErrorRateLimit {
  onRetry?: () => void
  onDismiss?: () => void
  isDismissable?: boolean
  title?: string
  description?: string
}

function ErrorRateLimitPremium(props: ErrorRateLimit) {
  const pricing = getLocale('premiumPricing').split('/')

  return (
    <div className={styles.boxPremium}>
      <h4>{props.title}</h4>
      <p>{props.description}</p>
      <ul className={styles.featuresListing}>
        <li>
          <Icon name='check-normal' />
          <span>{getLocale('premiumFeature_1')}</span>
        </li>
        <li>
          <Icon name='check-normal' />
          <span>{getLocale('premiumFeature_2')}</span>
        </li>
        <li className={styles.priceList}>
          <span>{getLocale('premiumLabel')}</span>
          <span className={styles.price}>
            {pricing[0]}
            <data>{pricing[1]}</data>
            {pricing[2]}
          </span>
        </li>
      </ul>
      <div className={styles.actionsBox}>
        <Button onClick={props.onRetry}>
          {getLocale('premiumButtonLabel')}
        </Button>
        <Button
          kind='plain-faint'
          onClick={props.isDismissable ? props.onDismiss : props.onRetry}
        >
          {props.isDismissable ? getLocale('dismissButtonLabel') : getLocale('retryButtonLabel')}
        </Button>
      </div>
    </div>
  )
}

export default ErrorRateLimitPremium

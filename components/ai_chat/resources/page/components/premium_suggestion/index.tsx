/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import formatMessage from '$web-common/formatMessage'
import DataContext from '../../state/context'
import styles from './style.module.scss'

interface PremiumSuggestionProps {
  title?: string
  description?: string
  secondaryActionButton?: React.ReactNode
}

const featuresList = [
  {
    title: getLocale('premiumFeature_1'),
    desc: getLocale('premiumFeature_1_desc'),
    icon: 'widget-generic'
  },
  {
    title: getLocale('premiumFeature_2'),
    desc: getLocale('premiumFeature_2_desc'),
    icon: 'idea'
  },
  {
    title: getLocale('premiumFeature_3'),
    desc: getLocale('premiumFeature_3_desc'),
    icon: 'edit-pencil'
  },
  {
    title: getLocale('premiumFeature_4'),
    desc: getLocale('premiumFeature_4_desc'),
    icon: 'message-bubble-comments'
  }
]

function PremiumSuggestion(props: PremiumSuggestionProps) {
  const context = React.useContext(DataContext)

  const pricingInfo = formatMessage(getLocale('premiumPricing'), {
    placeholders: {
      $1: <data>15</data>
    }
  })

  return (
    <div className={styles.boxPremium}>
      <h4>{props.title}</h4>
      <p>{props.description}</p>
      <ul className={styles.featuresListing}>
        {featuresList.map((entry) => {
          return (
            <li>
              <div className={styles.icon}>
                <Icon name={entry.icon} />
              </div>
              <span>
                {entry.title}
                <p>{entry.desc}</p>
              </span>
            </li>
          )
        })}
      </ul>
      <div className={styles.priceList}>
        <button className={styles.priceButton} tabIndex={-1}>
          <span className={styles.priceButtonLabel}>Monthly</span>
          <span className={styles.price}>{pricingInfo}</span>
        </button>
        <div className={styles.subscriptionPolicy}>
          {getLocale('subscriptionPolicyInfo')}
        </div>
      </div>
      <div className={styles.actionsBox}>
        <Button onClick={context.goPremium}>
          {getLocale('upgradeButtonLabel')}
        </Button>
        {props.secondaryActionButton}
      </div>
    </div>
  )
}

export default PremiumSuggestion

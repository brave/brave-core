/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import Label from '@brave/leo/react/label'
import classnames from '$web-common/classnames'
import { getLocale, formatLocale } from '$web-common/locale'
import { useAIChat } from '../../state/ai_chat_context'
import styles from './style.module.scss'

interface PremiumSuggestionProps {
  title?: string
  description?: string
  secondaryActionButton?: React.ReactNode
}

const featuresList = [
  {
    title: getLocale(S.CHAT_UI_PREMIUM_FEATURE_1),
    desc: getLocale(S.CHAT_UI_PREMIUM_FEATURE_1_DESC),
    icon: 'widget-generic'
  },
  {
    title: getLocale(S.CHAT_UI_PREMIUM_FEATURE_2),
    desc: getLocale(S.CHAT_UI_PREMIUM_FEATURE_2_DESC),
    icon: 'idea'
  },
  {
    title: getLocale(S.CHAT_UI_PREMIUM_FEATURE_3),
    desc: getLocale(S.CHAT_UI_PREMIUM_FEATURE_3_DESC),
    icon: 'edit-pencil'
  },
  {
    title: getLocale(S.CHAT_UI_PREMIUM_FEATURE_4),
    desc: getLocale(S.CHAT_UI_PREMIUM_FEATURE_4_DESC),
    icon: 'message-bubble-comments'
  }
]

function PremiumSuggestion(props: PremiumSuggestionProps) {
  const aiChatContext = useAIChat()
  const buttonRef = React.useRef<HTMLButtonElement>()

  const pricingInfo = formatLocale(S.CHAT_UI_PREMIUM_PRICING, {
    $1: <data>14.99</data>
  })

  const pricingAnnualInfo = formatLocale(S.CHAT_UI_PREMIUM_ANNUAL_PRICING, {
    $1: <data>149.99</data>
  })

  React.useEffect(() => {
    if (buttonRef.current === undefined) return
    buttonRef.current.scrollIntoView({ behavior: 'smooth' })
  }, [])

  return (
    <div className={styles.boxPremium}>
      <div className={styles.header}>
        <h4>{props.title}</h4>
        <p>{props.description}</p>
      </div>
      <div className={styles.features}>
        <ul>
          {featuresList.map((entry, i) => {
            return <li key={i}>
              <div className={styles.icon}>
                <Icon name={entry.icon} />
              </div>
              <span>
                {entry.title}
                <p>{entry.desc}</p>
              </span>
            </li>
          })}
        </ul>
      </div>
      {!aiChatContext.isMobile && (
        <div className={styles.priceListWrapper}>
          <div className={styles.priceList}>
            <button className={styles.priceButton} tabIndex={-1}>
              <div className={styles.bestValueColumn}>
                <span className={styles.priceButtonLabel}>{getLocale(S.CHAT_UI_ONE_YEAR_LABEL)}</span>
                <Label color='green'>{getLocale(S.CHAT_UI_BEST_VALUE_LABEL)}</Label>
              </div>
              <span className={styles.price}>{pricingAnnualInfo}</span>
            </button>
            <button className={classnames(styles.priceButton, styles.priceButtonMonthly)} tabIndex={-1}>
              <span className={styles.priceButtonLabel}>{getLocale(S.CHAT_UI_MONTHLY_LABEL)}</span>
              <span className={styles.price}>{pricingInfo}</span>
            </button>
          </div>
          <div className={styles.subscriptionPolicy}>
            {getLocale(S.CHAT_UI_SUBSCRIPTION_POLICY_INFO)}
          </div>
        </div>
      )}
      <div className={styles.actions}>
        <Button onClick={aiChatContext.goPremium} ref={buttonRef}>
          {getLocale(S.CHAT_UI_UPGRADE_BUTTON_LABEL)}
        </Button>
        {props.secondaryActionButton}
      </div>
    </div>
  )
}

export default PremiumSuggestion

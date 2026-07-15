/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import Label from '@brave/leo/react/label'
import { getLocale, formatLocale } from '$web-common/locale'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import styles from './style.module.scss'

interface PremiumSuggestionProps {
  title?: string
  description?: string
  secondaryActionButton?: React.ReactNode
}

const sellingPoints = [
  getLocale(S.CHAT_UI_PREMIUM_FEATURE_1),
  getLocale(S.CHAT_UI_PREMIUM_FEATURE_2),
  getLocale(S.CHAT_UI_PREMIUM_FEATURE_3),
]

function PremiumSuggestion(props: PremiumSuggestionProps) {
  const context = useUntrustedConversationContext()
  const buttonRef = React.useRef<HTMLButtonElement>(null)

  const pricingInfo = formatLocale(S.CHAT_UI_PREMIUM_PRICING, {
    $1: <data>14.99</data>,
  })

  const pricingAnnualInfo = formatLocale(S.CHAT_UI_PREMIUM_ANNUAL_PRICING, {
    $1: <data>149.99</data>,
  })

  React.useEffect(() => {
    if (!buttonRef.current) return
    buttonRef.current.scrollIntoView({ behavior: 'smooth' })
  }, [])

  const isMobile = context.isMobile

  return (
    <div className={styles.boxPremium}>
      <div className={styles.layout}>
        <div className={styles.left}>
          <div className={styles.header}>
            <h4>{props.title}</h4>
            {props.description && <p>{props.description}</p>}
          </div>
          <ul className={styles.sellingPoints}>
            {sellingPoints.map((point, i) => {
              return (
                <li
                  key={i}
                  className={styles.sellingPoint}
                >
                  <Icon
                    className={styles.checkIcon}
                    name='check-normal'
                  />
                  <span>{point}</span>
                </li>
              )
            })}
          </ul>
        </div>
        <div className={styles.right}>
          {!isMobile && (
            <div className={styles.priceListWrapper}>
              <div className={styles.planCard}>
                <div className={styles.planDetails}>
                  <div className={styles.planHeader}>
                    <span className={styles.planName}>
                      {getLocale(S.CHAT_UI_PREMIUM_ANNUAL_LABEL)}
                    </span>
                    <span className={styles.planPrice}>
                      {pricingAnnualInfo}
                    </span>
                  </div>
                </div>
                <Label
                  className={styles.saveLabel}
                  mode='loud'
                  color='primary'
                >
                  {getLocale(S.CHAT_UI_PREMIUM_SAVE_PERCENT_LABEL)}
                </Label>
              </div>
              <div className={styles.planCard}>
                <div className={styles.planDetails}>
                  <div className={styles.planHeader}>
                    <span className={styles.planName}>
                      {getLocale(S.CHAT_UI_MONTHLY_LABEL)}
                    </span>
                    <span className={styles.planPrice}>{pricingInfo}</span>
                  </div>
                </div>
              </div>
              <div className={styles.subscriptionPolicy}>
                {getLocale(S.CHAT_UI_SUBSCRIPTION_POLICY_INFO)}
              </div>
            </div>
          )}
          <div className={styles.actions}>
            <Button
              onClick={() => context.uiHandler.goPremium()}
              ref={buttonRef}
            >
              {getLocale(S.CHAT_UI_UPGRADE_BUTTON_LABEL)}
            </Button>
            {props.secondaryActionButton}
          </div>
        </div>
      </div>
    </div>
  )
}

export default PremiumSuggestion

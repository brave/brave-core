// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import backgroundImage from '../assets/agent_ntp_background.jpg'
import styles from './style.module.scss'

const cards = [
  {
    title: getLocale(S.AI_AGENT_NTP_ABOUT_THE_LEO_PROFILE_CARD_TITLE),
    description: getLocale(
      S.AI_AGENT_NTP_ABOUT_THE_LEO_PROFILE_CARD_DESCRIPTION,
    ),
    icon: 'leo-cursor',
  },
  {
    title: getLocale(S.AI_AGENT_NTP_YOUR_DATA_AND_SESSIONS_CARD_TITLE),
    description: getLocale(
      S.AI_AGENT_NTP_YOUR_DATA_AND_SESSIONS_CARD_DESCRIPTION,
    ),
    icon: 'key',
  },
  {
    title: getLocale(S.AI_AGENT_NTP_SWITCHING_BACK_CARD_TITLE),
    description: getLocale(S.AI_AGENT_NTP_SWITCHING_BACK_CARD_DESCRIPTION),
    icon: 'product-brave-leo',
  },
]

export function App() {
  return (
    <>
      <img
        src={backgroundImage}
        alt=''
        className={styles.imageBackground}
      />
      <div
        className={styles.content}
        data-theme='dark'
      >
        <div className={styles.header}>
          <a
            className={styles.agentModeLink}
            href='https://support.brave.app/hc/en-us/articles/41240379376909'
            target='_blank'
            rel='noopener noreferrer'
          >
            <Icon name='leo-cursor-filled' />
            {getLocale(S.AI_AGENT_NTP_AGENT_MODE_BUTTON_LABEL)}
          </a>
        </div>

        <div className={styles.cardsContainer}>
          {cards.map((card) => (
            <div
              className={styles.card}
              key={card.title}
            >
              <div className={styles.cardHeader}>
                <div className={styles.cardIcon}>
                  <Icon name={card.icon} />
                </div>
                <div className={styles.cardTitle}>{card.title}</div>
              </div>
              <div className={styles.cardDescription}>{card.description}</div>
            </div>
          ))}
        </div>
      </div>
    </>
  )
}

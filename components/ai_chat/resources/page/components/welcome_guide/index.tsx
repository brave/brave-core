/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'
import { useConversation } from '../../state/conversation_context'
import styles from './style.module.scss'

function WelcomeGuide() {
  const conversationContext = useConversation()

  const summarizeNow = () => {
    conversationContext.conversationHandler?.submitSummarizationRequest()
  }

  return (
    <div className={styles.box}>
      <div className={styles.header}>
        <h1 className={styles.title}>{getLocale(S.CHAT_UI_WELCOME_GUIDE_TITLE)}</h1>
        <h2 className={styles.subtitle}>{getLocale(S.CHAT_UI_WELCOME_GUIDE_SUBTITLE)}</h2>
      </div>
      <div className={`${styles.card} ${styles.siteHelpCard}`}>
        <h4 className={styles.cardTitle}>
          {getLocale(S.CHAT_UI_WELCOME_GUIDE_SITE_HELP_CARD_TITLE)}
        </h4>
        {conversationContext.associatedContentInfo.length > 0 ? (
          <>
            <p>{getLocale(S.CHAT_UI_WELCOME_GUIDE_SITE_HELP_CARD_WITH_ACTION)}</p>
            <div className={styles.actions}>
              <Button
                kind='outline'
                onClick={summarizeNow}
              >
                {getLocale(S.CHAT_UI_SUMMARIZE_BUTTON_LABEL)}
              </Button>
            </div>
          </>
        ) : (
          <p>{getLocale(S.CHAT_UI_WELCOME_GUIDE_SITE_HELP_CARD_DESC)}</p>
        )}
      </div>
      <div className={`${styles.card} ${styles.chatCard}`}>
        <h4 className={styles.cardTitle}>
          {getLocale(S.CHAT_UI_WELCOME_GUIDE_CHAT_CARD_TITLE)}
        </h4>
        <p>{getLocale(S.CHAT_UI_WELCOME_GUIDE_CHAT_CARD_DESC)}</p>
      </div>
    </div>
  )
}

export default WelcomeGuide

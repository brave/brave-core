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
        <h1 className={styles.title}>{getLocale(StringIds.WelcomeGuideTitle)}</h1>
        <h2 className={styles.subtitle}>{getLocale(StringIds.WelcomeGuideSubtitle)}</h2>
      </div>
      <div className={`${styles.card} ${styles.siteHelpCard}`}>
        <h4 className={styles.cardTitle}>
          {getLocale(StringIds.WelcomeGuideSiteHelpCardTitle)}
        </h4>
        {conversationContext.associatedContentInfo &&
        conversationContext.shouldSendPageContents ? (
          <>
            <p>{getLocale(StringIds.WelcomeGuideSiteHelpCardWithAction)}</p>
            <div className={styles.actions}>
              <Button
                kind='outline'
                onClick={summarizeNow}
              >
                {getLocale(StringIds.SummarizeButtonLabel)}
              </Button>
            </div>
          </>
        ) : (
          <p>{getLocale(StringIds.WelcomeGuideSiteHelpCardDesc)}</p>
        )}
      </div>
      <div className={`${styles.card} ${styles.chatCard}`}>
        <h4 className={styles.cardTitle}>
          {getLocale(StringIds.WelcomeGuideChatCardTitle)}
        </h4>
        <p>{getLocale(StringIds.WelcomeGuideChatCardDesc)}</p>
      </div>
    </div>
  )
}

export default WelcomeGuide

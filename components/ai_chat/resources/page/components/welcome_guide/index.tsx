/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '$web-common/locale'
import Button from '@brave/leo/react/button'

import styles from './style.module.scss'
import getPageHandlerInstance from '../../api/page_handler'
import DataContext from '../../state/context'

function WelcomeGuide() {
  const context = React.useContext(DataContext)

  const summarizeNow = () => {
    getPageHandlerInstance().pageHandler.submitSummarizationRequest()
  }

  return (
    <div className={styles.box}>
      <div className={styles.header}>
        <h1 className={styles.title}>{getLocale('welcomeGuideTitle')}</h1>
        <h2 className={styles.subtitle}>{getLocale('welcomeGuideSubtitle')}</h2>
      </div>
      <div className={`${styles.card} ${styles.siteHelpCard}`}>
        <h4 className={styles.cardTitle}>
          {getLocale('welcomeGuideSiteHelpCardTitle')}
        </h4>
        {context.siteInfo?.isContentAssociationPossible &&
        context.shouldSendPageContents ? (
          <>
            <p>{getLocale('welcomeGuideSiteHelpCardDescWithAction')}</p>
            <div className={styles.actions}>
              <Button
                kind='outline'
                onClick={summarizeNow}
              >
                {getLocale('summarizePageButtonLabel')}
              </Button>
            </div>
          </>
        ) : (
          <p>{getLocale('welcomeGuideSiteHelpCardDesc')}</p>
        )}
      </div>
      <div className={`${styles.card} ${styles.chatCard}`}>
        <h4 className={styles.cardTitle}>
          {getLocale('welcomeGuideShatCardTitle')}
        </h4>
        <p>{getLocale('welcomeGuideShatCardDesc')}</p>
      </div>
    </div>
  )
}

export default WelcomeGuide

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

interface MainProps {
  conversationList: React.ReactNode
  inputBox: React.ReactNode
  siteTitle?: React.ReactNode
  promptAutoSuggestion?: React.ReactNode
  onSettingsClick?: () => void
  onEraseClick?: () => void
}

function Main (props: MainProps) {
  return (
    <main className={styles.main}>
      <div className={styles.header}>
        <div className={styles.logo}>
          <Icon name="product-brave-ai" />
          <div className={styles.logoTitle}>Brave <span>Leo</span></div>
        </div>
        <div className={styles.actions}>
          <Button kind="plain-faint" aria-label="Erase conversation history" title="Erase conversation history" onClick={props.onEraseClick}>
              <Icon name="erase" />
          </Button>
          <Button kind="plain-faint" aria-label="Settings" title="Settings" onClick={props.onSettingsClick}>
              <Icon name="settings" />
          </Button>
        </div>
      </div>
      <div className={styles.scroller}>
        {props.siteTitle && (
          <div className={styles.siteTitleBox}>
            {props.siteTitle}
          </div>
        )}
        {props.conversationList}
      </div>
      <div className={styles.inputBox}>
        {props.promptAutoSuggestion}
        {props.inputBox}
      </div>
    </main>
  )
}

export default Main

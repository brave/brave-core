/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import ConversationTile from '../conversation_tile'
import Icon from '@brave/leo/react/icon'

interface MainProps {
  activeConversation: React.ReactNode
  inputBox: React.ReactNode
}


function Main (props: MainProps) {
  return (
    <main className={styles.main}>
      <aside className={styles.aside}>
        <header className={styles.header}>
          <button className={styles.buttonSettings}>
            <Icon name='settings' />
          </button>
        </header>
        <div className={styles.scroller}>
          {Array(15).fill(1).map((value, idx) => {
            return (
              <ConversationTile
                title="marvel movie"
                date={new Date().toLocaleDateString()}
                isSelected={idx === 0}
              />
            )
          })}
        </div>
      </aside>
      <div className={styles.conversationBox}>
        <div className={styles.scroller}>
          {props.activeConversation}
        </div>
        <div className={styles.inputBox}>
          {props.inputBox}
        </div>
      </div>
    </main>
  )
}

export default Main

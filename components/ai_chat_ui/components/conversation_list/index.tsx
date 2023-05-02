/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from 'classnames'

import styles from './style.module.scss'
import { ConversationTurn, CharacterType } from '../../api/page_handler'
import { getLocale } from '$web-common/locale'

interface ConversationListProps {
  list: ConversationTurn[]
  isLoading: boolean
}

function ConversationList (props: ConversationListProps) {
  return (
    <div className={styles.list}>
      {props.list.map((turn, id) => {
        const turnClass = classnames({
          [styles.turnAI]: turn.characterType === CharacterType.ASSISTANT,
          [styles.turnHuman]: turn.characterType === CharacterType.HUMAN,
        })

        return (
          <div key={id} className={turnClass}>
            <p>
              {turn.text}
            </p>
          </div>
        )
      })}
      {props.isLoading && (
        <div className={styles.turnAI}>
          <p>
            {getLocale('loadingLabel')}
            <span className={styles.caret}/>
          </p>
        </div>
      )}
    </div>
  )
}

export default ConversationList

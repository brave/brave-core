/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import DataContext from '../../state/context'
import styles from './style.module.scss'

function PromptLongArticle() {
  const context = React.useContext(DataContext)

  return (
    <div className={styles.common}>
      <div className={styles.box}>
        <Icon name='warning-triangle-filled' className={styles.icon} />
        <div>
          <p>{getLocale('pageContentTooLongWarning')}</p>
          <div className={styles.actionsBox}>
            <Button onClick={() =>  context.dismissArticleLongPrompt()}>
              {getLocale('gotItButtonLabel')}
            </Button>
          </div>
        </div>
      </div>
    </div>
  )
}

export default PromptLongArticle

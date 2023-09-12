/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import { getLocale } from '$web-common/locale'

import styles from './style.module.scss'
import DataContext from '../../state/context'

function PromptAutoSuggestion () {
  const { setUserAllowsAutoGenerating, generateSuggestedQuestions } = React.useContext(DataContext)

  const handleOnEnableAutoGenerateQuestion = () => {
    setUserAllowsAutoGenerating(true)
    generateSuggestedQuestions()
  }

  const handleOnDismiss = () => {
    setUserAllowsAutoGenerating(false)
  }

  return (
    <div className={styles.box}>
      <Icon name="product-brave-ai" className={styles.icon} />
      <h1>{getLocale('enableQuestionsTitle')}</h1>
      <p>{getLocale('enableQuestionsDesc')}</p>
      <div className={styles.actionsBox}>
        <Button onClick={handleOnEnableAutoGenerateQuestion}>
          {getLocale('enableQuestionsButtonLabel')}
        </Button>
        <Button kind="plain-faint" onClick={handleOnDismiss}>
          {getLocale('noThanksButtonLabel')}
        </Button>
      </div>
    </div>
  )
}

export default PromptAutoSuggestion

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import classnames from 'classnames'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'

import styles from './style.module.scss'
import DataContext from '../../state/context'
import getPageHandlerInstance, { APIError } from '../../api/page_handler'

const MAX_INPUT_CHAR = 2000
const CHAR_LIMIT_THRESHOLD = MAX_INPUT_CHAR * 0.80

function InputBox () {
  const [inputText, setInputText] = React.useState('')
  const { currentError, isGenerating, hasSeenAgreement, handleAgreeClick } = React.useContext(DataContext)

  const isCharLimitExceeded = inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching = inputText.length >= CHAR_LIMIT_THRESHOLD

  const apiHasError = !!currentError && (currentError !== APIError.None)
  const isInputDisabled = apiHasError || isGenerating || isCharLimitExceeded

  if (!hasSeenAgreement) {
    return (
      <div className={styles.container}>
        <button className={styles.buttonAgree} onClick={handleAgreeClick}>{getLocale('acceptButtonLabel')}</button>
      </div>
    )
  }

  const onInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    setInputText(e.target.value)
  }

  const submitInputTextToAPI = () => {
    getPageHandlerInstance().pageHandler.submitHumanConversationEntry(inputText)
    setInputText('')
  }

  const handleSubmit = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
    if (!inputText) return
    if (isCharLimitExceeded) return

    e.preventDefault()
    submitInputTextToAPI()
  }

  const onUserPressEnter = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter') {
      if (!e.repeat) {
        submitInputTextToAPI()
      }

      e.preventDefault()
    }
  }

  return (
    <form className={styles.form}>
      <div className={styles.textareaBox}>
        <textarea
          className={styles.textarea}
          placeholder={getLocale('placeholderLabel')}
          onChange={onInputChange}
          onKeyDown={onUserPressEnter}
          value={inputText}
          disabled={isInputDisabled}
          autoFocus
        />
        <div className={classnames({
          [styles.counterText]: true,
          [styles.counterTextVisible]: isCharLimitApproaching,
          [styles.counterTextError]: isCharLimitExceeded
        })}>
          {`${inputText.length} / ${MAX_INPUT_CHAR}`}
        </div>
      </div>
      <div>
        <button
          className={styles.buttonSend}
          onClick={handleSubmit}
          disabled={isInputDisabled}
        >
          <Icon name='send' />
        </button>
      </div>
    </form>
  )
}

export default InputBox

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '$web-common/locale'
import styles from './style.module.scss'
import Icon from '@brave/leo/react/icon'
import classnames from 'classnames'

interface InputBoxProps {
  onInputChange?: (e: React.ChangeEvent<HTMLTextAreaElement>) => void
  onSubmit?: (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => void
  onSummaryClick?: (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => void
  onKeyDown?: (e: React.KeyboardEvent<HTMLTextAreaElement>) => void
  value: string
  hasSeenAgreement: boolean
  onHandleAgreeClick: Function
}

const MAX_INPUT_CHAR = 340000 * 0.80
const CHAR_LIMIT_THRESHOLD = MAX_INPUT_CHAR * 0.80

function InputBox (props: InputBoxProps) {
  const [inputText, setInputText] = React.useState(props.value)

  const isCharLimitExceeded = inputText.length >= MAX_INPUT_CHAR
  const isCharLimitApproaching = inputText.length >= CHAR_LIMIT_THRESHOLD

  if (!props.hasSeenAgreement) {
    const handleAgreeClick = () => {
      props.onHandleAgreeClick()
    }

    return (
      <div className={styles.container}>
        <button className={styles.buttonAgree} onClick={handleAgreeClick}>{getLocale('acceptButtonLabel')}</button>
      </div>
    )
  }

  const handleInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    setInputText(e.target.value)
    props.onInputChange?.(e)
  }

  const handleClick = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
    if (isCharLimitExceeded) return
    props.onSubmit?.(e)
    setInputText('')
  }

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter') {
      if (!e.repeat) {
        if (isCharLimitExceeded) return
        props.onKeyDown?.(e)
        setInputText('')
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
          onChange={handleInputChange}
          onKeyDown={handleKeyDown}
          value={inputText}
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
        <button className={styles.buttonSend} onClick={handleClick} disabled={isCharLimitExceeded}>
          <Icon name='send' />
        </button>
      </div>
    </form>
  )
}

export default InputBox

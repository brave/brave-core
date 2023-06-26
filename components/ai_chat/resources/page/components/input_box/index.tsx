/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '$web-common/locale'
import styles from './style.module.scss'
import Icon from '@brave/leo/react/icon'

interface InputBoxProps {
  onInputChange?: (e: React.ChangeEvent<HTMLTextAreaElement>) => void
  onSubmit?: (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => void
  onSummaryClick?: (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => void
  onKeyDown?: (e: React.KeyboardEvent<HTMLTextAreaElement>) => void
  value: string
  hasSeenAgreement: boolean
  onHandleAgreeClick: Function
}

function InputBox (props: InputBoxProps) {
  if (!props.hasSeenAgreement) {
    const handleAgreeClick = () => {
      props.onHandleAgreeClick()
    }

    return (
      <div className={styles.container}>
        <button className={styles.buttonPrimary} onClick={handleAgreeClick}>{getLocale('acceptButtonLabel')}</button>
      </div>
    )
  }

  const handleInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    props.onInputChange?.(e)
  }

  const handleClick = (e: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
    props.onSubmit?.(e)
  }

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter') {
      if (!e.repeat) {
        props.onKeyDown?.(e)
      }

      e.preventDefault()
    }
  }

  return (
    <div className={styles.container}>
      <form className={styles.form}>
        <textarea
          className={styles.textbox}
          placeholder={getLocale('placeholderLabel')}
          onChange={handleInputChange}
          onKeyDown={handleKeyDown}
          value={props.value}
        />
        <div>
          <button className={styles.buttonSend} onClick={handleClick}>
            <Icon name='send' />
          </button>
        </div>
      </form>
    </div>
  )
}

export default InputBox

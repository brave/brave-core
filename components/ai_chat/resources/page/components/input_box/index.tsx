/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import styles from './style.module.scss'
import DataContext from '../../state/context'
import getPageHandlerInstance from '../../api/page_handler'

function InputBox () {
  const context = React.useContext(DataContext)

  const onInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    context.setInputText(e.target.value)
  }

  const handleSubmit = (e: CustomEvent<any>) => {
    e.preventDefault()
    context.submitInputTextToAPI()
  }

  const handleMic = (e: CustomEvent<any>) => {
    e.preventDefault()
    getPageHandlerInstance().pageHandler.handleVoiceRecognition()
  }

  const onUserPressEnter = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey && !e.nativeEvent.isComposing) {
      if (!e.repeat) {
        context.submitInputTextToAPI()
      }

      e.preventDefault()
    }
  }

  return (
    <form className={styles.form}>
      <div
        className={(context.isMobile ? styles.growWrapMobile : styles.growWrap)}
        data-replicated-value={context.inputText}
      >
        <textarea
          placeholder={getLocale('placeholderLabel')}
          onChange={onInputChange}
          onKeyDown={onUserPressEnter}
          value={context.inputText}
          autoFocus
          rows={1}
        />
      </div>
      {context.isCharLimitApproaching && (
        <div className={classnames({
          [styles.counterText]: true,
          [styles.counterTextVisible]: context.isCharLimitApproaching,
          [styles.counterTextError]: context.isCharLimitExceeded
        })}>
          {context.inputTextCharCountDisplay}
        </div>
      )}
      <div className={styles.actions}>
        {context.isMobile && <Button
          kind="plain-faint"
          onClick={handleMic}
          disabled={context.shouldDisableUserInput}
          >
          <Icon name='microphone' />
        </Button>}
        <Button
          kind="plain-faint"
          onClick={handleSubmit}
          disabled={context.shouldDisableUserInput}
          title={getLocale('sendChatButtonLabel')}
          >
          <Icon name='send' />
        </Button>
      </div>
    </form>
  )
}

export default InputBox

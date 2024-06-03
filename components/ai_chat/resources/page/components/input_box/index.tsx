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
import ActionTypeLabel from '../action_type_label'

function InputBox() {
  const context = React.useContext(DataContext)

  const onInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    context.setInputText(e.target.value)
  }

  const handleSubmit = (e: PointerEvent) => {
    context.submitInputTextToAPI()
  }

  const handleMic = (e: PointerEvent) => {
    getPageHandlerInstance().pageHandler.handleVoiceRecognition()
  }

  const handleOnKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey && !e.nativeEvent.isComposing) {
      if (!e.repeat) {
        context.submitInputTextToAPI()
      }

      e.preventDefault()
    }

    if (
      e.key === 'Backspace' &&
      context.inputText === '' &&
      context.selectedActionType
    ) {
      context.resetSelectedActionType()
    }
  }

  const maybeAutofocus = (node: HTMLTextAreaElement | null) => {
    if (node && context.selectedActionType) {
      node.focus()
    }
  }

  return (
    <form className={styles.form}>
      {context.selectedActionType && (
        <div className={styles.actionsLabelContainer}>
          <ActionTypeLabel
            removable={true}
            actionType={context.selectedActionType}
            onCloseClick={context.resetSelectedActionType}
          />
        </div>
      )}
      <div
        className={styles.growWrap}
        data-replicated-value={context.inputText}
      >
        <textarea
          ref={maybeAutofocus}
          placeholder={getLocale('placeholderLabel')}
          onChange={onInputChange}
          onKeyDown={handleOnKeyDown}
          value={context.inputText}
          autoFocus
          rows={1}
        />
      </div>
      {context.isCharLimitApproaching && (
        <div
          className={classnames({
            [styles.counterText]: true,
            [styles.counterTextVisible]: context.isCharLimitApproaching,
            [styles.counterTextError]: context.isCharLimitExceeded
          })}
        >
          {context.inputTextCharCountDisplay}
        </div>
      )}
      <div className={styles.toolsContainer}>
        <div className={styles.tools}>
          <Button
            fab
            kind='plain-faint'
            onClick={() => context.setIsToolsMenuOpen(!context.isToolsMenuOpen)}
            title={getLocale('toolsMenuButtonLabel')}
          >
            <Icon
              className={classnames({
                [styles.slashIconActive]: context.isToolsMenuOpen
              })}
              name='slash'
            />
          </Button>
          {context.isMobile && (
            <Button
              fab
              kind='plain-faint'
              onClick={handleMic}
              disabled={context.shouldDisableUserInput}
              title={getLocale('useMicButtonLabel')}
            >
              <Icon name='microphone' />
            </Button>
          )}
        </div>
        <div>
          <Button
            fab
            kind='plain-faint'
            onClick={handleSubmit}
            disabled={context.shouldDisableUserInput}
            title={getLocale('sendChatButtonLabel')}
          >
            <Icon name='send' />
          </Button>
        </div>
      </div>
    </form>
  )
}

export default InputBox

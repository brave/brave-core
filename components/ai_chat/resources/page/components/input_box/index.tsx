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
import { AIChatContext } from '../../state/context'
import getPageHandlerInstance from '../../api/page_handler'
import ActionTypeLabel from '../action_type_label'

type Props = Pick<AIChatContext,
  'inputText'
  | 'setInputText'
  | 'submitInputTextToAPI'
  | 'selectedActionType'
  | 'resetSelectedActionType'
  | 'isCharLimitApproaching'
  | 'isCharLimitExceeded'
  | 'inputTextCharCountDisplay'
  | 'isToolsMenuOpen'
  | 'setIsToolsMenuOpen'
  | 'isMobile'
  | 'shouldDisableUserInput'>

function InputBox(props: Props) {
  const onInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    props.setInputText(e.target.value)
  }

  const handleSubmit = () => {
    props.submitInputTextToAPI()
  }

  const handleMic = () => {
    getPageHandlerInstance().pageHandler.handleVoiceRecognition()
  }

  const handleOnKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey && !e.nativeEvent.isComposing) {
      if (!e.repeat) {
        props.submitInputTextToAPI()
      }

      e.preventDefault()
    }

    if (
      e.key === 'Backspace' &&
      props.inputText === '' &&
      props.selectedActionType
    ) {
      props.resetSelectedActionType()
    }
  }

  const maybeAutofocus = (node: HTMLTextAreaElement | null) => {
    if (node && props.selectedActionType) {
      node.focus()
    }
  }

  return (
    <form className={styles.form}>
      {props.selectedActionType && (
        <div className={styles.actionsLabelContainer}>
          <ActionTypeLabel
            removable={true}
            actionType={props.selectedActionType}
            onCloseClick={props.resetSelectedActionType}
          />
        </div>
      )}
      <div
        className={styles.growWrap}
        data-replicated-value={props.inputText}
      >
        <textarea
          ref={maybeAutofocus}
          placeholder={getLocale('placeholderLabel')}
          onChange={onInputChange}
          onKeyDown={handleOnKeyDown}
          value={props.inputText}
          autoFocus
          rows={1}
        />
      </div>
      {props.isCharLimitApproaching && (
        <div
          className={classnames({
            [styles.counterText]: true,
            [styles.counterTextVisible]: props.isCharLimitApproaching,
            [styles.counterTextError]: props.isCharLimitExceeded
          })}
        >
          {props.inputTextCharCountDisplay}
        </div>
      )}
      <div className={styles.toolsContainer}>
        <div className={styles.tools}>
          <Button
            fab
            kind='plain-faint'
            onClick={() => props.setIsToolsMenuOpen(!props.isToolsMenuOpen)}
            title={getLocale('toolsMenuButtonLabel')}
          >
            <Icon
              className={classnames({
                [styles.slashIconActive]: props.isToolsMenuOpen
              })}
              name='slash'
            />
          </Button>
          {props.isMobile && (
            <Button
              fab
              kind='plain-faint'
              onClick={handleMic}
              disabled={props.shouldDisableUserInput}
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
            disabled={props.shouldDisableUserInput}
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

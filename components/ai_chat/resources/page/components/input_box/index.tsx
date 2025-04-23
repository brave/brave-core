/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import * as React from 'react'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import ActionTypeLabel from '../../../common/components/action_type_label'
import { AIChatContext } from '../../state/ai_chat_context'
import { ConversationContext } from '../../state/conversation_context'
import styles from './style.module.scss'
import AttachmentButtonMenu from '../attachment_button_menu'
import UploadedImgItem from '../uploaded_img_item'

type Props = Pick<
  ConversationContext,
  | 'inputText'
  | 'setInputText'
  | 'submitInputTextToAPI'
  | 'selectedActionType'
  | 'resetSelectedActionType'
  | 'isCharLimitApproaching'
  | 'isCharLimitExceeded'
  | 'inputTextCharCountDisplay'
  | 'isToolsMenuOpen'
  | 'setIsToolsMenuOpen'
  | 'shouldDisableUserInput'
  | 'handleVoiceRecognition'
  | 'isGenerating'
  | 'handleStopGenerating'
  | 'uploadImage'
  | 'getScreenshots'
  | 'pendingMessageImages'
  | 'removeImage'
  | 'conversationHistory'
  | 'associatedContentInfo'
> &
  Pick<AIChatContext, 'isMobile' | 'hasAcceptedAgreement'>

interface InputBoxProps {
  context: Props
  conversationStarted: boolean
  maybeShowSoftKeyboard?: (querySubmitted: boolean) => unknown
}

function InputBox(props: InputBoxProps) {
  const onInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    props.context.setInputText(e.target.value)
  }

  const querySubmitted = React.useRef(false)

  const handleSubmit = () => {
    querySubmitted.current = true
    props.context.submitInputTextToAPI()
  }

  const handleStopGenerating = () => {
    props.context.handleStopGenerating()
  }

  const handleMic = () => {
    props.context.handleVoiceRecognition?.()
  }

  const handleOnKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey && !e.nativeEvent.isComposing) {
      if (!e.repeat) {
        props.context.submitInputTextToAPI()
      }

      e.preventDefault()
    }

    if (
      e.key === 'Backspace' &&
      props.context.inputText === '' &&
      props.context.selectedActionType
    ) {
      props.context.resetSelectedActionType()
    }
  }

  const maybeAutofocus = (node: HTMLTextAreaElement | null) => {
    if (!node) {
      return
    }
    if (props.context.selectedActionType ||
      props.maybeShowSoftKeyboard?.(querySubmitted.current)) {
      node.focus()
    }
  }

  return (
    <form className={styles.form}>
      {props.context.selectedActionType && (
        <div className={styles.actionsLabelContainer}>
          <ActionTypeLabel
            removable={true}
            actionType={props.context.selectedActionType}
            onCloseClick={props.context.resetSelectedActionType}
          />
        </div>
      )}
      {props.context.pendingMessageImages && (
        <div className={styles.attachmentWrapper}>
          {props.context.pendingMessageImages.map((img, i) =>
            <UploadedImgItem
              key={img.filename}
              uploadedImage={img}
              removeImage={() => props.context.removeImage(i)}
            />
          )}
        </div>
      )}
      <div
        className={styles.growWrap}
        data-replicated-value={props.context.inputText}
      >
        <textarea
          ref={maybeAutofocus}
          placeholder={getLocale(props.conversationStarted
            ? 'placeholderLabel'
            : 'initialPlaceholderLabel')}
          onChange={onInputChange}
          onKeyDown={handleOnKeyDown}
          value={props.context.inputText}
          autoFocus
          rows={1}
        />
      </div>
      {props.context.isCharLimitApproaching && (
        <div
          className={classnames({
            [styles.counterText]: true,
            [styles.counterTextVisible]: props.context.isCharLimitApproaching,
            [styles.counterTextError]: props.context.isCharLimitExceeded
          })}
        >
          {props.context.inputTextCharCountDisplay}
        </div>
      )}
      <div className={styles.toolsContainer}>
        <div className={styles.tools}>
          <Button
            fab
            kind='plain-faint'
            onClick={(e) =>
              {
                e.preventDefault()
                e.stopPropagation()
                props.context.setIsToolsMenuOpen(!props.context.isToolsMenuOpen)
              }
            }
            title={getLocale('toolsMenuButtonLabel')}
          >
            <Icon
              className={classnames({
                [styles.slashIconActive]: props.context.isToolsMenuOpen
              })}
              name='slash'
            />
          </Button>
          {props.context.isMobile && (
            <Button
              fab
              kind='plain-faint'
              onClick={handleMic}
              disabled={props.context.shouldDisableUserInput}
              title={getLocale('useMicButtonLabel')}
            >
              <Icon name='microphone' />
            </Button>
          )}
          <AttachmentButtonMenu
            uploadImage={props.context.uploadImage}
            getScreenshots={props.context.getScreenshots}
            conversationHistory={props.context.conversationHistory}
            associatedContentInfo={props.context.associatedContentInfo}
            isMobile={props.context.isMobile}
          />
        </div>
        <div>
          {props.context.isGenerating ? (
            <Button
              fab
              kind='plain-faint'
              onClick={handleStopGenerating}
              title={getLocale('stopGenerationButtonLabel')}
            >
              <Icon name='stop-circle' />
            </Button>
          ) : (
            <Button
              fab
              kind='plain-faint'
              onClick={handleSubmit}
              disabled={props.context.shouldDisableUserInput}
              title={getLocale('sendChatButtonLabel')}
            >
              <Icon name='send' />
            </Button>
          )}
        </div>
      </div>
    </form>
  )
}

export default InputBox

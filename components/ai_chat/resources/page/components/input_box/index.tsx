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
import { AttachmentImageItem, AttachmentSpinnerItem, AttachmentPageItem } from '../attachment_item'
import usePromise from '$web-common/usePromise'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'

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
  | 'isUploadingFiles'
  | 'shouldSendPageContents'
  | 'disassociateContent'
  | 'associateDefaultContent'
> &
  Pick<AIChatContext, 'isMobile' | 'hasAcceptedAgreement'>

interface InputBoxProps {
  context: Props
  conversationStarted: boolean
  maybeShowSoftKeyboard?: (querySubmitted: boolean) => unknown
}

function usePlaceholderText(attachmentsCount: number, shouldSendPageContents: boolean, conversationStarted: boolean) {
  const { result: attachmentsPlaceholder } = usePromise(() => PluralStringProxyImpl.getInstance().getPluralString('placeholderAttachedPagesLabel', attachmentsCount), [attachmentsCount])

  if (conversationStarted) return getLocale('placeholderLabel')

  if (shouldSendPageContents && attachmentsCount > 0) {
    return attachmentsPlaceholder
  }

  return getLocale('initialPlaceholderLabel')
}

function InputBox(props: InputBoxProps) {
  const onInputChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    props.context.setInputText(e.target.value)
  }

  const querySubmitted = React.useRef(false)
  const attachmentWrapperRef = React.useRef<HTMLDivElement>(null)
  const [attachmentWrapperHeight, setAttachmentWrapperHeight] =
    React.useState(0)

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

  const updateAttachmentWrapperHeight = () => {
    let { height } = attachmentWrapperRef?.current?.getBoundingClientRect() ?? {
      height: 0
    }
    setAttachmentWrapperHeight(height)
  }

  React.useEffect(() => {
    // Update the height of the attachment wrapper when
    // pendingMessageImages changes.
    if (props.context?.pendingMessageImages.length > 0) {
      updateAttachmentWrapperHeight()
    }
  }, [props.context.pendingMessageImages])

  const placeholderText = usePlaceholderText(
    props.context.associatedContentInfo.length,
    props.context.shouldSendPageContents,
    props.conversationStarted
  )

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
      {(props.context.pendingMessageImages.length > 0 ||
        props.context.isUploadingFiles ||
        (props.context.associatedContentInfo &&
          props.context.shouldSendPageContents &&
          !props.conversationStarted)) && (
        <div
          className={classnames({
            [styles.attachmentWrapper]: true,
            [styles.attachmentWrapperScrollStyles]:
              attachmentWrapperHeight >= 240
          })}
          ref={attachmentWrapperRef}
        >
           {props.context.shouldSendPageContents &&
            !props.conversationStarted && props.context.associatedContentInfo.map((content) => (
              <AttachmentPageItem
                key={content.contentId}
                title={content.title}
                url={content.url.url}
                remove={() => props.context.disassociateContent(content)}
              />
            ))}
          {props.context.isUploadingFiles && (
            <AttachmentSpinnerItem title={getLocale('uploadingFileLabel')} />
          )}
          {props.context.pendingMessageImages?.map((img, i) => (
            <AttachmentImageItem
              key={img.filename}
              uploadedImage={img}
              remove={() => props.context.removeImage(i)}
            />
          ))}
        </div>
      )}
      <div
        className={styles.growWrap}
        data-replicated-value={props.context.inputText || placeholderText}
      >
        <textarea
          ref={maybeAutofocus}
          placeholder={placeholderText}
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
            size='large'
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
            associateDefaultContent={props.context.associateDefaultContent}
            conversationStarted={props.conversationStarted}
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

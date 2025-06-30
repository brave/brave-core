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
  | 'disassociateContent'
  | 'associateDefaultContent'
  | 'setShowAttachments'
> &
  Pick<AIChatContext, 'isMobile' | 'hasAcceptedAgreement' | 'getPluralString' | 'tabs'>

export interface InputBoxProps {
  context: Props
  conversationStarted: boolean
  maybeShowSoftKeyboard?: (querySubmitted: boolean) => unknown
}

function usePlaceholderText(attachmentsCount: number, conversationStarted: boolean, getter: AIChatContext['getPluralString']) {
  const { result: attachmentsPlaceholder } = usePromise(async () => getter(S.CHAT_UI_PLACEHOLDER_ATTACHED_PAGES_LABEL, attachmentsCount), [attachmentsCount, getter])

  if (conversationStarted) return getLocale(S.CHAT_UI_PLACEHOLDER_LABEL)

  if (attachmentsCount > 0) {
    return attachmentsPlaceholder
  }

  return getLocale(S.CHAT_UI_INITIAL_PLACEHOLDER_LABEL)
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
    props.conversationStarted,
    props.context.getPluralString
  )

  const showImageAttachments = props.context.pendingMessageImages.length > 0 || props.context.isUploadingFiles
  const showPageAttachments = props.context.associatedContentInfo.length > 0
    && !props.conversationStarted
  const isSendButtonDisabled =
    props.context.shouldDisableUserInput || props.context.inputText === ''

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
      {(showImageAttachments || showPageAttachments) && (
        <div
          className={classnames({
            [styles.attachmentWrapper]: true,
            [styles.attachmentWrapperScrollStyles]:
              attachmentWrapperHeight >= 240
          })}
          ref={attachmentWrapperRef}
        >
          {showPageAttachments && props.context.associatedContentInfo.map((content) => (
            <AttachmentPageItem
              key={content.contentId}
              title={content.title}
              url={content.url.url}
              remove={() => props.context.disassociateContent(content)}
            />
          ))}
          {props.context.isUploadingFiles && (
            <AttachmentSpinnerItem title={getLocale(S.AI_CHAT_UPLOADING_FILE_LABEL)} />
          )}
          {showImageAttachments && props.context.pendingMessageImages?.map((img, i) => (
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
            onClick={(e) => {
              e.preventDefault()
              e.stopPropagation()
              props.context.setIsToolsMenuOpen(!props.context.isToolsMenuOpen)
            }
            }
            title={getLocale(S.AI_CHAT_LEO_TOOLS_BUTTON_LABEL)}
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
              title={getLocale(S.AI_CHAT_USE_MICROPHONE_BUTTON_LABEL)}
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
            tabs={props.context.tabs}
            setShowAttachments={props.context.setShowAttachments}
          />
        </div>
        <div>
          {props.context.isGenerating ? (
            <Button
              fab
              kind='filled'
              className={classnames({
                [styles.button]: true,
                [styles.streamingButton]: true
              })}
              onClick={handleStopGenerating}
              title={getLocale(S.CHAT_UI_STOP_GENERATION_BUTTON_LABEL)}
            >
              <Icon name='stop-circle' className={styles.streamingIcon} />
            </Button>
          ) : (
            <Button
              fab
              kind='filled'
              className={classnames({
                [styles.button]: true,
                [styles.sendButtonDisabled]: isSendButtonDisabled
              })}
              onClick={handleSubmit}
              disabled={isSendButtonDisabled}
              title={getLocale(S.CHAT_UI_SEND_CHAT_BUTTON_LABEL)}
            >
              <Icon className={classnames({
                [styles.sendIconDisabled]: isSendButtonDisabled
              })} name='arrow-up' />
            </Button>
          )}
        </div>
      </div>
    </form>
  )
}

export default InputBox

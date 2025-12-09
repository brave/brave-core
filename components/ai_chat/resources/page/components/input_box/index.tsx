/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import Tooltip from '@brave/leo/react/tooltip'
import * as React from 'react'
import classnames from '$web-common/classnames'
import { getLocale, formatLocale } from '$web-common/locale'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import ActionTypeLabel from '../../../common/components/action_type_label'
import * as Mojom from '../../../common/mojom'
import { AIChatContext } from '../../state/ai_chat_context'
import { ConversationContext } from '../../state/conversation_context'
import styles from './style.module.scss'
import AttachmentButtonMenu from '../attachment_button_menu'
import {
  AttachmentUploadItems,
  AttachmentSpinnerItem,
  AttachmentPageItem,
} from '../attachment_item'
import { ModelSelector } from '../model_selector'
import usePromise from '$web-common/usePromise'
import { isImageFile } from '../../constants/file_types'
import { convertFileToUploadedFile } from '../../utils/file_utils'
import Editable from './editable'
import { stringifyContent } from './editable_content'

const LEARN_MORE_CONTENT_AGENT_URL =
  'https://support.brave.app/hc/en-us/articles/41240379376909'

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
  | 'toolUseTaskState'
  | 'shouldDisableUserInput'
  | 'handleVoiceRecognition'
  | 'isGenerating'
  | 'handleStopGenerating'
  | 'uploadFile'
  | 'getScreenshots'
  | 'pendingMessageFiles'
  | 'removeFile'
  | 'conversationHistory'
  | 'associatedContentInfo'
  | 'isUploadingFiles'
  | 'disassociateContent'
  | 'associateDefaultContent'
  | 'setAttachmentsDialog'
  | 'attachImages'
  | 'pauseTask'
  | 'resumeTask'
  | 'stopTask'
  | 'unassociatedTabs'
  | 'handleSkillClick'
  | 'selectedSkill'
>
  & Pick<
    AIChatContext,
    | 'isMobile'
    | 'isAIChatAgentProfileFeatureEnabled'
    | 'isAIChatAgentProfile'
    | 'hasAcceptedAgreement'
    | 'getPluralString'
    | 'openAIChatAgentProfile'
    | 'skills'
    | 'uiHandler'
  >

export interface InputBoxProps {
  context: Props
  conversationStarted: boolean
  maybeShowSoftKeyboard?: (querySubmitted: boolean) => unknown
}

function usePlaceholderText(
  attachmentsCount: number,
  conversationStarted: boolean,
  getter: AIChatContext['getPluralString'],
) {
  const { result: attachmentsPlaceholder } = usePromise(
    async () =>
      getter(S.CHAT_UI_PLACEHOLDER_ATTACHED_PAGES_LABEL, attachmentsCount),
    [attachmentsCount, getter],
  )

  if (conversationStarted) return getLocale(S.CHAT_UI_PLACEHOLDER_LABEL)

  if (attachmentsCount > 0) {
    return attachmentsPlaceholder
  }

  return getLocale(S.CHAT_UI_INITIAL_PLACEHOLDER_LABEL)
}

function InputBox(props: InputBoxProps) {
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

  const handleOnKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey && !e.nativeEvent.isComposing) {
      if (!e.repeat) {
        props.context.submitInputTextToAPI()
      }

      e.preventDefault()
    }

    if (
      e.key === 'Backspace'
      && stringifyContent(props.context.inputText) === ''
      && props.context.selectedActionType
    ) {
      props.context.resetSelectedActionType()
    }
  }

  const handleOnPaste = async (
    e: React.ClipboardEvent<HTMLTextAreaElement>,
  ) => {
    const clipboardData = e.clipboardData

    if (!clipboardData || clipboardData.files.length === 0) {
      return
    }

    const files = Array.from(clipboardData.files).filter(isImageFile)

    if (files.length === 0) {
      return
    }

    // Prevent the default paste behavior for images
    e.preventDefault()

    try {
      const uploadedFiles = await Promise.all(
        files.map((file) => convertFileToUploadedFile(file)),
      )
      props.context.attachImages(uploadedFiles)
    } catch (error) {
      // Silently fail - error will be handled by the upload system
    }
  }

  const maybeAutofocus = (node: HTMLElement | null) => {
    if (!node) {
      return
    }
    if (
      props.context.selectedActionType
      || props.maybeShowSoftKeyboard?.(querySubmitted.current)
    ) {
      node.focus()
    }
  }

  const updateAttachmentWrapperHeight = () => {
    let { height } = attachmentWrapperRef?.current?.getBoundingClientRect() ?? {
      height: 0,
    }
    setAttachmentWrapperHeight(height)
  }

  React.useEffect(() => {
    // Update the height of the attachment wrapper when
    // pendingMessageFiles changes.
    if (props.context.pendingMessageFiles.length > 0) {
      updateAttachmentWrapperHeight()
    }
  }, [props.context.pendingMessageFiles])

  const placeholderText = usePlaceholderText(
    props.context.associatedContentInfo.length,
    props.conversationStarted,
    props.context.getPluralString,
  )

  const handleContentAgentToggle = () => {
    props.context.openAIChatAgentProfile()
  }

  const showUploadedFiles = props.context.pendingMessageFiles.length > 0
  const pendingContent = props.context.associatedContentInfo.filter(
    (c) => !c.conversationTurnUuid,
  )
  const showTaskStateActions =
    props.context.toolUseTaskState !== Mojom.TaskState.kNone
    && props.context.toolUseTaskState !== Mojom.TaskState.kStopped
  const isSendButtonDisabled =
    props.context.shouldDisableUserInput
    || stringifyContent(props.context.inputText) === ''

  const handleLearnMoreClicked = React.useCallback(() => {
    const mojomUrl = new Url()
    mojomUrl.url = LEARN_MORE_CONTENT_AGENT_URL
    props.context.uiHandler?.openURL(mojomUrl)
  }, [props.context.uiHandler])

  return (
    <form
      className={styles.form}
      onKeyDownCapture={handleOnKeyDown}
    >
      {props.context.selectedActionType && (
        <div className={styles.actionsLabelContainer}>
          <ActionTypeLabel
            removable={true}
            actionType={props.context.selectedActionType}
            onCloseClick={props.context.resetSelectedActionType}
          />
        </div>
      )}
      {props.context.isAIChatAgentProfileFeatureEnabled
        && props.context.isAIChatAgentProfile
        && !props.conversationStarted && (
          <div className={styles.contentAgentWarning}>
            <div className={styles.contentAgentWarningIcon}>
              <Icon name='leo-cursor-filled' />
            </div>
            <div className={styles.contentAgentWarningText}>
              {formatLocale(S.CHAT_UI_CONTENT_AGENT_WARNING_TEXT, {
                $1: (content) => (
                  <a
                    // While we preventDefault onClick, we still need to pass
                    // the href here so we can show link preview.
                    href={LEARN_MORE_CONTENT_AGENT_URL}
                    className={styles.learnMoreLink}
                    onClick={(e) => {
                      e.preventDefault()
                      handleLearnMoreClicked()
                    }}
                  >
                    {content}
                  </a>
                ),
              })}
            </div>
          </div>
        )}

      {showTaskStateActions && (
        <div
          className={styles.taskStateActions}
          data-testid='task-state-actions'
        >
          {props.context.toolUseTaskState === Mojom.TaskState.kPaused && (
            <Button
              size='medium'
              kind='outline'
              onClick={props.context.resumeTask}
              title={getLocale(S.CHAT_UI_RESUME_TASK_BUTTON_LABEL)}
            >
              <Icon
                slot='icon-before'
                name='play-circle'
              />
              <span data-testid='resume-task-button'>
                {getLocale(S.CHAT_UI_RESUME_TASK_BUTTON_LABEL)}
              </span>
            </Button>
          )}

          {props.context.toolUseTaskState === Mojom.TaskState.kRunning && (
            <Button
              kind='outline'
              onClick={props.context.pauseTask}
              title={getLocale(S.CHAT_UI_PAUSE_TASK_BUTTON_LABEL)}
            >
              <Icon
                slot='icon-before'
                name='pause-circle'
              />
              <span data-testid='pause-task-button'>
                {getLocale(S.CHAT_UI_PAUSE_TASK_BUTTON_LABEL)}
              </span>
            </Button>
          )}

          <Button
            kind='outline'
            onClick={props.context.stopTask}
            title={getLocale(S.CHAT_UI_STOP_TASK_BUTTON_LABEL)}
            className={styles.taskStateActionButtonStop}
          >
            <Icon
              slot='icon-before'
              name='stop-circle'
            />
            <span data-testid='stop-task-button'>
              {getLocale(S.CHAT_UI_STOP_TASK_BUTTON_LABEL)}
            </span>
          </Button>
        </div>
      )}

      {(showUploadedFiles || pendingContent.length > 0) && (
        <div
          className={classnames({
            [styles.attachmentWrapper]: true,
            [styles.attachmentWrapperScrollStyles]:
              attachmentWrapperHeight >= 240,
          })}
          ref={attachmentWrapperRef}
        >
          {pendingContent.map((content) => (
            <AttachmentPageItem
              key={content.contentId}
              title={content.title}
              url={content.url.url}
              remove={() => props.context.disassociateContent(content)}
            />
          ))}
          {props.context.isUploadingFiles && (
            <AttachmentSpinnerItem
              title={getLocale(S.AI_CHAT_UPLOADING_FILE_LABEL)}
            />
          )}
          <AttachmentUploadItems
            uploadedFiles={props.context.pendingMessageFiles}
            remove={(index) => props.context.removeFile(index)}
          />
        </div>
      )}
      <Editable
        ref={maybeAutofocus}
        placeholder={placeholderText}
        content={props.context.inputText}
        onContentChange={(e) => {
          props.context.setInputText(e)
        }}
        onPaste={handleOnPaste}
      />
      {props.context.isCharLimitApproaching && (
        <div
          className={classnames({
            [styles.counterText]: true,
            [styles.counterTextVisible]: props.context.isCharLimitApproaching,
            [styles.counterTextError]: props.context.isCharLimitExceeded,
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
            onClick={(e) => {
              e.preventDefault()
              e.stopPropagation()
              props.context.setIsToolsMenuOpen(!props.context.isToolsMenuOpen)
            }}
            title={getLocale(S.AI_CHAT_LEO_TOOLS_BUTTON_LABEL)}
          >
            <Icon
              className={classnames({
                [styles.slashIconActive]: props.context.isToolsMenuOpen,
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
            uploadFile={props.context.uploadFile}
            getScreenshots={props.context.getScreenshots}
            conversationHistory={props.context.conversationHistory}
            associatedContentInfo={props.context.associatedContentInfo}
            associateDefaultContent={props.context.associateDefaultContent}
            conversationStarted={props.conversationStarted}
            isMobile={props.context.isMobile}
            unassociatedTabs={props.context.unassociatedTabs}
            setAttachmentsDialog={props.context.setAttachmentsDialog}
          />
          {props.context.hasAcceptedAgreement
            && props.context.isAIChatAgentProfileFeatureEnabled
            && !props.context.isAIChatAgentProfile && (
              <Button
                fab
                kind='plain-faint'
                onClick={handleContentAgentToggle}
                title={getLocale(S.CHAT_UI_AI_BROWSING_TOGGLE_BUTTON_LABEL)}
              >
                <Icon name='leo-cursor' />
              </Button>
            )}
          {props.context.isAIChatAgentProfileFeatureEnabled
            && props.context.isAIChatAgentProfile && (
              <div data-testid='agent-profile-tooltip'>
                <Tooltip
                  text={getLocale(S.CHAT_UI_CONTENT_AGENT_PROFILE_BUTTON_LABEL)}
                >
                  <Icon
                    className={styles.contentAgentButtonEnabled}
                    name='leo-cursor'
                  />
                </Tooltip>
              </div>
            )}
        </div>
        <div className={styles.modelSelectorAndSendButton}>
          <ModelSelector />
          {props.context.isGenerating ? (
            <Button
              fab
              kind='filled'
              className={classnames({
                [styles.button]: true,
                [styles.streamingButton]: true,
              })}
              onClick={handleStopGenerating}
              title={getLocale(S.CHAT_UI_STOP_GENERATION_BUTTON_LABEL)}
            >
              <Icon
                name='stop-circle'
                className={styles.streamingIcon}
              />
            </Button>
          ) : (
            <Button
              fab
              kind='filled'
              className={classnames({
                [styles.button]: true,
                [styles.sendButtonDisabled]: isSendButtonDisabled,
              })}
              onClick={handleSubmit}
              disabled={isSendButtonDisabled}
              title={getLocale(S.CHAT_UI_SEND_CHAT_BUTTON_LABEL)}
            >
              <Icon
                className={classnames({
                  [styles.sendIconDisabled]: isSendButtonDisabled,
                })}
                name='arrow-up'
              />
            </Button>
          )}
        </div>
      </div>
    </form>
  )
}

export default InputBox

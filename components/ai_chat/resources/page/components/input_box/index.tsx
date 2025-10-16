/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import Tooltip from '@brave/leo/react/tooltip'
import * as React from 'react'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import ActionTypeLabel from '../../../common/components/action_type_label'
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
import * as Mojom from '../../../common/mojom'

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
  | 'unassociatedTabs'
  | 'handleSmartModeClick'
  | 'selectedSmartMode'
  | 'resetSelectedSmartMode'
>
  & Pick<
    AIChatContext,
    | 'isMobile'
    | 'isAIChatAgentProfileFeatureEnabled'
    | 'isAIChatAgentProfile'
    | 'hasAcceptedAgreement'
    | 'getPluralString'
    | 'openAIChatAgentProfile'
    | 'smartModes'
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

// Smart mode regex patterns - currently limited to start of input only.
// We plan to support it at anywhere after
// https://github.com/brave/brave-browser/issues/48610 is resolved.
const SMART_MODE_DETECTION_REGEX = /^\/([a-zA-Z0-9_-]+)/

function InputBox(props: InputBoxProps) {
  const detectAndSetSmartMode = React.useCallback(
    (value: string) => {
      const match = value.match(SMART_MODE_DETECTION_REGEX)

      if (!match) {
        return
      }

      const shortcut = match[1]
      const foundMode = props.context.smartModes.find(
        (mode: Mojom.SmartMode) =>
          mode.shortcut.toLowerCase() === shortcut.toLowerCase(),
      )

      // Only set if different from current selection
      if (foundMode) {
        props.context.handleSmartModeClick(foundMode)
      }
    },
    [props.context.smartModes, props.context.handleSmartModeClick],
  )

  const onInputChange = React.useCallback(
    (e: React.ChangeEvent<HTMLTextAreaElement>) => {
      const newValue = e.target.value
      props.context.setInputText(newValue)

      if (props.context.selectedSmartMode) {
        // Check if current smart mode shortcut is still valid
        const currentModeShortcut = `/${props.context.selectedSmartMode.shortcut}`
        if (!newValue.startsWith(currentModeShortcut)) {
          props.context.resetSelectedSmartMode()
        }
        return
      }
      if (newValue.startsWith('/') && props.context.smartModes.length > 0) {
        // No smart mode selected, but input starts with /, try to detect
        detectAndSetSmartMode(newValue)
      }
    },
    [
      props.context.selectedSmartMode,
      props.context.smartModes,
      detectAndSetSmartMode,
    ],
  )

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
      e.key === 'Backspace'
      && props.context.inputText === ''
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

  const maybeAutofocus = (node: HTMLTextAreaElement | null) => {
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
      <div
        className={styles.growWrap}
        data-replicated-value={props.context.inputText || placeholderText}
      >
        <textarea
          ref={maybeAutofocus}
          placeholder={placeholderText}
          onChange={onInputChange}
          onKeyDown={handleOnKeyDown}
          onPaste={handleOnPaste}
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
                title={'Open Leo AI Content Agent Window'}
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

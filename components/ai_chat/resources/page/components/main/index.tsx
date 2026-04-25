/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import AlertCenter from '@brave/leo/react/alertCenter'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import classnames from '$web-common/classnames'
import DragOverlay from '../drag_overlay'
import * as Mojom from '../../../common/mojom'
import scrollerStyles from '../../../common/scroller.module.scss'
import { useConversation } from '../../state/conversation_context'
import { useAIChat } from '../../state/ai_chat_context'
import ConversationsList from '../conversations_list'
import DeleteConversationModal from '../delete_conversation_modal'
import { ConversationHeader } from '../header'
import InputBox from '../input_box'
import OpenExternalLinkModal from '../open_external_link_modal'
import RateMessagePrivacyModal from '../rate_message_privacy_modal'
import SkillModal from '../skill_modal/skill_modal'
import PrivacyMessage from '../privacy_message'
import FeedbackForm from '../feedback_form'
import ToolsMenu, {
  ExtendedActionEntry,
  getIsSkill,
} from '../filter_menu/tools_menu'
import WelcomeGuide from '../welcome_guide'
import styles from './style.module.scss'
import Attachments from '../attachments'
import useHasConversationStarted from '../../hooks/useHasConversationStarted'
import { useExtractedQuery } from '../filter_menu/query'
import TabsMenu from '../filter_menu/attachments_menu'
import { stringifyContent } from '../input_box/editable_content'

function Main() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const [isConversationListOpen, setIsConversationsListOpen] =
    React.useState(false)
  const { isDragActive, isDragOver } = conversationContext

  const showAttachments = !!conversationContext.attachmentsDialog

  const headerElement = React.useRef<HTMLDivElement>(null)

  // Ask for opt-in once the first message is sent
  const showAgreementModal =
    !aiChatContext.hasAcceptedAgreement
    && !!conversationContext.conversationHistory.length

  const hasConversationStarted = useHasConversationStarted(
    conversationContext.conversationUuid,
  )

  const isHistoryPlaceholderData =
    conversationContext.api.useGetConversationHistory().isPlaceholderData

  const maybeShowSoftKeyboard = (querySubmitted: boolean) => {
    if (
      aiChatContext.isMobile
      && aiChatContext.hasAcceptedAgreement
      // We have loaded real data
      && !isHistoryPlaceholderData
      && !querySubmitted
      && !conversationContext.isGenerating
      && conversationContext.conversationHistory.length === 0
    ) {
      aiChatContext.api.uiHandler.showSoftKeyboard()
      return true
    }
    return false
  }

  const extractedQuery = useExtractedQuery(
    stringifyContent(conversationContext.inputText),
    {
      onlyAtStart: true,
      triggerCharacter: '/',
    },
  )

  // Transform skills into ActionGroup format and append to actionList
  const categoriesWithSkills = React.useMemo(() => {
    if (!aiChatContext.skills || aiChatContext.skills.length === 0) {
      return aiChatContext.actionList
    }

    const skillGroup = {
      category: getLocale(S.CHAT_UI_SKILLS_GROUP),
      entries: aiChatContext.skills,
    }

    return [skillGroup, ...aiChatContext.actionList]
  }, [aiChatContext.actionList, aiChatContext.skills])

  const handleToolsMenuSelect = React.useCallback(
    (value: ExtendedActionEntry) => {
      if (getIsSkill(value)) {
        conversationContext.handleSkillClick(value)
        return
      }
      conversationContext.handleActionTypeClick(value.details!.type)
    },
    [
      conversationContext.handleSkillClick,
      conversationContext.handleActionTypeClick,
    ],
  )

  const handleToolsMenuClick = React.useCallback(
    (value: ExtendedActionEntry) => {
      if (getIsSkill(value)) {
        aiChatContext.api.metrics.recordSkillClick(value.shortcut)
      }
      handleToolsMenuSelect(value)
    },
    [
      conversationContext.handleSkillClick,
      conversationContext.handleActionTypeClick,
    ],
  )

  const handleToolsMenuEditClick = (skill: Mojom.Skill) => {
    conversationContext.handleSkillEdit(skill)
  }

  const handleNewSkillClick = React.useCallback(() => {
    const inputText = stringifyContent(conversationContext.inputText)
    aiChatContext.setSkillDialog({
      id: '',
      shortcut: inputText.startsWith('/') ? inputText.substring(1) : inputText,
      prompt: '',
      model: '',
      createdTime: { internalValue: BigInt(0) },
      lastUsed: { internalValue: BigInt(0) },
    })
  }, [conversationContext.inputText, aiChatContext.setSkillDialog])

  return (
    <main
      data-testid='main'
      className={classnames({
        [styles.main]: true,
        [styles.mainPanel]: !aiChatContext.isStandalone,
        [styles.mainMobile]: aiChatContext.isMobile,
        [styles.dragOver]: isDragOver,
        [styles.dragActive]: isDragActive,
      })}
    >
      <DragOverlay />
      {isConversationListOpen && !aiChatContext.isStandalone && (
        <div className={styles.conversationsList}>
          <div
            className={classnames({
              [styles.conversationsListHeader]: true,
            })}
          >
            <Button
              kind='plain-faint'
              fab
              onClick={() => {
                setIsConversationsListOpen?.(false)
              }}
            >
              <Icon name='arrow-left' />
            </Button>
          </div>
          <ConversationsList
            setIsConversationsListOpen={setIsConversationsListOpen}
          />
        </div>
      )}
      {showAgreementModal && <PrivacyMessage />}
      <ConversationHeader
        ref={headerElement}
        setIsConversationsListOpen={setIsConversationsListOpen}
      />
      <AlertCenter
        position='top-left'
        className={styles.alertCenter}
      />
      {!aiChatContext.hasAcceptedAgreement && !hasConversationStarted ? (
        <div
          className={classnames(
            scrollerStyles.scroller,
            styles.centeredContent,
          )}
        >
          <WelcomeGuide />
        </div>
      ) : (
        <>
          {conversationContext.isFeedbackFormVisible && (
            <Dialog
              isOpen
              onClose={conversationContext.handleFeedbackFormCancel}
              className={styles.attachmentsDialog}
            >
              <FeedbackForm />
            </Dialog>
          )}

          <aiChatContext.conversationEntriesComponent
            className={styles.conversationContainer}
          />
        </>
      )}
      {showAttachments && (
        <Dialog
          isOpen
          onClose={() => conversationContext.setAttachmentsDialog(null)}
          className={styles.attachmentsDialog}
        >
          <Attachments />
        </Dialog>
      )}
      <div className={styles.input}>
        <ToolsMenu
          isOpen={
            !conversationContext.selectedSkill
            && conversationContext.isToolsMenuOpen
          }
          setIsOpen={conversationContext.setIsToolsMenuOpen}
          query={extractedQuery}
          categories={categoriesWithSkills}
          isMobile={aiChatContext.isMobile}
          handleClick={handleToolsMenuClick}
          handleEditClick={handleToolsMenuEditClick}
          handleNewSkillClick={handleNewSkillClick}
          handleAutoSelect={handleToolsMenuSelect}
        />
        <TabsMenu />
        <InputBox
          conversationStarted={hasConversationStarted}
          context={{ ...conversationContext, ...aiChatContext }}
          maybeShowSoftKeyboard={maybeShowSoftKeyboard}
        />
      </div>
      <DeleteConversationModal />
      <OpenExternalLinkModal />
      <RateMessagePrivacyModal />
      {aiChatContext.skillDialog && <SkillModal />}
    </main>
  )
}

export default Main

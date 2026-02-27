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
import { useConversation } from '../../state/conversation_context'
import { useAIChat } from '../../state/ai_chat_context'
import { isLeoModel } from '../../model_utils'
import ErrorConnection from '../alerts/error_connection'
import ErrorConversationEnd from '../alerts/error_conversation_end'
import ErrorInvalidAPIKey from '../alerts/error_invalid_api_key'
import ErrorInvalidEndpointURL from '../alerts/error_invalid_endpoint_url'
import ErrorRateLimit from '../alerts/error_rate_limit'
import ErrorServiceOverloaded from '../alerts/error_service_overloaded'
import LongConversationInfo from '../alerts/long_conversation_info'
import TemporaryChatInfo from '../alerts/temporary_chat_info'
import NoticeConversationStorage from '../notices/notice_conversation_storage'
import WarningPremiumDisconnected from '../alerts/warning_premium_disconnected'
import ConversationsList from '../conversations_list'
import DeleteConversationModal from '../delete_conversation_modal'
import FeedbackForm from '../feedback_form'
import { ConversationHeader } from '../header'
import InputBox from '../input_box'
import ModelIntro from '../model_intro'
import OpenExternalLinkModal from '../open_external_link_modal'
import RateMessagePrivacyModal from '../rate_message_privacy_modal'
import SkillModal from '../skill_modal/skill_modal'
import PremiumSuggestion from '../premium_suggestion'
import PrivacyMessage from '../privacy_message'
import {
  GenerateSuggestionsButton,
  SuggestedQuestion,
} from '../suggested_question'
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
import { useScrollToBottom } from './useScrollToBottom'

const SUGGESTION_STATUS_SHOW_BUTTON = new Set<Mojom.SuggestionGenerationStatus>(
  [
    Mojom.SuggestionGenerationStatus.CanGenerate,
    Mojom.SuggestionGenerationStatus.IsGenerating,
  ],
)

function Main() {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const [isConversationListOpen, setIsConversationsListOpen] =
    React.useState(false)
  const [isContentReady, setIsContentReady] = React.useState(false)
  const { isDragActive, isDragOver } = conversationContext

  const shouldShowPremiumSuggestionForModel =
    aiChatContext.hasAcceptedAgreement
    && !aiChatContext.isPremiumStatusFetching // Avoid flash of content
    && !aiChatContext.isPremiumUser
    && (conversationContext.currentModel?.options.leoModelOptions?.access
      === Mojom.ModelAccess.PREMIUM
      || !!conversationContext.showPremiumSuggestionForRegenerate)

  const shouldShowStorageNotice =
    aiChatContext.hasAcceptedAgreement
    && aiChatContext.isHistoryFeatureEnabled
    && aiChatContext.isStoragePrefEnabled
    && !aiChatContext.isStorageNoticeDismissed

  const shouldShowPremiumSuggestionStandalone =
    aiChatContext.hasAcceptedAgreement
    && !aiChatContext.isPremiumStatusFetching // Avoid flash of content
    && !shouldShowPremiumSuggestionForModel // Don't show 2 premium prompts
    && !conversationContext.apiHasError // Don't show premium prompt and errors (rate limit error has its own premium prompt suggestion)
    && !shouldShowStorageNotice // Don't show premium prompt and storage notice
    && aiChatContext.canShowPremiumPrompt
    && conversationContext.associatedContentInfo === null // AssociatedContent request has finished and this is a standalone conversation
    && !aiChatContext.isPremiumUser

  const showAttachments = !!conversationContext.attachmentsDialog

  const showTemporaryChatInfo =
    conversationContext.api.useGetState().data.temporary

  let currentErrorElement = null

  const headerElement = React.useRef<HTMLDivElement>(null)
  const conversationContentElement = React.useRef<HTMLDivElement>(null)

  // Determine which, if any, error message should be displayed
  if (aiChatContext.hasAcceptedAgreement && conversationContext.apiHasError) {
    switch (conversationContext.currentError) {
      case Mojom.APIError.ConnectionIssue:
        currentErrorElement = (
          <ErrorConnection onRetry={conversationContext.retryAPIRequest} />
        )
        break
      case Mojom.APIError.InvalidAPIKey:
        currentErrorElement = (
          <ErrorInvalidAPIKey onRetry={conversationContext.retryAPIRequest} />
        )
        break
      case Mojom.APIError.ServiceOverloaded:
        currentErrorElement = (
          <ErrorServiceOverloaded
            onRetry={conversationContext.retryAPIRequest}
          />
        )
        break
      case Mojom.APIError.RateLimitReached:
        currentErrorElement = <ErrorRateLimit />
        break
      case Mojom.APIError.ContextLimitReached:
        currentErrorElement = <ErrorConversationEnd />
        break
      case Mojom.APIError.InvalidEndpointURL:
        currentErrorElement = <ErrorInvalidEndpointURL />
        break
    }
  }

  const scrollElement = React.useRef<HTMLDivElement | null>(null)
  const { scrollToBottomContinuously, hasScrollableContent } =
    useScrollToBottom(scrollElement, conversationContentElement)

  // Reset scroll and content-ready state when switching conversations
  // so the new conversation starts fresh at the top. useLayoutEffect
  // runs before paint so the user never sees the stale scroll position.
  // <if expr="is_ios">
  React.useLayoutEffect(() => {
    setIsContentReady(false)
    if (scrollElement.current) {
      scrollElement.current.scrollTop = 0
    }
  }, [conversationContext.conversationUuid])
  // </if>

  // Scroll to bottom when opening a conversation
  React.useEffect(() => {
    if (!conversationContext.conversationUuid || !isContentReady) {
      return
    }

    scrollToBottomContinuously(/*animate=*/ false)
  }, [
    conversationContext.conversationUuid,
    isContentReady,
    scrollToBottomContinuously,
  ])

  // Ask for opt-in once the first message is sent
  const showAgreementModal =
    !aiChatContext.hasAcceptedAgreement
    && !!conversationContext.conversationHistory.length

  const showContent =
    !aiChatContext.hasAcceptedAgreement
    || !conversationContext.conversationUuid
    || isContentReady

  const showSuggestions: boolean =
    aiChatContext.hasAcceptedAgreement
    && (conversationContext.suggestedQuestions.length > 0
      || SUGGESTION_STATUS_SHOW_BUTTON.has(
        conversationContext.suggestionStatus,
      ))

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
      <div
        className={classnames({
          [styles.scroller]: true,
          [styles.centeredContent]: !aiChatContext.hasAcceptedAgreement,
        })}
        ref={scrollElement}
      >
        <div
          className={classnames({
            [styles.conversationContent]: true,
            [styles.hasAcceptedAgreement]: aiChatContext.hasAcceptedAgreement,
            [styles.showContent]: showContent,
          })}
          ref={conversationContentElement}
        >
          {aiChatContext.hasAcceptedAgreement && (
            <>
              <ModelIntro />

              {showTemporaryChatInfo && (
                <div className={styles.promptContainer}>
                  <TemporaryChatInfo />
                </div>
              )}

              <div
                className={classnames({
                  [styles.aichatIframeContainer]: true,
                  [styles.dragActive]: isDragActive,
                })}
              >
                {!!conversationContext.conversationUuid && (
                  <aiChatContext.conversationEntriesComponent
                    // Force remount when switching conversations on iOS
                    // so the iframe height resets cleanly.
                    // <if expr="is_ios">
                    key={conversationContext.conversationUuid}
                    // </if>
                    onIsContentReady={setIsContentReady}
                  />
                )}
              </div>

              {conversationContext.isFeedbackFormVisible && (
                <div
                  className={classnames([
                    styles.promptContainer,
                    styles.feedbackForm,
                  ])}
                >
                  <FeedbackForm />
                </div>
              )}

              {showSuggestions && (
                <div className={styles.suggestionsContainer}>
                  <div className={styles.questionsList}>
                    {conversationContext.suggestedQuestions.map(
                      (question, i) => (
                        <SuggestedQuestion
                          key={question}
                          question={question}
                          index={i}
                        />
                      ),
                    )}
                    {SUGGESTION_STATUS_SHOW_BUTTON.has(
                      conversationContext.suggestionStatus,
                    )
                      && conversationContext.associatedContentInfo.length
                        > 0 && <GenerateSuggestionsButton />}
                  </div>
                </div>
              )}
            </>
          )}
          {currentErrorElement && (
            <div className={styles.promptContainer}>{currentErrorElement}</div>
          )}
          {shouldShowStorageNotice && (
            <div className={styles.promptContainer}>
              <NoticeConversationStorage />
            </div>
          )}
          {shouldShowPremiumSuggestionForModel && (
            <div className={styles.promptContainer}>
              <PremiumSuggestion
                title={getLocale(S.CHAT_UI_UNLOCK_PREMIUM_TITLE)}
                secondaryActionButton={
                  <Button
                    kind='plain-faint'
                    onClick={() => conversationContext.switchToBasicModel()}
                  >
                    {getLocale(S.CHAT_UI_SWITCH_TO_BASIC_MODEL_BUTTON_LABEL)}
                  </Button>
                }
              />
            </div>
          )}
          {shouldShowPremiumSuggestionStandalone && (
            <div className={styles.promptContainer}>
              <PremiumSuggestion
                title={getLocale(S.CHAT_UI_UNLOCK_PREMIUM_TITLE)}
                secondaryActionButton={
                  <Button
                    kind='plain-faint'
                    onClick={() => aiChatContext.dismissPremiumPrompt()}
                  >
                    {getLocale(S.CHAT_UI_DISMISS_BUTTON_LABEL)}
                  </Button>
                }
              />
            </div>
          )}
          {aiChatContext.isPremiumUserDisconnected
            && (!conversationContext.currentModel
              || isLeoModel(conversationContext.currentModel)) && (
              <div className={styles.promptContainer}>
                <WarningPremiumDisconnected />
              </div>
            )}
          {conversationContext.shouldShowLongConversationInfo && (
            <div className={styles.promptContainer}>
              <LongConversationInfo />
            </div>
          )}
          {!aiChatContext.hasAcceptedAgreement
            && !conversationContext.conversationHistory.length && (
              <WelcomeGuide />
            )}
        </div>
        <div className={styles.scrollButtonContainer}>
          <Button
            kind='outline'
            size='small'
            title={getLocale(S.CHAT_UI_SCROLL_TO_BOTTOM_BUTTON_TITLE)}
            fab
            className={classnames({
              [styles.scrollToBottomButton]: true,
              [styles.hasScrollableContent]: hasScrollableContent,
            })}
            onClick={() => scrollToBottomContinuously()}
          >
            <Icon name='arrow-down' />
          </Button>
        </div>
      </div>
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

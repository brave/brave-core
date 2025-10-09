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
import SmartModeModal from '../smart_mode_modal/smart_mode_modal'
import PremiumSuggestion from '../premium_suggestion'
import PrivacyMessage from '../privacy_message'
import {
  GenerateSuggestionsButton,
  SuggestedQuestion,
} from '../suggested_question'
import ToolsMenu, {
  ExtendedActionEntry,
  getIsSmartMode,
} from '../filter_menu/tools_menu'
import WelcomeGuide from '../welcome_guide'
import styles from './style.module.scss'
import Attachments from '../attachments'
import useHasConversationStarted from '../../hooks/useHasConversationStarted'
import { useExtractedQuery } from '../filter_menu/query'
import TabsMenu from '../filter_menu/tabs_menu'

// Amount of pixels user has to scroll up to break out of
// automatic scroll to bottom when new response lines are generated.
const SCROLL_BOTTOM_THRESHOLD = 20
// Amount of pixels below the currently generated line to show
// when automatically scrolling to bottom.
const SCROLL_BOTTOM_PADDING = 18

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
    && conversationContext.currentModel?.options.leoModelOptions?.access
      === Mojom.ModelAccess.PREMIUM

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

  const showTemporaryChatInfo = conversationContext.isTemporaryChat

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

  // Automatic scroll to bottom of scroll anchor when generating new response lines
  const scrollIsAtBottom = React.useRef(true)
  const scrollElement = React.useRef<HTMLDivElement | null>(null)
  const scrollAnchor = React.useRef<HTMLDivElement | null>(null)

  const handleScroll = React.useCallback(
    (e: React.UIEvent<HTMLDivElement>) => {
      // Monitor scroll positions only when Assistant is generating,
      // but always reset to bottom before next generation
      if (!conversationContext.isGenerating) {
        scrollIsAtBottom.current = true
      } else if (scrollAnchor.current && conversationContentElement.current) {
        const el = e.currentTarget
        const idealScrollFromBottom =
          el.scrollHeight
          - (scrollAnchor.current.offsetTop + scrollAnchor.current.offsetHeight)
        const scrollBottom = el.scrollHeight - (el.clientHeight + el.scrollTop)
        scrollIsAtBottom.current =
          scrollBottom <= idealScrollFromBottom + SCROLL_BOTTOM_THRESHOLD
      }
    },
    [conversationContext.isGenerating],
  )

  const handleConversationEntriesHeightChanged = () => {
    if (
      !conversationContext.isGenerating
      || !scrollElement.current
      || !scrollIsAtBottom.current
      || !scrollAnchor.current
    ) {
      return
    }
    scrollElement.current.scrollTop =
      scrollAnchor.current.offsetTop
      + scrollAnchor.current?.offsetHeight
      - scrollElement.current.clientHeight
      + SCROLL_BOTTOM_PADDING
  }

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

  const maybeShowSoftKeyboard = (querySubmitted: boolean) => {
    if (
      aiChatContext.isMobile
      && aiChatContext.hasAcceptedAgreement
      && conversationContext.historyInitialized
      && !querySubmitted
      && !conversationContext.isGenerating
      && conversationContext.conversationHistory.length === 0
    ) {
      aiChatContext.uiHandler?.showSoftKeyboard()
      return true
    }
    return false
  }

  const extractedQuery = useExtractedQuery(conversationContext.inputText, {
    onlyAtStart: true,
    triggerCharacter: '/',
  })

  // Transform smartModes into ActionGroup format and append to actionList
  const categoriesWithSmartModes = React.useMemo(() => {
    if (!aiChatContext.smartModes || aiChatContext.smartModes.length === 0) {
      return aiChatContext.actionList
    }

    const smartModeGroup = {
      category: getLocale(S.CHAT_UI_SMART_MODES_GROUP),
      entries: aiChatContext.smartModes,
    }

    return [smartModeGroup, ...aiChatContext.actionList]
  }, [aiChatContext.actionList, aiChatContext.smartModes])

  const handleToolsMenuClick = React.useCallback(
    (value: ExtendedActionEntry) => {
      if (getIsSmartMode(value)) {
        conversationContext.handleSmartModeClick(value)
        return
      }
      conversationContext.handleActionTypeClick(value.details!.type)
    },
    [
      conversationContext.handleSmartModeClick,
      conversationContext.handleActionTypeClick,
    ],
  )

  const handleToolsMenuEditClick = (smartMode: Mojom.SmartMode) => {
    conversationContext.handleSmartModeEdit(smartMode)
  }

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
      <div
        className={classnames({
          [styles.scroller]: true,
          [styles.centeredContent]: !aiChatContext.hasAcceptedAgreement,
        })}
        ref={scrollElement}
        onScroll={handleScroll}
      >
        <AlertCenter
          position='top-left'
          className={styles.alertCenter}
        />
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
                ref={scrollAnchor}
              >
                {!!conversationContext.conversationUuid && (
                  <aiChatContext.conversationEntriesComponent
                    onIsContentReady={setIsContentReady}
                    onHeightChanged={handleConversationEntriesHeightChanged}
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
            !conversationContext.selectedSmartMode
            && conversationContext.isToolsMenuOpen
          }
          setIsOpen={conversationContext.setIsToolsMenuOpen}
          query={extractedQuery}
          categories={categoriesWithSmartModes}
          handleClick={handleToolsMenuClick}
          handleEditClick={handleToolsMenuEditClick}
        />
        {!hasConversationStarted && <TabsMenu />}
        <InputBox
          conversationStarted={hasConversationStarted}
          context={{ ...conversationContext, ...aiChatContext }}
          maybeShowSoftKeyboard={maybeShowSoftKeyboard}
        />
      </div>
      <DeleteConversationModal />
      <OpenExternalLinkModal />
      <RateMessagePrivacyModal />
      {aiChatContext.smartModeDialog && <SmartModeModal />}
    </main>
  )
}

export default Main

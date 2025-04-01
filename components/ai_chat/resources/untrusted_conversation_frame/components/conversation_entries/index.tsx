/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import useLongPress from '$web-common/useLongPress'
import * as Mojom from '../../../common/mojom'
import ActionTypeLabel from '../../../common/components/action_type_label'
import UploadedImgItem from '../../../page/components/uploaded_img_item'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import AssistantReasoning from '../assistant_reasoning'
import ContextActionsAssistant from '../context_actions_assistant'
import ContextMenuHuman from '../context_menu_human'
import Quote from '../quote'
import LongPageInfo from '../page_context_message/long_page_info'
import AssistantResponse from '../assistant_response'
import EditInput from '../edit_input'
import EditIndicator from '../edit_indicator'
import { getReasoningText } from './conversation_entries_utils'
import styles from './style.module.scss'

function ConversationEntries() {
  const conversationContext = useUntrustedConversationContext()

  const [hoverMenuButtonId, setHoverMenuButtonId] = React.useState<number>()
  const [activeMenuId, setActiveMenuId] = React.useState<number>()
  const [editInputId, setEditInputId] = React.useState<number>()

  const handleEditSubmit = (index: number, text: string) => {
    conversationContext.conversationHandler?.modifyConversation(index, text)
    setEditInputId(undefined)
  }

  const showHumanMenu = (id: number) => {
    setActiveMenuId(id)
  }

  const hideHumanMenu = () => {
    setActiveMenuId(undefined)
  }

  const { onTouchEnd, onTouchMove, onTouchStart } = useLongPress({
    onLongPress: (e: React.TouchEvent) => {
      const currentTarget = e.currentTarget as HTMLElement
      const id = currentTarget.getAttribute('data-id')
      if (id === null) return
      showHumanMenu(parseInt(id))
    },
    onTouchMove: () => setActiveMenuId(undefined)
  })

  const getCompletion = (turn: Mojom.ConversationTurn) => {
    const event = turn.events?.find((event) => event.completionEvent)
    return event?.completionEvent?.completion ?? ''
  }

  const allAllowedLinks: string[] = conversationContext.conversationHistory
    .flatMap(turn =>
    turn.events?.flatMap(event =>
      event.sourcesEvent?.sources?.map(source => source.url.url) || []
    ) || []
  )

  return (
    <>
      <div>
        {conversationContext.conversationHistory.map((turn, id) => {
          const isLastEntry =
            (id === conversationContext.conversationHistory.length - 1)
          const isAIAssistant =
            turn.characterType === Mojom.CharacterType.ASSISTANT
          const isEntryInProgress =
            isLastEntry && isAIAssistant && conversationContext.isGenerating
          const isHuman = turn.characterType === Mojom.CharacterType.HUMAN
          const showLongPageContentInfo =
            id === 1 &&
            isAIAssistant &&
            ((conversationContext.contentUsedPercentage ?? 100) < 100 ||
             (conversationContext.trimmedTokens > 0 && conversationContext.totalTokens > 0))
          const showEditInput = editInputId === id
          const showEditIndicator = !showEditInput && !!turn.edits?.length
          const latestEdit = turn.edits?.at(-1)
          const latestTurn = latestEdit ?? turn
          const latestTurnText = isAIAssistant
            ? getCompletion(latestTurn)
            : latestTurn.text
          const lastEditedTime = latestTurn.createdTime
          const hasReasoning = latestTurnText.includes('<think>')

          const turnContainer = classnames({
            [styles.turnContainerMobile]: conversationContext.isMobile
          })

          const turnClass = classnames({
            [styles.turn]: true,
            [styles.turnAI]: isAIAssistant
          })

          const handleCopyText = () => {
            if (isAIAssistant) {
              const event = latestTurn.events?.find(
                (event) => event.completionEvent
              )
              if (!event?.completionEvent) return
              navigator.clipboard.writeText(event.completionEvent.completion)
            } else {
              navigator.clipboard.writeText(latestTurnText)
            }
          }

          return (
            <div
              key={id}
              className={turnContainer}
            >
              <div
                data-id={id}
                className={turnClass}
                onMouseEnter={() => isHuman && setHoverMenuButtonId(id)}
                onMouseLeave={() => {
                  if (!isHuman) return
                  setActiveMenuId(undefined)
                  setHoverMenuButtonId(undefined)
                }}
                onTouchStart={isHuman ? onTouchStart : undefined}
                onTouchEnd={isHuman ? onTouchEnd : undefined}
                onTouchMove={isHuman ? onTouchMove : undefined}
              >
                <div
                  className={
                    isAIAssistant ? styles.message : styles.humanMessage
                  }
                >
                  {isAIAssistant && hasReasoning && (
                    <AssistantReasoning
                      text={getReasoningText(latestTurnText)}
                      isReasoning={
                        isEntryInProgress &&
                        !latestTurnText.includes('</think>')
                      }
                    />
                  )}
                  {isAIAssistant && !showEditInput && (
                    <AssistantResponse
                      entry={latestTurn}
                      isEntryInProgress={isEntryInProgress}
                      allowedLinks={allAllowedLinks}
                      isLeoModel={conversationContext.isLeoModel}
                    />
                  )}
                  {isHuman && !turn.selectedText && !showEditInput && (
                    <>
                      {hoverMenuButtonId === id ? (
                        <ContextMenuHuman
                          isOpen={activeMenuId === id}
                          onClick={() => showHumanMenu(id)}
                          onClose={hideHumanMenu}
                          onEditQuestionClicked={() => setEditInputId(id)}
                          onCopyQuestionClicked={handleCopyText}
                        />
                      ) : (
                        <div className={styles.divToKeepGap} />
                      )}
                      <div className={styles.humanMessageBubble}>
                        <div className={styles.humanTextRow}>
                          {latestTurnText}
                          {latestEdit && (
                            <div className={styles.editLabel}>
                              <span className={styles.editLabelText}>
                                {getLocale('editedLabel')}
                              </span>
                            </div>
                          )}
                        </div>
                        <div className={styles.uploadedImages}>
                          {!!latestTurn.uploadedImages?.length &&
                            latestTurn.uploadedImages.map((img) => (
                              <UploadedImgItem
                              key={img.filename}
                              uploadedImage={img}
                              />
                          ))}
                        </div>
                      </div>
                    </>
                  )}
                  {showEditInput && (
                    <EditInput
                      text={latestTurnText}
                      onSubmit={(text) => handleEditSubmit(id, text)}
                      onCancel={() => setEditInputId(undefined)}
                      isSubmitDisabled={
                        !conversationContext.canSubmitUserEntries
                      }
                    />
                  )}
                  {turn.selectedText && (
                    <ActionTypeLabel actionType={turn.actionType} />
                  )}
                  {turn.selectedText && <Quote text={turn.selectedText} />}
                  {showLongPageContentInfo && <LongPageInfo />}
                </div>
                {isAIAssistant && showEditIndicator && (
                  <EditIndicator time={lastEditedTime} />
                )}
                {isAIAssistant &&
                  conversationContext.isLeoModel &&
                  !turn.selectedText &&
                  !showEditInput && (
                    <ContextActionsAssistant
                      turnUuid={turn.uuid}
                      onEditAnswerClicked={() => setEditInputId(id)}
                      onCopyTextClicked={handleCopyText}
                    />
                  )}
              </div>
            </div>
          )
        })}
      </div>
    </>
  )
}

export default ConversationEntries

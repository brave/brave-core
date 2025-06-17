/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import useLongPress from '$web-common/useLongPress'
import * as Mojom from '../../../common/mojom'
import ActionTypeLabel from '../../../common/components/action_type_label'
import {  AttachmentImageItem, AttachmentPageItem  } from '../../../page/components/attachment_item'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import AssistantReasoning from '../assistant_reasoning'
import ContextActionsAssistant from '../context_actions_assistant'
import ContextMenuHuman from '../context_menu_human'
import Quote from '../quote'
import LongPageInfo from '../page_context_message/long_page_info'
import AssistantResponse from '../assistant_response'
import EditInput from '../edit_input'
import EditIndicator from '../edit_indicator'
import { getReasoningText, groupConversationEntries } from './conversation_entries_utils'
import { getImageFiles } from '../../../common/conversation_history_utils'
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

  // Render events from consecutive assistant entries in the same parent element for style purposes.
  // Keep their grouping so that we can still know which events are part of "active" entries.
  const groupedEntries = React.useMemo<Mojom.ConversationTurn[][]>(
    () => groupConversationEntries(conversationContext.conversationHistory),
    [conversationContext.conversationHistory]
  )

  console.log('groupedEntries', groupedEntries)

  return (
    <>
      <div>
        {groupedEntries.map((group, index) => {
          const turn = group[0]
          const latestEdit = turn.edits?.at(-1)
          const latestTurn = latestEdit ?? turn
          const lastEditedTime = latestTurn.createdTime
          const isLastEntry = (index === groupedEntries.length - 1)
          const isAIAssistant =
            turn.characterType === Mojom.CharacterType.ASSISTANT
          const isEntryInProgress =
            isLastEntry && isAIAssistant && conversationContext.isGenerating
          const isHuman = turn.characterType === Mojom.CharacterType.HUMAN
          const isGeneratingResponse =
            isHuman && isLastEntry && conversationContext.isGenerating
          const showLongPageContentInfo =
            index === 1 &&
            isAIAssistant &&
            ((conversationContext.contentUsedPercentage ?? 100) < 100 ||
              (conversationContext.trimmedTokens > 0 && conversationContext.totalTokens > 0))
          // TODO(petemill): editInputId should be uuid of entry
          const showEditInput = editInputId === index
          const showEditIndicator = !showEditInput && !!turn.edits?.length

          // Can't edit or copy complicated structured content
          const canEditOrCopyContent = group.length === 1 &&
              !group[0].events?.some(event => !!event.toolUseEvent)

          const turnModelKey = turn.modelKey
            ? conversationContext.allModels
                .find(m => m.key === turn.modelKey)
                ?.key ?? undefined
            : undefined;

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
              navigator.clipboard.writeText(latestTurn.text)
            }
          }

          const imageFiles =
            getImageFiles(latestTurn.uploadedFiles) || []

          const hasImageAttachments = imageFiles.length > 0
          const hasTabAttachments = index === 0 && conversationContext.associatedContent.length > 0
          const hasAttachments = hasImageAttachments || hasTabAttachments

          return (
            <div
              key={turn.uuid || index}
              className={turnContainer}
            >
              <div
                data-id={index}
                className={turnClass}
                onMouseEnter={() => isHuman && setHoverMenuButtonId(index)}
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
                  {isAIAssistant && !showEditInput &&
                    group.map((entry, i) => {
                      const isThisGroupInProgress = isEntryInProgress && (i === group.length - 1)
                      const isThisGroupActive = isLastEntry && (i === group.length - 1)
                      const latestEdit = entry.edits?.at(-1)
                      const latestTurn = latestEdit ?? entry
                      const allowedLinksForTurn: string[] =
                        latestTurn.events?.flatMap(event =>
                          event.sourcesEvent?.sources?.map(
                            source => source.url.url
                          ) || []
                        ) || []
                      const latestTurnText = getCompletion(latestTurn)
                      const hasReasoning = latestTurnText.includes('<think>')
                      console.log('entry', entry)
                      return (
                        <React.Fragment key={entry.uuid || i}>
                          {hasReasoning && (
                            <AssistantReasoning
                              text={getReasoningText(latestTurnText)}
                              isReasoning={
                                isEntryInProgress &&
                                !latestTurnText.includes('</think>')
                              }
                            />
                          )}
                          <AssistantResponse
                            key={entry.uuid || i}
                            events={entry.events!.filter(Boolean)}
                            isEntryActive={isThisGroupActive}
                            isEntryInProgress={isThisGroupInProgress}
                            allowedLinks={allowedLinksForTurn}
                            isLeoModel={conversationContext.isLeoModel}
                          />
                        </React.Fragment>
                      )
                    })
                  }
                  {isHuman && !turn.selectedText && !showEditInput && (
                    <>
                      {hoverMenuButtonId === index ? (
                        <ContextMenuHuman
                          isOpen={activeMenuId === index}
                          onClick={() => showHumanMenu(index)}
                          onClose={hideHumanMenu}
                          onEditQuestionClicked={() => setEditInputId(index)}
                          onCopyQuestionClicked={handleCopyText}
                        />
                      ) : (
                        <div className={styles.divToKeepGap} />
                      )}
                      <div className={styles.humanMessageBubble}>
                        <div className={styles.humanTextRow}>
                          {latestTurn.text}
                          {latestEdit && (
                            <div className={styles.editLabel}>
                              <span className={styles.editLabelText}>
                                {getLocale(S.CHAT_UI_EDITED_LABEL)}
                              </span>
                            </div>
                          )}
                        </div>
                        {hasAttachments &&
                          <div className={styles.attachmentsContainer}>
                          {conversationContext.associatedContent.map(c => <AttachmentPageItem
                            key={c.contentId}
                            url={c.url.url}
                            title={c.title}
                          />)}
                            {imageFiles.map((img) => (
                              <AttachmentImageItem
                                key={img.filename}
                                uploadedImage={img}
                              />
                            ))}
                          </div>
                        }
                      </div>
                    </>
                  )}
                  {showEditInput && (
                    <EditInput
                      text={latestTurn.text}
                      onSubmit={(text) => handleEditSubmit(index, text)} // TODO: uuid
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
                      turnModelKey={turnModelKey}
                      onEditAnswerClicked={canEditOrCopyContent ? () => setEditInputId(index) : undefined}
                      onCopyTextClicked={canEditOrCopyContent ? handleCopyText : undefined}
                    />
                  )}
                {isGeneratingResponse && (
                  <div className={styles.loading}><ProgressRing /></div>
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

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
import styles from './style.module.scss'

const getReasoningText = (text: string) => {
  const startTag = `<think>`
  const endTag = `</think>`
  const startTagIndex = text.indexOf(startTag) + startTag.length
  const endTagIndex = text.indexOf(endTag, startTagIndex)
  return text.slice(startTagIndex, endTagIndex).trim()
}

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

  // Combine entry events if there are multiple assistant character types in a row.
  // If we stopped to handle a tool, it should still look like the Assistant did it.
  // TODO: this is not efficient, optimize by rendering so that
  // const combinedEntries = React.useMemo<Mojom.ConversationTurn[]>(() => {
  //   const combinedEntries: Mojom.ConversationTurn[] = []
  //   for (const entry of conversationContext.conversationHistory) {
  //     const lastEntry = combinedEntries[combinedEntries.length - 1]
  //     if (combinedEntries.length === 0 || entry.characterType !== Mojom.CharacterType.ASSISTANT || lastEntry.characterType !== Mojom.CharacterType.ASSISTANT) {
  //       combinedEntries.push({...entry})
  //       continue
  //     }
  //     if (entry.characterType === Mojom.CharacterType.ASSISTANT && lastEntry.characterType === Mojom.CharacterType.ASSISTANT) {
  //       lastEntry.events = [...(lastEntry.events ?? []), ...(entry.events ?? [])]
  //       continue
  //     }
  //   }
  //   return combinedEntries
  // }, [conversationContext.conversationHistory])

  return (
    <>
      <div className={styles.conversationEntries}>
        {conversationContext.conversationHistory.map((turn, id) => {
          const nextEntry = conversationContext.conversationHistory[id + 1]
          const previousEntry = conversationContext.conversationHistory[id - 1]
          const isNextEntryAIAssistant = nextEntry?.characterType === Mojom.CharacterType.ASSISTANT
          const isPreviousEntryAIAssistant = previousEntry?.characterType === Mojom.CharacterType.ASSISTANT
          const isLastEntry = !nextEntry
          const isAIAssistant =
            turn.characterType === Mojom.CharacterType.ASSISTANT
          const isEntryInProgress =
            isLastEntry && isAIAssistant && conversationContext.isGenerating
          const isHuman = turn.characterType === Mojom.CharacterType.HUMAN
          const showLongPageContentInfo =
            id === 1 &&
            isAIAssistant &&
            (conversationContext.contentUsedPercentage ?? 100) < 100
          const showEditInput = editInputId === id
          const showEditIndicator = !showEditInput && !!turn.edits?.length
          const latestEdit = turn.edits?.at(-1)
          const latestTurn = latestEdit ?? turn
          const latestTurnText = isAIAssistant
            ? getCompletion(latestTurn)
            : latestTurn.text
          const lastEditedTime = latestTurn.createdTime
          const hasReasoning = latestTurnText.includes('<think>')

          // Since we're combining turns, we don't yet have the ability
          // to combine edits or copy. These entries are generally
          // too complicated for those actions anyway since they contain
          // structured tool use events.
          const canEditOrCopyContent = !isAIAssistant || !isPreviousEntryAIAssistant

          const turnClass = classnames({
            [styles.turn]: true,
            [styles.turnAI]: isAIAssistant,
            [styles.isMobile]: conversationContext.isMobile
            // [styles.isCombinedWithPrevious]: isAIAssistant && isPreviousEntryAIAssistant
          })

          const handleCopyText = () => {
            if (isAIAssistant) {
              // TODO(petemill): Combine multiple completion events from turn
              // and combine multiple assistant turns since they
              // are rendered as one turn. Or don't offer to copy if
              // non-completion events are present, which is the only way both
              // scenarios can occur.
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
                data-id={id}
                key={turn.uuid || id}
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
                        {!!latestTurn.uploadedImages?.length &&
                          latestTurn.uploadedImages.map((img) => (
                            <UploadedImgItem
                              key={img.filename}
                              uploadedImage={img}
                            />
                          ))}
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
                  !isNextEntryAIAssistant &&
                  conversationContext.isLeoModel &&
                  !turn.selectedText &&
                  !showEditInput && (
                    <ContextActionsAssistant
                      turnUuid={turn.uuid}
                      onEditAnswerClicked={canEditOrCopyContent ? () => setEditInputId(id) : undefined}
                      onCopyTextClicked={canEditOrCopyContent ? handleCopyText : undefined}
                    />
                  )}
              </div>
          )
        })}
      </div>
    </>
  )
}

export default ConversationEntries

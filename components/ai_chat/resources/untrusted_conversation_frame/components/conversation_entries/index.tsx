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

  // Combine entry events if there are multiple assistant character types in a row.
  // If we stopped to handle a tool, it should still look like the Assistant did it.
  const groupedEntries = React.useMemo<Mojom.ConversationTurn[][]>(() => {
    const groupedEntries: Mojom.ConversationTurn[][] = []
    for (const entry of conversationContext.conversationHistory) {
      const lastEntry = groupedEntries[groupedEntries.length - 1]
      if (!groupedEntries.length || !lastEntry?.length || entry.characterType !== Mojom.CharacterType.ASSISTANT || lastEntry[0]?.characterType !== Mojom.CharacterType.ASSISTANT) {
        groupedEntries.push([entry])
        continue
      }
      if (entry.characterType === Mojom.CharacterType.ASSISTANT && lastEntry[0]?.characterType === Mojom.CharacterType.ASSISTANT) {
        groupedEntries[groupedEntries.length - 1].push(entry)
        continue
      }
    }
    return groupedEntries
  }, [conversationContext.conversationHistory])

  return (
    <>
      <div className={styles.conversationEntries}>
      {groupedEntries.map((group, index) => {
          const turn = group[0]
          const isLastEntry = (index === (groupedEntries.length - 1))
          const isAIAssistant =
            turn.characterType === Mojom.CharacterType.ASSISTANT
          const isEntryInProgress =
            isLastEntry && isAIAssistant && conversationContext.isGenerating
          const isHuman = turn.characterType === Mojom.CharacterType.HUMAN
          const showLongPageContentInfo =
            index === 1 &&
            isAIAssistant &&
            (conversationContext.contentUsedPercentage ?? 100) < 100
          // TODO: editId should be uuid of entry
          const showEditInput = editInputId === index
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
          const canEditOrCopyContent = group.length === 1

          const turnClass = classnames({
            [styles.turn]: true,
            [styles.turnAI]: isAIAssistant,
            [styles.isMobile]: conversationContext.isMobile
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
                data-id={index}
                key={turn.uuid || index}
                className={turnClass}
                onMouseEnter={() => isHuman && setHoverMenuButtonId(index)} // TODO: uuid
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
                  {isAIAssistant && !showEditInput && group.length && (
                    <AssistantResponse
                      events={group.flatMap((entry) => entry.events!).filter(Boolean)}
                      isLastEntry={isLastEntry}
                    />
                  )}
                  {isHuman && !turn.selectedText && !showEditInput && (
                    <>
                      {hoverMenuButtonId === index ? (
                        <ContextMenuHuman
                          isOpen={activeMenuId === index} // TODO: uuid
                          onClick={() => showHumanMenu(index)} // TODO: uuid
                          onClose={hideHumanMenu}
                          onEditQuestionClicked={() => setEditInputId(index)} // TODO: uuid
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
                      onEditAnswerClicked={canEditOrCopyContent ? () => setEditInputId(index) : undefined} // TODO: uuid
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

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'
import Icon from '@brave/leo/react/icon'
import useLongPress from '$web-common/useLongPress'
import * as Mojom from '../../../common/mojom'
import ContextMenuAssistant from '../context_menu_assistant'
import { getLocale } from '$web-common/locale'
import Quote from '../quote'
import ActionTypeLabel from '../action_type_label'
import LongPageInfo from '../page_context_message/long_page_info'
import AssistantResponse from '../assistant_response'
import styles from './style.module.scss'
import CopyButton from '../copy_button'
import EditButton from '../edit_button'
import EditInput from '../edit_input'
import EditIndicator from '../edit_indicator'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'

interface ConversationEntriesProps {
  onLastElementHeightChange: () => void
}

function ConversationEntries(props: ConversationEntriesProps) {
  const conversationContext = useUntrustedConversationContext()

  const lastEntryElementRef = React.useRef<HTMLDivElement>(null)
  const [activeMenuId, setActiveMenuId] = React.useState<number | null>()
  const [editInputId, setEditInputId] = React.useState<number | null>()

  const handleEditSubmit = (index: number, text: string) => {
    conversationContext.conversationHandler?.modifyConversation(index, text)
    setEditInputId(null)
  }

  const showAssistantMenu = (id: number) => {
    setActiveMenuId(id)
  }

  const hideAssistantMenu = () => {
    setActiveMenuId(null)
  }

  const longPressProps = useLongPress({
    onLongPress: (e: React.TouchEvent) => {
      const currentTarget = e.currentTarget as HTMLElement
      const id = currentTarget.getAttribute('data-id')
      if (id === null) return
      showAssistantMenu(parseInt(id))
    },
    onTouchMove: () => setActiveMenuId(null)
  })

  React.useEffect(() => {
    if (lastEntryElementRef.current === null) return
    if (!conversationContext.isGenerating) return
    props.onLastElementHeightChange()
  }, [
    conversationContext.conversationHistory.length,
    lastEntryElementRef.current?.clientHeight
  ])

  const lastAssistantId = React.useMemo(() => {
    // Get the last entry that is an assistant entry
    for (
      let i = conversationContext.conversationHistory.length - 1;
      i >= 0;
      i--
    ) {
      if (
        conversationContext.conversationHistory[i].characterType ===
        Mojom.CharacterType.ASSISTANT
      ) {
        return i
      }
    }
    return -1
  }, [conversationContext.conversationHistory])

  const getCompletion = (turn: Mojom.ConversationTurn) => {
    const event = turn.events?.find((event) => event.completionEvent)
    return event?.completionEvent?.completion ?? ''
  }

  return (
    <>
      <div>
        {conversationContext.conversationHistory.map((turn, id) => {
          const isLastEntry = id === lastAssistantId
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

          const turnContainer = classnames({
            [styles.turnContainerMobile]: conversationContext.isMobile,
            [styles.turnContainerHighlight]:
              isAIAssistant && activeMenuId === id
          })

          const turnClass = classnames({
            [styles.turn]: true,
            [styles.turnAI]: isAIAssistant
          })

          const avatarStyles = classnames({
            [styles.avatar]: true,
            [styles.avatarAI]: isAIAssistant
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
              ref={isLastEntry ? lastEntryElementRef : null}
            >
              <div
                data-id={id}
                className={turnClass}
                onMouseLeave={
                  isAIAssistant ? () => setActiveMenuId(null) : undefined
                }
                {...(isAIAssistant ? longPressProps : {})}
              >
                <div className={styles.turnHeader}>
                  <div className={styles.avatarContainer}>
                    <div className={avatarStyles}>
                      <Icon
                        name={isHuman ? 'user-circle' : 'product-brave-leo'}
                      />
                    </div>
                    <span>{isHuman ? 'You' : 'Leo'}</span>
                  </div>
                  {!turn.selectedText && (
                    <div className={styles.rightContainer}>
                      {latestEdit && (
                        <div className={styles.editLabel}>
                          <span className={styles.editLabelText}>
                            {getLocale('editedLabel')}
                          </span>
                        </div>
                      )}
                      <div className={styles.turnActions}>
                        <CopyButton onClick={handleCopyText} />
                        {!isAIAssistant && (
                          <EditButton onClick={() => setEditInputId(id)} />
                        )}
                        {isAIAssistant &&
                          conversationContext.isLeoModel && (
                            <ContextMenuAssistant
                              turnUuid={turn.uuid}
                              isOpen={activeMenuId === id}
                              onClick={() => showAssistantMenu(id)}
                              onClose={hideAssistantMenu}
                              onEditAnswerClicked={() => setEditInputId(id)}
                            />
                          )}
                      </div>
                    </div>
                  )}
                </div>
                <div className={styles.message}>
                  {isAIAssistant && !showEditInput && (
                    <AssistantResponse
                      entry={latestTurn}
                      isEntryInProgress={isEntryInProgress}
                    />
                  )}
                  {!isAIAssistant && !turn.selectedText && !showEditInput && (
                    <span className={styles.humanMessageContent}>
                      {latestTurnText}
                    </span>
                  )}
                  {showEditIndicator && <EditIndicator time={lastEditedTime} />}
                  {showEditInput && (
                    <EditInput
                      text={latestTurnText}
                      onSubmit={(text) => handleEditSubmit(id, text)}
                      onCancel={() => setEditInputId(null)}
                      isSubmitDisabled={!conversationContext.canSubmitUserEntries}
                    />
                  )}
                  {turn.selectedText && (
                    <ActionTypeLabel actionType={turn.actionType} />
                  )}
                  {turn.selectedText && <Quote text={turn.selectedText} />}
                  {showLongPageContentInfo && <LongPageInfo />}
                </div>
              </div>
            </div>
          )
        })}
      </div>
    </>
  )
}

export default ConversationEntries

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Label from '@brave/leo/react/label'
import ProgressRing from '@brave/leo/react/progressRing'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import ActionTypeLabel from '../../../common/components/action_type_label'
import {
  AttachmentPageItem,
  AttachmentUploadItems,
} from '../../../page/components/attachment_item'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import AssistantReasoning from '../assistant_reasoning'
import ContextActionsAssistant from '../context_actions_assistant'
import ContextMenuHuman from '../context_menu_human'
import Quote from '../quote'
import {
  LongPageContentWarning,
  LongTextContentWarning,
  LongVisualContentWarning,
} from '../page_context_message/long_page_info'
import AssistantResponse from '../assistant_response'
import EditInput from '../edit_input'
import EditIndicator from '../edit_indicator'
import {
  getReasoningText,
  getToolArtifacts,
  groupConversationEntries,
  isAssistantGroupTask,
} from './conversation_entries_utils'
import useConversationEventClipboardCopyHandler from './use_conversation_event_clipboard_copy_handler'
import styles from './style.module.scss'
import AssistantTask from '../assistant_task/assistant_task'

// Function to highlight skill shortcuts in text
const maybeHighlightSkillText = (text: string, skill?: Mojom.SkillEntry) => {
  if (!skill) return text

  const shortcutPattern = `/${skill.shortcut}`
  const index = text.indexOf(shortcutPattern)

  if (index === -1) return text

  return (
    <span>
      {text.substring(0, index)}
      <Label
        className={styles.skillLabel}
        color='primary'
        mode='default'
      >
        {shortcutPattern}
      </Label>
      {text.substring(index + shortcutPattern.length)}
    </span>
  )
}

function usePairedConversationGroups() {
  const conversationContext = useUntrustedConversationContext()
  // Render events from consecutive assistant entries in the same parent element for both
  // semantic and style purposes. The user should see them as a single entry since
  // the back and forth could be considered an implementation detail when
  // conceptually it's more like a continuation of a response without human interaction
  // in-between.
  const groupedEntries = React.useMemo<Mojom.ConversationTurn[][]>(
    () => groupConversationEntries(conversationContext.conversationHistory),
    [conversationContext.conversationHistory],
  )

  // This pairs a human turn with the following (potentially grouped) assistant turn.
  const pairedEntries = React.useMemo(() => {
    const result: Mojom.ConversationTurn[][][] = []
    for (let i = 0; i < groupedEntries.length; i++) {
      const entry = groupedEntries[i][0]

      // Free hanging assistant turn - just leave it be.
      if (entry.characterType !== Mojom.CharacterType.HUMAN) {
        result.push([groupedEntries[i]])
        continue
      }

      const nextEntry = groupedEntries[i + 1]?.[0]
      // Free hanging or last human turn - just leave it be.
      if (
        !nextEntry
        || nextEntry.characterType !== Mojom.CharacterType.ASSISTANT
      ) {
        result.push([groupedEntries[i]])
        continue
      }

      // Pair the human turn with the grouped assistant turns.
      result.push([groupedEntries[i], groupedEntries[i + 1]])
      // we've already processed the next entry, so skip it.
      i++
    }
    return result
  }, [groupedEntries])

  return pairedEntries
}

function scrollEntryPairToTop(el: HTMLDivElement | null) {
  el?.scrollIntoView({ block: 'start', behavior: 'smooth' })
}

function ConversationEntries() {
  const conversationContext = useUntrustedConversationContext()

  const [hoverMenuButtonId, setHoverMenuButtonId] = React.useState<number>()
  const [activeMenuId, setActiveMenuId] = React.useState<number>()
  const [editInputId, setEditInputId] = React.useState<number>()
  const entryPairs = usePairedConversationGroups()
  const hasGenerated = React.useRef(false)
  hasGenerated.current =
    hasGenerated.current || conversationContext.isGenerating

  const showHumanMenu = (id: number) => {
    setActiveMenuId(id)
  }

  const hideHumanMenu = () => {
    setActiveMenuId(undefined)
  }

  const getCompletion = (turn: Mojom.ConversationTurn) => {
    const event = turn.events?.find((event) => event.completionEvent)
    return event?.completionEvent?.completion ?? ''
  }

  const handleEditSubmit = (index: number, text: string) => {
    const entryUuid = conversationContext.conversationHistory[index]?.uuid
    if (!entryUuid) return
    conversationContext.conversationHandler?.modifyConversation(entryUuid, text)
    setEditInputId(undefined)
  }

  function renderEntryGroup(
    isLastGroup: boolean,
    group: Mojom.ConversationTurn[],
    entryNumber: number,
  ) {
    // TODO(https://github.com/brave/brave-browser/issues/50132):
    // Split this component up to make it more readable.
    const firstEntry = group[0]
    const firstEntryEdit = firstEntry.edits?.at(-1) ?? group[0]
    const lastEditedTime = firstEntryEdit.createdTime
    const isAIAssistant =
      firstEntryEdit.characterType === Mojom.CharacterType.ASSISTANT
    const isEntryInProgressButGroup =
      isLastGroup && isAIAssistant && conversationContext.isGenerating
    const isHuman = firstEntryEdit.characterType === Mojom.CharacterType.HUMAN
    const isGeneratingResponse =
      isHuman && isLastGroup && conversationContext.isGenerating
    const showLongPageContentInfo =
      entryNumber === 1
      && isAIAssistant
      && ((conversationContext.contentUsedPercentage ?? 100) < 100
        || (conversationContext.trimmedTokens > 0
          && conversationContext.totalTokens > 0)
        || (conversationContext.visualContentUsedPercentage ?? 100) < 100)

    const showEditInput = editInputId === entryNumber
    const showEditIndicator = !showEditInput && !!group[0].edits?.length
    const turnIds = new Set([
      firstEntry.uuid,
      ...(firstEntry.edits?.map((edit) => edit.uuid) ?? []),
    ])

    // Can't edit complicated structured content (for now)
    // Not allowed to edit agent conversations until the edit submission
    // happens on a different frame.
    const canEditEntry =
      !conversationContext.conversationCapabilities.includes(
        Mojom.ConversationCapability.CONTENT_AGENT,
      )
      && group.length === 1
      && !firstEntryEdit.events?.some((event) => !!event.toolUseEvent)

    const turnModelKey = firstEntryEdit.modelKey
      ? (conversationContext.allModels.find(
          (m) => m.key === firstEntryEdit.modelKey,
        )?.key ?? undefined)
      : undefined

    const turnClass = classnames({
      [styles.turn]: true,
      [styles.turnAI]: isAIAssistant,
    })

    const handleCopyText = useConversationEventClipboardCopyHandler(group)

    const tabAttachments =
      conversationContext.associatedContent?.filter((c) =>
        turnIds.has(c.conversationTurnUuid),
      ) ?? []
    const hasAttachments =
      !!firstEntryEdit.uploadedFiles?.length || tabAttachments.length > 0

    const groupIsTask = isAssistantGroupTask(group)

    // Omit artifacts until generation is complete so we show
    // the artifacts and the final response text at the same time.
    const shouldOmitToolArtifacts =
      isLastGroup && conversationContext.isGenerating
    const toolArtifacts = !shouldOmitToolArtifacts
      ? getToolArtifacts(group)
      : null

    return (
      <div key={firstEntryEdit.uuid || entryNumber}>
        <div
          data-id={entryNumber}
          data-testid={isHuman ? 'human-turn' : 'assistant-turn'}
          className={turnClass}
          onMouseEnter={() => isHuman && setHoverMenuButtonId(entryNumber)}
          onMouseLeave={() => isHuman && setHoverMenuButtonId(undefined)}
        >
          <div className={isAIAssistant ? styles.message : styles.humanMessage}>
            {groupIsTask && (
              <AssistantTask
                assistantEntries={group}
                isActiveTask={isLastGroup}
                isLeoModel={conversationContext.isLeoModel}
              />
            )}
            {!groupIsTask
              && group.map((entry, i) => {
                const isEntryInProgress =
                  isEntryInProgressButGroup && i === group.length - 1
                const isLastEntryInLastGroup =
                  isLastGroup && i === group.length - 1
                const currentEntryEdit = entry.edits?.at(-1) ?? entry
                const allowedLinksForEntry: string[] =
                  currentEntryEdit.events?.flatMap(
                    (event) =>
                      event.sourcesEvent?.sources?.map(
                        (source) => source.url.url,
                      ) || [],
                  ) || []
                const entryText = getCompletion(currentEntryEdit)
                const hasReasoning = entryText.includes('<think>')

                return (
                  <React.Fragment key={entry.uuid || i}>
                    {isAIAssistant && !showEditInput && (
                      <>
                        {hasReasoning && (
                          <AssistantReasoning
                            text={getReasoningText(entryText)}
                            isReasoning={
                              isEntryInProgressButGroup
                              && !entryText.includes('</think>')
                            }
                          />
                        )}
                        <AssistantResponse
                          key={entry.uuid || i}
                          events={
                            currentEntryEdit.events?.filter(Boolean) ?? []
                          }
                          isEntryInteractivityAllowed={isLastEntryInLastGroup}
                          isEntryInProgress={isEntryInProgress}
                          allowedLinks={allowedLinksForEntry}
                          isLeoModel={conversationContext.isLeoModel}
                          toolArtifacts={
                            i === group.length - 1 ? toolArtifacts : null
                          }
                        />
                      </>
                    )}
                    {isHuman
                      && !firstEntryEdit.selectedText
                      && !showEditInput && (
                        <>
                          <ContextMenuHuman
                            isOpen={activeMenuId === entryNumber}
                            isVisible={
                              conversationContext.isMobile
                              || hoverMenuButtonId === entryNumber
                              || activeMenuId === entryNumber
                            }
                            onClick={() => showHumanMenu(entryNumber)}
                            onClose={hideHumanMenu}
                            onEditQuestionClicked={
                              canEditEntry
                                ? () => setEditInputId(entryNumber)
                                : undefined
                            }
                            onCopyQuestionClicked={handleCopyText}
                            onSaveAsSkillClicked={() =>
                              conversationContext.parentUiFrame?.showSkillDialog(
                                firstEntryEdit.text,
                              )
                            }
                          />
                          <div className={styles.humanMessageBubble}>
                            <div className={styles.humanTextRow}>
                              {maybeHighlightSkillText(
                                currentEntryEdit.text,
                                currentEntryEdit.skill,
                              )}
                              {!!entry.edits?.length && (
                                <div className={styles.editLabel}>
                                  <span className={styles.editLabelText}>
                                    {getLocale(S.CHAT_UI_EDITED_LABEL)}
                                  </span>
                                </div>
                              )}
                            </div>
                            {hasAttachments && (
                              <div className={styles.attachmentsContainer}>
                                {tabAttachments.map((c) => (
                                  <AttachmentPageItem
                                    key={c.contentId}
                                    url={c.url.url}
                                    title={c.title}
                                  />
                                ))}
                                <AttachmentUploadItems
                                  uploadedFiles={
                                    firstEntryEdit.uploadedFiles || []
                                  }
                                />
                              </div>
                            )}
                          </div>
                        </>
                      )}

                    {showEditInput && (
                      <EditInput
                        text={firstEntryEdit.text}
                        onSubmit={(text) => handleEditSubmit(entryNumber, text)}
                        onCancel={() => setEditInputId(undefined)}
                        isSubmitDisabled={
                          !conversationContext.canSubmitUserEntries
                        }
                      />
                    )}
                    {firstEntryEdit.selectedText && (
                      <div className={styles.selectedTextContext}>
                        <ActionTypeLabel
                          actionType={firstEntryEdit.actionType}
                        />
                        <Quote text={firstEntryEdit.selectedText} />
                      </div>
                    )}
                    {showLongPageContentInfo
                      && (() => {
                        if (
                          conversationContext.trimmedTokens > 0
                          && conversationContext.totalTokens > 0
                        ) {
                          const percentage =
                            100
                            - Math.floor(
                              (Number(conversationContext.trimmedTokens)
                                / Number(conversationContext.totalTokens))
                                * 100,
                            )
                          return (
                            <LongTextContentWarning
                              percentageUsed={percentage}
                            />
                          )
                        } else if (
                          (conversationContext.visualContentUsedPercentage
                            ?? 100) < 100
                        ) {
                          return (
                            <LongVisualContentWarning
                              visualContentUsedPercentage={
                                conversationContext.visualContentUsedPercentage!
                              }
                            />
                          )
                        } else if (
                          (conversationContext.contentUsedPercentage ?? 100)
                          < 100
                        ) {
                          return (
                            <LongPageContentWarning
                              contentUsedPercentage={
                                conversationContext.contentUsedPercentage!
                              }
                            />
                          )
                        }
                        return null
                      })()}
                  </React.Fragment>
                )
              })}
          </div>

          {!groupIsTask && (
            <>
              {isAIAssistant && showEditIndicator && (
                <EditIndicator time={lastEditedTime} />
              )}
              {isAIAssistant
                && conversationContext.isLeoModel
                && !firstEntryEdit.selectedText
                && !showEditInput && (
                  <ContextActionsAssistant
                    turnUuid={firstEntryEdit.uuid}
                    turnModelKey={turnModelKey}
                    turnNEARVerified={
                      group.at(-1)?.nearVerificationStatus?.verified
                    }
                    onEditAnswerClicked={
                      canEditEntry
                        ? () => setEditInputId(entryNumber)
                        : undefined
                    }
                    onCopyTextClicked={handleCopyText}
                  />
                )}
            </>
          )}
          {isGeneratingResponse && (
            <div className={styles.loading}>
              <ProgressRing />
            </div>
          )}
        </div>
      </div>
    )
  }

  let entryNumber = 0

  return (
    <div className={hasGenerated.current ? styles.hasGenerated : ''}>
      {entryPairs.map((pair, pairIndex) => (
        <div
          key={pairIndex}
          className={styles.entryPair}
          ref={
            pairIndex === entryPairs.length - 1 && hasGenerated.current
              ? scrollEntryPairToTop
              : undefined
          }
        >
          {pair.map((group, groupIndex) =>
            renderEntryGroup(
              pairIndex === entryPairs.length - 1
                && groupIndex === pair.length - 1,
              group,
              // Note, we need to keep track of the entry number across pairs and groups.
              entryNumber++,
            ),
          )}
        </div>
      ))}
    </div>
  )
}

export default ConversationEntries

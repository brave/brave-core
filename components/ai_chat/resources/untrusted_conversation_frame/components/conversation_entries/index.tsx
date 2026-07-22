/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import classnames from '$web-common/classnames'
import { getLocale } from '$web-common/locale'
import * as Mojom from '../../../common/mojom'
import { getActionTypeLabel } from '../../../common/components/action_type_label'
import {
  AttachmentPageItem,
  AttachmentUploadItems,
} from '../../../page/components/attachment_item'
import {
  Content,
  stringifyContent,
} from '../../../page/components/input_box/editable_content'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import AssistantReasoning from '../assistant_reasoning'
import ContextActionsAssistant from '../context_actions_assistant'
import ContextMenuHuman from '../context_menu_human'
import {
  LongPageContentWarning,
  LongTextContentWarning,
  LongVisualContentWarning,
} from '../page_context_message/long_page_info'
import AssistantResponse from '../assistant_response'
import EditInput from '../edit_input'
import EditIndicator from '../edit_indicator'
import {
  getGroupAllowedLinks,
  getReasoningText,
  getToolArtifacts,
  groupConversationEntries,
  isAssistantGroupTask,
} from './conversation_entries_utils'
import useConversationEventClipboardCopyHandler from './use_conversation_event_clipboard_copy_handler'
import styles from './style.module.scss'
import AssistantTask from '../assistant_task/assistant_task'
import ProgressBubble, {
  ProgressBubbleContextProvider,
} from '../progress_bubble/progress_bubble'

const escape = (text: string): string => (RegExp as any).escape(text)

type RichSegment =
  | { type: 'text'; value: string }
  | { type: 'skill'; id: string; text: string }
  | { type: 'attachment'; content: Mojom.AssociatedContent }

const parseRichSegments = (
  text: string,
  skill: Mojom.SkillEntry | undefined | null,
  associatedContent: Mojom.AssociatedContent[],
): RichSegment[] => {
  const replacements: string[] = []

  // Skills can only be at the beginning of the text
  if (skill) {
    replacements.push(`^${escape(`/${skill.shortcut}`)}`)
  }

  replacements.push(
    ...associatedContent.map((c) => escape(`[mention(${c.title})]`)),
  )

  if (!replacements.length) return [{ type: 'text', value: text }]

  const regex = new RegExp(replacements.map((r) => `(${r})`).join('|'), 'g')

  const segments: RichSegment[] = []
  let lastIndex = 0
  for (const match of text.matchAll(regex)) {
    if (match.index > lastIndex) {
      segments.push({
        type: 'text',
        value: text.substring(lastIndex, match.index),
      })
    }
    lastIndex = match.index + match[0].length

    const content = associatedContent.find(
      (c) => match[0] === `[mention(${c.title})]`,
    )
    if (content) {
      segments.push({ type: 'attachment', content })
    } else {
      segments.push({ type: 'skill', id: skill!.shortcut, text: match[0] })
    }
  }
  if (lastIndex < text.length) {
    segments.push({ type: 'text', value: text.substring(lastIndex) })
  }

  return segments
}

export const highlightRichText = (
  text: string,
  skill: Mojom.SkillEntry | undefined,
  associatedContent: Mojom.AssociatedContent[],
) => {
  const segments = parseRichSegments(text, skill, associatedContent)

  if (segments.every((s) => s.type === 'text')) return text

  return (
    <>
      {segments.map((seg, i) => {
        if (seg.type === 'text') return seg.value
        return (
          <span
            key={i}
            className={styles.richLabel}
            title={seg.type === 'attachment' ? seg.content.url.url : undefined}
          >
            {seg.type === 'attachment' && (
              <img
                src={`chrome-untrusted://favicon2?size=64&pageUrl=${encodeURIComponent(seg.content.url.url)}`}
              />
            )}
            <span className={styles.richLabelTitle}>
              {seg.type === 'attachment' ? seg.content.title : seg.text}
            </span>
          </span>
        )
      })}
    </>
  )
}

// Renders an action (e.g. "Explain") as an inline highlighted chip, matching
// the way skills are rendered.
const renderActionLabel = (actionType: Mojom.ActionType) => {
  const label = getActionTypeLabel(actionType)
  if (!label) return null
  return (
    <span className={styles.richLabel}>
      <span className={styles.richLabelTitle}>{`/${label.toLowerCase()}`}</span>
    </span>
  )
}

// Build editable Content, rendering skill and attachment mentions as highlighted nodes
const makeEditContent = (
  text: string,
  skill?: Mojom.SkillEntry | null,
  associatedContent: Mojom.AssociatedContent[] = [],
): Content =>
  parseRichSegments(text, skill, associatedContent).map((seg) => {
    if (seg.type === 'text') return seg.value
    if (seg.type === 'skill') {
      return { type: 'skill' as const, id: seg.id, text: seg.text }
    }
    return {
      type: 'attachment' as const,
      id: seg.content.contentId.toString(),
      text: seg.content.title,
      url: seg.content.url.url,
    }
  })

/**
 * Returns the conversation history as an array of pairs, each pair containing
 * a human turn group followed by its associated assistant response group, so
 * responses are treated as a single entity when they span multiple tool-use
 * loops. For example:
 *
 *   [[[human], [assistant, ...assistant]], [[human], [assistant, ...assistant]]]
 *
 * Consecutive assistant entries are kept together in the same parent element
 * for semantic and style purposes — the back-and-forth between tool calls is
 * an implementation detail the user shouldn't have to see.
 */
function usePairedConversationGroups() {
  const conversationContext = useUntrustedConversationContext()
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

function ConversationEntries(props: { scrollToBottom: () => void }) {
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

  const handleEditSubmit = (index: number, content: Content) => {
    const entryUuid = conversationContext.conversationHistory[index]?.uuid
    if (!entryUuid) return
    const text = stringifyContent(content)
    const skillNode = content.find(
      (c): c is Extract<Content[number], { type: string }> =>
        typeof c !== 'string' && c.type === 'skill',
    )
    conversationContext.conversationHandler?.modifyConversation(
      entryUuid,
      text,
      skillNode?.id ?? null,
    )
    setEditInputId(undefined)
  }

  function renderEntryGroup(
    isActiveGroup: boolean,
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
      isActiveGroup && isAIAssistant && conversationContext.isGenerating
    const isHuman = firstEntryEdit.characterType === Mojom.CharacterType.HUMAN

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

    const groupIsTask = isAIAssistant && isAssistantGroupTask(group)

    // Omit artifacts until generation is complete so we show
    // the artifacts and the final response text at the same time.
    const shouldOmitToolArtifacts =
      isActiveGroup && conversationContext.isGenerating
    const toolArtifacts = !shouldOmitToolArtifacts
      ? getToolArtifacts(group)
      : null

    // Computed once per group and passed to both AssistantTask and
    // AssistantResponse so all entries in the group share the same set of
    // anchor-permitted URLs. Required because a client-side tool call lives
    // in a separate assistant entry from the follow-up response that
    // references the tool's URLs.
    const allowedLinks = getGroupAllowedLinks(group)

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
                isActiveTask={isActiveGroup}
                allowedLinks={allowedLinks}
              />
            )}
            {!groupIsTask
              && group.map((entry, i) => {
                const isEntryInProgress =
                  isEntryInProgressButGroup && i === group.length - 1
                const isActiveEntryInActiveGroup =
                  isActiveGroup && i === group.length - 1
                const currentEntryEdit = entry.edits?.at(-1) ?? entry
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
                          entryUuid={entry.uuid}
                          events={
                            currentEntryEdit.events?.filter(Boolean) ?? []
                          }
                          isEntryInteractivityAllowed={
                            isActiveEntryInActiveGroup
                          }
                          isEntryInProgress={isEntryInProgress}
                          allowedLinks={allowedLinks}
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
                              {highlightRichText(
                                currentEntryEdit.text,
                                currentEntryEdit.skill,
                                conversationContext.associatedContent ?? [],
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
                        content={makeEditContent(
                          firstEntryEdit.text,
                          firstEntryEdit.skill,
                          tabAttachments,
                        )}
                        onSubmit={(content) =>
                          handleEditSubmit(entryNumber, content)
                        }
                        onCancel={() => setEditInputId(undefined)}
                        isSubmitDisabled={
                          !conversationContext.canSubmitUserEntries
                        }
                      />
                    )}
                    {firstEntryEdit.selectedText && (
                      <div className={styles.humanMessageBubble}>
                        <div className={styles.humanTextRow}>
                          {renderActionLabel(firstEntryEdit.actionType)}{' '}
                          {firstEntryEdit.selectedText}
                        </div>
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

          {isAIAssistant && showEditIndicator && (
            <EditIndicator time={lastEditedTime} />
          )}
          {isAIAssistant
            && (!conversationContext.isGenerating || !isActiveGroup)
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
                  canEditEntry ? () => setEditInputId(entryNumber) : undefined
                }
                onCopyTextClicked={handleCopyText}
              />
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
              ? props.scrollToBottom
              : undefined
          }
        >
          <ProgressBubbleContextProvider>
            {pair.map((group, groupIndex) => (
              <React.Fragment key={groupIndex}>
                {renderEntryGroup(
                  pairIndex === entryPairs.length - 1,
                  group,
                  // Note, we need to keep track of the entry number across pairs and groups.
                  entryNumber++,
                )}
                {groupIndex === 0 && (
                  <ProgressBubble
                    responseGroup={pair[1]}
                    isLastGroup={pairIndex === entryPairs.length - 1}
                  />
                )}
              </React.Fragment>
            ))}
          </ProgressBubbleContextProvider>
        </div>
      ))}
    </div>
  )
}

export default ConversationEntries

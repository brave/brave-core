/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styles from './style.module.scss'
import classnames from '$web-common/classnames'
import Icon from '@brave/leo/react/icon'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Input from '@brave/leo/react/input'
import * as Mojom from '../../../common/mojom'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { isWorkspaceContent } from '../../../common/conversation_history_utils'
import { useAIChat } from '../../state/ai_chat_context'
import { useActiveChat } from '../../state/active_chat_context'
import { getLocale } from '$web-common/locale'
import { useConversation } from '../../state/conversation_context'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { Link } from '$web-common/useRoute'
import ProgressRing from '@brave/leo/react/progressRing'

// The maximum number of workspaces shown in the recent workspaces section.
const MAX_RECENT_WORKSPACES = 5

interface SimpleInputProps {
  text?: string
  onSubmit?: (value: string) => void
  onBlur?: () => void
}

function SimpleInput(props: SimpleInputProps) {
  const [value, setValue] = React.useState(props.text || '')

  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setValue(event.target.value)
  }

  const handleSubmit = (event: React.FormEvent<HTMLFormElement>) => {
    event.preventDefault()
    props.onSubmit?.(value)
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Escape') {
      props.onBlur?.()
    }
  }

  return (
    <form onSubmit={handleSubmit}>
      <input
        className={styles.simpleInput}
        type='text'
        value={value}
        onChange={handleChange}
        autoFocus
        onBlur={props.onBlur}
        onKeyDown={handleKeyDown}
      />
    </form>
  )
}

function getConversationTitle(conversation: Mojom.Conversation) {
  return conversation.title || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)
}

// A conversation can be rendered more than once (e.g. in the recent workspaces
// section and in the main list), so per-item UI state (open options menu,
// inline rename) is tracked by an item key that is unique per rendered item,
// rather than by conversation uuid.
interface ItemState {
  openOptionsMenuKey: string | undefined
  setOpenOptionsMenuKey: React.Dispatch<
    React.SetStateAction<string | undefined>
  >
  // The item the current rename was started from. When unset (e.g. renaming
  // was started from outside the list), the main list item is used.
  editingItemKey: string | undefined
  setEditingItemKey: (key: string | undefined) => void
  activeUuid: string | undefined
  generationStateMap: Map<string, Mojom.ConversationGenerationState>
}

interface ConversationsListProps {
  setIsConversationsListOpen?: (value: boolean) => unknown
}

interface ConversationItemProps extends ConversationsListProps {
  conversation: Mojom.Conversation
  // Unique key for this rendered item. Defaults to the conversation uuid,
  // which is used by the main list.
  itemKey?: string
  itemState: ItemState
}

function ConversationItem(props: ConversationItemProps) {
  const aiChatContext = useAIChat()
  const { itemState } = props

  const { uuid } = props.conversation
  const itemKey = props.itemKey ?? uuid
  const title = getConversationTitle(props.conversation)

  const isOptionsMenuOpen = itemState.openOptionsMenuKey === itemKey

  const handleButtonMenuChange = (e: { isOpen: boolean }) => {
    if (e.isOpen) {
      itemState.setOpenOptionsMenuKey(itemKey)
    } else {
      itemState.setOpenOptionsMenuKey((current) =>
        current === itemKey ? undefined : current,
      )
    }
  }

  const startEditing = () => {
    itemState.setEditingItemKey(itemKey)
    aiChatContext.setEditingConversationId(uuid)
  }

  const stopEditing = () => {
    itemState.setEditingItemKey(undefined)
    aiChatContext.setEditingConversationId(null)
  }

  const handleEditTitle: EventListener = (e) => {
    e.preventDefault()
    startEditing()
  }

  const handleDelete: EventListener = (e) => {
    e.preventDefault()
    aiChatContext.setDeletingConversationId(uuid)
  }

  const isEditing =
    aiChatContext.editingConversationId === uuid
    && (itemState.editingItemKey ?? uuid) === itemKey

  const generationState = itemState.generationStateMap.get(uuid)

  const isGenerating =
    generationState === Mojom.ConversationGenerationState.kGenerating
  const isCompletedNotViewed =
    generationState === Mojom.ConversationGenerationState.kCompletedNotViewed

  const isActive = uuid === itemState.activeUuid

  const showGenerating = isGenerating && !isActive
  const showCompleted = isCompletedNotViewed && !isActive

  return (
    <li>
      <Link
        className={classnames(
          styles.navItem,
          isActive && styles.navItemActive,
          isOptionsMenuOpen && styles.isOptionsMenuOpen,
        )}
        onClick={(e) => {
          if (isEditing) {
            e.preventDefault()
            return
          }
          props.setIsConversationsListOpen?.(false)
        }}
        onDoubleClick={startEditing}
        href={`/${uuid}`}
      >
        <div className={styles.displayTitle}>
          <div className={styles.displayTitleContent}>
            <div
              className={styles.text}
              title={title}
              data-testid='conversation-title'
            >
              {title}
            </div>
          </div>

          {showGenerating && (
            <span
              className={styles.generatingIndicator}
              aria-label={getLocale(S.AI_CHAT_CONVERSATION_LIST_GENERATING)}
              title={getLocale(S.AI_CHAT_CONVERSATION_LIST_GENERATING)}
              role='status'
            >
              <ProgressRing mode='indeterminate' />
            </span>
          )}
          {showCompleted && (
            <span
              className={styles.completedIndicator}
              aria-label={getLocale(
                S.AI_CHAT_CONVERSATION_LIST_COMPLETED_NOT_VIEWED,
              )}
              title={getLocale(
                S.AI_CHAT_CONVERSATION_LIST_COMPLETED_NOT_VIEWED,
              )}
              role='status'
            />
          )}

          {/* Stop propagation so clicks don't bubble to the Link and close the sidebar */}
          <div onClick={(e) => e.stopPropagation()}>
            <ButtonMenu
              className={styles.optionsMenu}
              isOpen={isOptionsMenuOpen}
              onChange={handleButtonMenuChange}
            >
              <Button
                slot='anchor-content'
                className={styles.optionsButton}
                kind='plain-faint'
                fab
                size='small'
              >
                <Icon name='more-vertical' />
              </Button>
              <leo-menu-item onClick={handleEditTitle}>
                <div className={styles.optionsMenuItemWithIcon}>
                  <Icon name='edit-pencil' />
                  <div>{getLocale(S.CHAT_UI_MENU_RENAME_CONVERSATION)}</div>
                </div>
              </leo-menu-item>
              <leo-menu-item onClick={handleDelete}>
                <div className={styles.optionsMenuItemWithIcon}>
                  <Icon name='trash' />
                  <div>{getLocale(S.CHAT_UI_MENU_DELETE_CONVERSATION)}</div>
                </div>
              </leo-menu-item>
            </ButtonMenu>
          </div>
        </div>
        {isEditing && (
          <div className={styles.editibleTitle}>
            <SimpleInput
              text={title}
              onBlur={stopEditing}
              onSubmit={(value) => {
                stopEditing()
                aiChatContext.api.service.renameConversation(uuid, value)
              }}
            />
          </div>
        )}
      </Link>
    </li>
  )
}

interface WorkspaceGroup {
  // The workspace's workspace://<uuid> URL, which identifies it.
  url: Url
  // Conversations attached to the workspace, most recently updated first.
  conversations: Mojom.Conversation[]
}

// Groups conversations by attached workspace. |conversations| must be sorted
// most recently updated first (as provided by the service), so a workspace's
// recency is that of its most recently updated conversation.
function getRecentWorkspaces(
  conversations: Mojom.Conversation[],
): WorkspaceGroup[] {
  const groups = new Map<string, WorkspaceGroup>()
  for (const conversation of conversations) {
    for (const content of conversation.associatedContent) {
      if (!isWorkspaceContent(content)) continue
      let group = groups.get(content.url.url)
      if (!group) {
        group = { url: content.url, conversations: [] }
        groups.set(content.url.url, group)
      }
      if (!group.conversations.includes(conversation)) {
        group.conversations.push(conversation)
      }
    }
  }
  return [...groups.values()].slice(0, MAX_RECENT_WORKSPACES)
}

interface CollapsibleHeaderProps {
  className: string
  isExpanded: boolean
  onToggle: () => void
  controlsId: string
  testId: string
  children: React.ReactNode
}

function CollapsibleHeader(props: CollapsibleHeaderProps) {
  return (
    <button
      type='button'
      className={props.className}
      aria-expanded={props.isExpanded}
      aria-controls={props.controlsId}
      data-testid={props.testId}
      onClick={props.onToggle}
    >
      <Icon
        className={styles.collapseIcon}
        name={props.isExpanded ? 'carat-down' : 'carat-right'}
      />
      {props.children}
    </button>
  )
}

interface WorkspaceItemProps extends ConversationsListProps {
  workspace: WorkspaceGroup
  itemState: ItemState
}

function WorkspaceItem(props: WorkspaceItemProps) {
  const { createNewConversationWithWorkspace } = useActiveChat()
  const [isExpanded, setIsExpanded] = React.useState(false)
  const listId = React.useId()
  const { conversations } = props.workspace

  // Workspaces don't have a user-facing name, so label each one with the title
  // of the conversation it was created in (the oldest attached conversation).
  const title = getConversationTitle(conversations[conversations.length - 1])
  const newConversationLabel = getLocale(
    S.AI_CHAT_CONVERSATION_LIST_NEW_CONVERSATION_IN_WORKSPACE,
  )

  return (
    <li data-testid='workspace-item'>
      <div className={styles.workspaceRow}>
        <CollapsibleHeader
          className={styles.workspaceHeader}
          isExpanded={isExpanded}
          onToggle={() => setIsExpanded((expanded) => !expanded)}
          controlsId={listId}
          testId='workspace-toggle'
        >
          <Icon
            className={styles.workspaceIcon}
            name={isExpanded ? 'folder-open' : 'folder'}
          />
          <span
            className={styles.text}
            title={title}
            data-testid='workspace-title'
          >
            {title}
          </span>
        </CollapsibleHeader>
        <Button
          className={styles.workspaceNewButton}
          fab
          kind='plain-faint'
          size='small'
          aria-label={newConversationLabel}
          title={newConversationLabel}
          data-testid='workspace-new-conversation'
          onClick={() => {
            createNewConversationWithWorkspace(props.workspace.url)
            props.setIsConversationsListOpen?.(false)
          }}
        >
          <Icon name='edit-box' />
        </Button>
      </div>
      {isExpanded && (
        <ol
          id={listId}
          className={styles.workspaceConversations}
        >
          {conversations.map((conversation) => (
            <ConversationItem
              key={conversation.uuid}
              setIsConversationsListOpen={props.setIsConversationsListOpen}
              conversation={conversation}
              itemKey={`workspace:${props.workspace.url.url}:${conversation.uuid}`}
              itemState={props.itemState}
            />
          ))}
        </ol>
      )}
    </li>
  )
}

interface RecentWorkspacesProps extends ConversationsListProps {
  workspaces: WorkspaceGroup[]
  itemState: ItemState
}

function RecentWorkspaces(props: RecentWorkspacesProps) {
  const [isExpanded, setIsExpanded] = React.useState(true)
  const listId = React.useId()

  return (
    <section
      className={styles.recentWorkspaces}
      data-testid='recent-workspaces'
    >
      <CollapsibleHeader
        className={styles.sectionHeader}
        isExpanded={isExpanded}
        onToggle={() => setIsExpanded((expanded) => !expanded)}
        controlsId={listId}
        testId='recent-workspaces-toggle'
      >
        {getLocale(S.AI_CHAT_CONVERSATION_LIST_RECENT_WORKSPACES)}
      </CollapsibleHeader>
      {isExpanded && (
        <ol id={listId}>
          {props.workspaces.map((workspace) => (
            <WorkspaceItem
              key={workspace.url.url}
              setIsConversationsListOpen={props.setIsConversationsListOpen}
              workspace={workspace}
              itemState={props.itemState}
            />
          ))}
        </ol>
      )}
    </section>
  )
}

export default function ConversationsList(props: ConversationsListProps) {
  const aiChatContext = useAIChat()
  const [filterText, setFilterText] = React.useState('')
  const [openOptionsMenuKey, setOpenOptionsMenuKey] = React.useState<string>()
  const [editingItemKey, setEditingItemKey] = React.useState<string>()

  const generationStateMap =
    aiChatContext.api.useConversationGenerationStateData()
  const { selectedConversationId } = useActiveChat()
  const conversationCtx = useConversation()
  const { conversationUuid: contextConversationUuid } =
    conversationCtx.api.useGetStateData()

  const activeUuid = selectedConversationId || contextConversationUuid

  const itemState: ItemState = {
    openOptionsMenuKey,
    setOpenOptionsMenuKey,
    editingItemKey,
    setEditingItemKey,
    activeUuid,
    generationStateMap,
  }

  const startedNonTemporaryConversations = aiChatContext.conversations.filter(
    (c) => !c.temporary && c.hasContent,
  )

  const filteredConversations = React.useMemo(() => {
    if (!filterText) return startedNonTemporaryConversations
    const lower = filterText.toLowerCase()
    return startedNonTemporaryConversations.filter((c) => {
      const title = getConversationTitle(c)
      return title.toLowerCase().includes(lower)
    })
  }, [startedNonTemporaryConversations, filterText])

  const recentWorkspaces = React.useMemo(
    () =>
      aiChatContext.isWorkspaceToolsEnabled
        ? getRecentWorkspaces(startedNonTemporaryConversations)
        : [],
    [aiChatContext.isWorkspaceToolsEnabled, startedNonTemporaryConversations],
  )

  return (
    <>
      <div className={styles.scroller}>
        <nav className={styles.nav}>
          {startedNonTemporaryConversations.length > 0 && (
            <Input
              className={styles.filterInput}
              style={
                filterText
                  ? ''
                  : '--leo-control-color: var(--leo-color-page-background)'
              }
              placeholder={getLocale(
                S.AI_CHAT_CONVERSATION_LIST_FILTER_PLACEHOLDER,
              )}
              value={filterText}
              onInput={(e) => setFilterText(e.value)}
            >
              <Icon
                name='search'
                slot='left-icon'
              />
              <Button
                fab
                kind='plain-faint'
                size='small'
                slot='right-icon'
                style={`visibility: ${filterText ? 'visible' : 'hidden'}`}
                onClick={() => setFilterText('')}
              >
                <Icon name='close' />
              </Button>
            </Input>
          )}
          {/* Hidden while filtering, as the filter applies to the main list */}
          {!filterText && recentWorkspaces.length > 0 && (
            <RecentWorkspaces
              setIsConversationsListOpen={props.setIsConversationsListOpen}
              workspaces={recentWorkspaces}
              itemState={itemState}
            />
          )}
          {!aiChatContext.isStoragePrefEnabled && (
            <Alert type='notice'>
              <Icon
                name='history'
                slot='icon'
              />
              <div slot='title'>
                {getLocale(
                  S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_TITLE_DISABLED_PREF,
                )}
              </div>
              {getLocale(S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_DISABLED_PREF)}
              <div slot='actions'>
                <Button
                  kind='outline'
                  onClick={aiChatContext.enableStoragePref}
                >
                  {getLocale(
                    S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_DISABLED_PREF_BUTTON,
                  )}
                </Button>
              </div>
            </Alert>
          )}
          {aiChatContext.isStoragePrefEnabled
            && startedNonTemporaryConversations.length === 0 && (
              <Alert type='notice'>
                <Icon
                  name='history'
                  slot='icon'
                />
                <div slot='title'>
                  {getLocale(S.CHAT_UI_MENU_CONVERSATION_HISTORY)}
                </div>
                {getLocale(S.CHAT_UI_NOTICE_CONVERSATION_HISTORY_EMPTY)}
              </Alert>
            )}
          {filterText && filteredConversations.length === 0 && (
            <div className={styles.filterNoResults}>
              <span>
                {getLocale(S.AI_CHAT_CONVERSATION_LIST_FILTER_NO_RESULTS)}
              </span>
              <span>
                {getLocale(
                  S.AI_CHAT_CONVERSATION_LIST_FILTER_NO_RESULTS_SUBTITLE,
                )}
              </span>
            </div>
          )}
          {filteredConversations.length > 0 && (
            <ol>
              {filteredConversations.map((conversation) => (
                <ConversationItem
                  key={conversation.uuid}
                  setIsConversationsListOpen={props.setIsConversationsListOpen}
                  conversation={conversation}
                  itemState={itemState}
                />
              ))}
            </ol>
          )}
        </nav>
      </div>
    </>
  )
}

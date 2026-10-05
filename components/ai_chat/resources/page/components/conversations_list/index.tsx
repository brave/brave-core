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
import { useAIChat } from '../../state/ai_chat_context'
import { getLocale } from '$web-common/locale'
import { useConversation } from '../../state/conversation_context'
import useSearchDelay from '../../hooks/useSearchDelay'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { Link } from '$web-common/useRoute'
import { makeEntryFragment } from '../../../common/entry_fragment'

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

interface ConversationItemProps extends ConversationsListProps {
  conversation: Mojom.Conversation
  openOptionsMenuUuid: string | undefined
  setOpenOptionsMenuUuid: React.Dispatch<
    React.SetStateAction<string | undefined>
  >
}

function ConversationItem(props: ConversationItemProps) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()

  const { uuid } = props.conversation
  const title =
    props.conversation.title || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)

  const isOptionsMenuOpen = props.openOptionsMenuUuid === uuid

  const handleButtonMenuChange = (e: { isOpen: boolean }) => {
    if (e.isOpen) {
      props.setOpenOptionsMenuUuid(uuid)
    } else {
      props.setOpenOptionsMenuUuid((current) =>
        current === uuid ? undefined : current,
      )
    }
  }

  const handleEditTitle: EventListener = (e) => {
    e.preventDefault()
    aiChatContext.setEditingConversationId(uuid)
  }

  const handleDelete: EventListener = (e) => {
    e.preventDefault()
    aiChatContext.setDeletingConversationId(uuid)
  }

  const isEditing = aiChatContext.editingConversationId === uuid
  const isActive = uuid === conversationContext.conversationUuid

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
        onDoubleClick={() => aiChatContext.setEditingConversationId(uuid)}
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
        {uuid === aiChatContext.editingConversationId && (
          <div className={styles.editibleTitle}>
            <SimpleInput
              text={title}
              onBlur={() => aiChatContext.setEditingConversationId(null)}
              onSubmit={(value) => {
                aiChatContext.setEditingConversationId(null)
                aiChatContext.api.service.renameConversation(uuid, value)
              }}
            />
          </div>
        )}
      </Link>
    </li>
  )
}

interface ConversationSearchItemProps extends ConversationsListProps {
  conversation: Mojom.Conversation
  snippet: string | undefined
  // The entry the snippet is from.
  entryUuid: string | undefined
}

// A conversation that matches the search query by its content, shown as a card
// with the passage that matched. It opens the conversation at the entry the
// passage is from.
function ConversationSearchItem(props: ConversationSearchItemProps) {
  const conversationContext = useConversation()

  const { uuid } = props.conversation
  const title =
    props.conversation.title || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)

  return (
    <li>
      <a
        className={classnames(
          styles.conversationSearchItem,
          uuid === conversationContext.conversationUuid
            && styles.conversationSearchItemActive,
        )}
        href={`/${uuid}`}
        data-entry-uuid={props.entryUuid}
        data-testid='conversation-search-item'
        onClick={(e) => {
          props.setIsConversationsListOpen?.(false)
          // Leave clicks that open another tab or window alone.
          if (e.altKey || e.ctrlKey || e.metaKey || e.shiftKey) {
            return
          }
          e.preventDefault()
          // The fragment is made for each click, so that clicking the card
          // again scrolls again.
          const fragment = props.entryUuid
            ? makeEntryFragment(props.entryUuid)
            : ''
          window.history.pushState(null, '', `/${uuid}${fragment}`)
        }}
      >
        <div
          className={styles.conversationSearchTitle}
          title={title}
        >
          {title}
        </div>
        {props.snippet && (
          <div
            className={styles.conversationSearchSnippet}
            data-testid='conversation-search-snippet'
          >
            {props.snippet}
          </div>
        )}
      </a>
    </li>
  )
}

interface ConversationsListProps {
  setIsConversationsListOpen?: (value: boolean) => unknown
}

export default function ConversationsList(props: ConversationsListProps) {
  const aiChatContext = useAIChat()
  const [conversationTitleSearch, setConversationTitleSearch] =
    React.useState('')
  const [openOptionsMenuUuid, setOpenOptionsMenuUuid] = React.useState<string>()

  const startedNonTemporaryConversations = aiChatContext.conversations.filter(
    (c) => !c.temporary && c.hasContent,
  )

  const conversationTitleSearchResults = React.useMemo(() => {
    if (!conversationTitleSearch) return startedNonTemporaryConversations
    const lower = conversationTitleSearch.toLowerCase()
    return startedNonTemporaryConversations.filter((c) => {
      const title = c.title || getLocale(S.AI_CHAT_CONVERSATION_LIST_UNTITLED)
      return title.toLowerCase().includes(lower)
    })
  }, [startedNonTemporaryConversations, conversationTitleSearch])

  // Conversations matching by content are searched for once typing pauses, as
  // brave://history searches its history semantically.
  const conversationSearch = useSearchDelay(conversationTitleSearch)
  const { data: conversationSearchMatches } =
    aiChatContext.api.useSearchConversations(conversationSearch)
  const conversationSearchResults = (
    (conversationTitleSearch && conversationSearchMatches)
    || []
  ).flatMap((match) => {
    const conversation = startedNonTemporaryConversations.find(
      (c) => c.uuid === match.conversationUuid,
    )
    return conversation
      ? [
          {
            conversation,
            snippet: match.snippet,
            entryUuid: match.entryUuid,
          },
        ]
      : []
  })

  return (
    <>
      <div className={styles.scroller}>
        <nav className={styles.nav}>
          {startedNonTemporaryConversations.length > 0 && (
            <Input
              className={styles.conversationTitleSearchInput}
              style={
                conversationTitleSearch
                  ? ''
                  : '--leo-control-color: var(--leo-color-page-background)'
              }
              placeholder={getLocale(
                S.AI_CHAT_CONVERSATION_LIST_FILTER_PLACEHOLDER,
              )}
              value={conversationTitleSearch}
              onInput={(e) => setConversationTitleSearch(e.value)}
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
                style={`visibility: ${conversationTitleSearch ? 'visible' : 'hidden'}`}
                onClick={() => setConversationTitleSearch('')}
              >
                <Icon name='close' />
              </Button>
            </Input>
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
          {conversationSearchResults.length > 0 && (
            <section
              className={styles.conversationSearch}
              data-testid='conversation-search-results'
            >
              <div className={styles.conversationSearchHeading}>
                {getLocale(
                  S.AI_CHAT_CONVERSATION_LIST_CONVERSATION_SEARCH_HEADING,
                )}
              </div>
              <ol>
                {conversationSearchResults.map(
                  ({ conversation, snippet, entryUuid }) => (
                    <ConversationSearchItem
                      key={conversation.uuid}
                      {...props}
                      conversation={conversation}
                      snippet={snippet}
                      entryUuid={entryUuid}
                    />
                  ),
                )}
              </ol>
            </section>
          )}
          {conversationTitleSearch
            && conversationTitleSearchResults.length === 0
            && conversationSearchResults.length === 0 && (
              <div className={styles.conversationTitleSearchNoResults}>
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
          {conversationTitleSearchResults.length > 0 && (
            <ol>
              {conversationTitleSearchResults.map((conversation) => (
                <ConversationItem
                  key={conversation.uuid}
                  {...props}
                  conversation={conversation}
                  openOptionsMenuUuid={openOptionsMenuUuid}
                  setOpenOptionsMenuUuid={setOpenOptionsMenuUuid}
                />
              ))}
            </ol>
          )}
        </nav>
      </div>
    </>
  )
}

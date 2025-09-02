// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styles from './style.module.scss'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import Input from '@brave/leo/react/input'
import Flex from '$web-common/Flex'
import { useAIChat } from '../../state/ai_chat_context'
import { ConversationContext } from '../../state/conversation_context'
import { Bookmark, TabData } from 'components/ai_chat/resources/common/mojom'
import { useConversation } from '../../state/conversation_context'
import { getLocale } from '$web-common/locale'

function TabItem({ tab }: { tab: TabData }) {
    const aiChat = useAIChat()
    const { conversationUuid, associatedContentInfo } = useConversation()
    const content = React.useMemo(() => associatedContentInfo.find(c => c.contentId === tab.contentId), [associatedContentInfo, tab])
    return <Checkbox className={styles.attachmentItem} checked={!!content} onChange={(e) => {
        if (e.checked) {
            aiChat.uiHandler?.associateTab(tab, conversationUuid!)
        } else if (content) {
            aiChat.uiHandler?.disassociateContent(content, conversationUuid!)
        }
    }}>
        <span className={styles.title}>{tab.title}</span>
        <img key={tab.contentId} className={styles.icon} src={`chrome://favicon2/?size=20&pageUrl=${encodeURIComponent(tab.url.url)}`} />
    </Checkbox>
}

function BookmarkItem({ bookmark }: { bookmark: Bookmark }) {
    const aiChat = useAIChat()
    const { conversationUuid, associatedContentInfo } = useConversation()

    // Check if this bookmark is already attached by looking for matching URL in associated content
    const attachedContent = React.useMemo(() =>
        associatedContentInfo.find(c => c.url.url === bookmark.url.url),
        [associatedContentInfo, bookmark.url.url]
    )

    return <Checkbox className={styles.attachmentItem} checked={!!attachedContent}
        onChange={(e) => {
            if (e.checked && !attachedContent) {
                aiChat.uiHandler?.attachBookmark(bookmark, conversationUuid!)
            } else if (!e.checked && attachedContent) {
                aiChat.uiHandler?.disassociateContent(attachedContent, conversationUuid!)
            }
        }} isDisabled={!!attachedContent?.conversationTurnUuid}>
        <span className={styles.title}>{bookmark.title}</span>
        <img key={bookmark.id} className={styles.icon} src={`chrome://favicon2/?size=20&pageUrl=${encodeURIComponent(bookmark.url.url)}`} />
    </Checkbox>
}

type StringKeys = Record<NonNullable<ConversationContext['showAttachments']>, keyof typeof S>;
const titleKey: StringKeys = {
    tabs: S.CHAT_UI_ATTACHMENTS_BROWSER_TABS_TITLE,
    bookmarks: S.CHAT_UI_ATTACHMENTS_BOOKMARKS_TITLE
}
const descriptionKey: StringKeys = {
    tabs: S.CHAT_UI_ATTACHMENTS_BROWSER_TABS_DESCRIPTION,
    bookmarks: S.CHAT_UI_ATTACHMENTS_BOOKMARKS_DESCRIPTION
}

const searchPlaceholderKey: StringKeys = {
    tabs: S.CHAT_UI_ATTACHMENTS_BROWSER_TABS_SEARCH_PLACEHOLDER,
    bookmarks: S.CHAT_UI_ATTACHMENTS_BOOKMARKS_SEARCH_PLACEHOLDER
}

export default function Attachments() {
    const conversation = useConversation()
    const { bookmarks } = useAIChat()
    const [search, setSearch] = React.useState('')

    // Get the current attachment type from showAttachments
    const attachmentType = conversation.showAttachments

    const tabs = React.useMemo(() => {
        const searchLower = search.toLowerCase()
        return conversation.unassociatedTabs
            .filter(t => t.title.toLowerCase().includes(searchLower))
    }, [conversation.unassociatedTabs, search])

    const filteredBookmarks = React.useMemo(() => {
        const searchLower = search.toLowerCase()
        return bookmarks
            .filter(b => b.title.toLowerCase().includes(searchLower))
    }, [bookmarks, search])

    return <div className={styles.root}>
        <div className={styles.header}>
            <Flex direction='row' justify='space-between' align='center'>
                <h4>{getLocale(titleKey[attachmentType!])}</h4>
                <Button fab kind='plain-faint' size='small' onClick={() => conversation.setShowAttachments(null)}>
                    <Icon name='close' />
                </Button>
            </Flex>
            <span className={styles.description}>{getLocale(descriptionKey[attachmentType!])} {getLocale(S.CHAT_UI_ATTACHMENTS_WILL_BE_SENT)}</span>
        </div>
        <Input mode='filled' placeholder={getLocale(searchPlaceholderKey[attachmentType!])} value={search} onInput={e => setSearch(e.value)}>
            <Icon name='search' slot='right-icon' />
        </Input>
        <div className={styles.attachmentList}>
            {attachmentType === 'tabs'
                && tabs.map(t => <TabItem key={t.id} tab={t} />)}
            {attachmentType === 'bookmarks'
                && filteredBookmarks.map(b => <BookmarkItem key={b.id} bookmark={b} />)}
        </div>
    </div>
}

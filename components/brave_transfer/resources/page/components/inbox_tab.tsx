/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'

import { InboxItem, ItemKind } from '../api/transfer_api'
import { useTransferApi } from '../api/transfer_api_context'
import { formatBytes, formatTime } from '../lib/format'
import { getString } from '../lib/strings'
import { EmptyState } from './empty_state'
import { itemKindIcon, itemKindLabel } from './icons'

import { style } from './inbox_tab.style'

// Text longer than this, or with more lines, is collapsed with "Show more".
const collapsedTextLength = 160
const collapsedTextLines = 3

function CopyButton(props: { text: string }) {
  const [copied, setCopied] = React.useState(false)

  React.useEffect(() => {
    if (!copied) {
      return
    }
    const id = setTimeout(() => setCopied(false), 2000)
    return () => clearTimeout(id)
  }, [copied])

  return (
    <Button
      kind='plain-faint'
      size='small'
      onClick={async () => {
        await navigator.clipboard.writeText(props.text)
        setCopied(true)
      }}
    >
      <Icon
        slot='icon-before'
        name={copied ? 'check-normal' : 'copy'}
      />
      {getString(copied ? 'BRAVE_TRANSFER_COPIED' : 'BRAVE_TRANSFER_COPY')}
    </Button>
  )
}

function TextContent(props: { item: InboxItem }) {
  const { text } = props.item
  const [expanded, setExpanded] = React.useState(false)
  const isLong =
    text.length > collapsedTextLength
    || text.split('\n').length > collapsedTextLines

  return (
    <>
      <p
        className='text'
        data-collapsed={isLong && !expanded}
      >
        {text}
      </p>
      <div className='item-actions'>
        <CopyButton text={text} />
        {isLong && (
          <Button
            kind='plain-faint'
            size='small'
            onClick={() => setExpanded(!expanded)}
          >
            {getString(
              expanded ? 'BRAVE_TRANSFER_COLLAPSE' : 'BRAVE_TRANSFER_EXPAND',
            )}
          </Button>
        )}
      </div>
    </>
  )
}

function LinkContent(props: { item: InboxItem }) {
  const api = useTransferApi()
  const { item } = props
  return (
    <>
      <a
        className='link'
        href={item.text}
        onClick={(event) => {
          event.preventDefault()
          api.openLink([item.id])
        }}
      >
        {item.text}
      </a>
      <div className='item-actions'>
        <CopyButton text={item.text} />
      </div>
    </>
  )
}

function FileContent(props: { item: InboxItem }) {
  const api = useTransferApi()
  const isIOS = api.useIsIOSData()
  const { item } = props
  const disabled = item.fileMissing

  return (
    <>
      <div className='file'>
        {item.kind === ItemKind.kImage && item.thumbnailUrl ? (
          <img
            className='thumbnail'
            src={item.thumbnailUrl}
            alt=''
          />
        ) : (
          <div className='file-icon'>
            <Icon name={itemKindIcon(item.kind)} />
          </div>
        )}
        <div className='file-info'>
          <span className='file-name'>{item.fileName}</span>
          <span className='file-size'>{formatBytes(item.size)}</span>
        </div>
      </div>
      {item.fileMissing && (
        <span className='item-warning'>
          <Icon name='warning-triangle-outline' />
          {getString('BRAVE_TRANSFER_FILE_MISSING')}
        </span>
      )}
      {item.savedToFallback && !item.fileMissing && (
        <span className='item-note'>
          <Icon name='info-outline' />
          {getString(
            isIOS
              ? 'BRAVE_TRANSFER_SAVED_TO_FALLBACK_IOS'
              : 'BRAVE_TRANSFER_SAVED_TO_FALLBACK',
          )}
        </span>
      )}
      <div className='item-actions'>
        <Button
          kind='plain-faint'
          size='small'
          isDisabled={disabled}
          onClick={() => api.openItem([item.id])}
        >
          <Icon
            slot='icon-before'
            name='launch'
          />
          {getString('BRAVE_TRANSFER_OPEN')}
        </Button>
        <Button
          kind='plain-faint'
          size='small'
          isDisabled={disabled}
          onClick={() => api.showItemInFolder([item.id])}
        >
          <Icon
            slot='icon-before'
            name='folder'
          />
          {getString(
            isIOS
              ? 'BRAVE_TRANSFER_OPEN_IN_FILES'
              : 'BRAVE_TRANSFER_SHOW_IN_FOLDER',
          )}
        </Button>
      </div>
    </>
  )
}

function InboxRow(props: { item: InboxItem }) {
  const { item } = props
  return (
    <div
      className='row'
      role='row'
      data-kind={item.kind}
    >
      <div
        className='cell time'
        role='cell'
      >
        {formatTime(item.receivedTime)}
      </div>
      <div
        className='cell from'
        role='cell'
      >
        {item.deviceName}
      </div>
      <div
        className='cell kind'
        role='cell'
      >
        <Icon name={itemKindIcon(item.kind)} />
        <span>{getString(itemKindLabel(item.kind))}</span>
      </div>
      <div
        className='cell content'
        role='cell'
      >
        {item.kind === ItemKind.kText && <TextContent item={item} />}
        {item.kind === ItemKind.kLink && <LinkContent item={item} />}
        {(item.kind === ItemKind.kImage || item.kind === ItemKind.kFile) && (
          <FileContent item={item} />
        )}
      </div>
    </div>
  )
}

function ClearHistoryDialog(props: { onClose: () => void }) {
  const api = useTransferApi()
  return (
    <Dialog
      isOpen
      onClose={props.onClose}
    >
      <div slot='title'>
        {getString('BRAVE_TRANSFER_CLEAR_HISTORY_CONFIRM_TITLE')}
      </div>
      {getString('BRAVE_TRANSFER_CLEAR_HISTORY_CONFIRM_TEXT')}
      <div slot='actions'>
        <Button
          kind='outline'
          onClick={props.onClose}
        >
          {getString('BRAVE_TRANSFER_CANCEL')}
        </Button>
        <Button
          kind='filled'
          onClick={() => {
            api.clearHistory()
            props.onClose()
          }}
        >
          {getString('BRAVE_TRANSFER_CLEAR_HISTORY')}
        </Button>
      </div>
    </Dialog>
  )
}

export function InboxTab() {
  const api = useTransferApi()
  const count = api.useInboxItemCountData()
  const { data: inbox } = api.useGetInbox(0, count)
  const [isConfirmingClear, setIsConfirmingClear] = React.useState(false)
  const items = inbox?.items ?? []

  return (
    <div data-css-scope={style.scope}>
      <div className='header'>
        <h2>{getString('BRAVE_TRANSFER_INBOX_TITLE')}</h2>
        <Button
          kind='outline'
          size='small'
          isDisabled={items.length === 0}
          onClick={() => setIsConfirmingClear(true)}
        >
          <Icon
            slot='icon-before'
            name='trash'
          />
          {getString('BRAVE_TRANSFER_CLEAR_HISTORY')}
        </Button>
      </div>
      {inbox && items.length === 0 ? (
        <EmptyState
          icon='inbox-empty'
          title={getString('BRAVE_TRANSFER_INBOX_EMPTY_TITLE')}
          text={getString('BRAVE_TRANSFER_INBOX_EMPTY_TEXT')}
        />
      ) : (
        <div
          className='table'
          role='table'
          aria-label={getString('BRAVE_TRANSFER_INBOX_TITLE')}
        >
          <div
            className='row header-row'
            role='row'
          >
            <div role='columnheader'>
              {getString('BRAVE_TRANSFER_INBOX_COLUMN_TIME')}
            </div>
            <div role='columnheader'>
              {getString('BRAVE_TRANSFER_INBOX_COLUMN_FROM')}
            </div>
            <div role='columnheader' />
            <div role='columnheader'>
              {getString('BRAVE_TRANSFER_INBOX_COLUMN_ITEM')}
            </div>
          </div>
          {items.map((item) => (
            <InboxRow
              key={item.id}
              item={item}
            />
          ))}
        </div>
      )}
      {inbox?.hasMore && (
        <div className='load-more'>
          <Button
            kind='plain'
            onClick={() => api.showOlderInboxItems()}
          >
            {getString('BRAVE_TRANSFER_LOAD_MORE')}
          </Button>
        </div>
      )}
      {isConfirmingClear && (
        <ClearHistoryDialog onClose={() => setIsConfirmingClear(false)} />
      )}
    </div>
  )
}

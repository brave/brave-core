/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { TabOpenerContext } from '../../../shared/components/new_tab_link'
import { useLocaleContext } from '../../lib/locale_strings'
import { AdsHistoryItem } from '../../lib/app_state'
import { AppModelContext } from '../../lib/app_model_context'
import { Modal } from '../common/modal'

import { style } from './ads_history_modal.style'

function truncateDate(date: number) {
  let d = new Date(date)
  return new Date(d.getFullYear(), d.getMonth(), d.getDate()).getTime()
}

interface Group {
  date: number
  items: AdsHistoryItem[]
}

function groupItems(items: AdsHistoryItem[]) {
  const groups: Group[] = []
  const now = Date.now()
  const maxAge = 1000 * 60 * 60 * 24 * 30

  let current: number | null = null

  items = [...items].sort((a, b) => b.createdAt - a.createdAt)

  for (const item of items) {
    let date = truncateDate(item.createdAt)
    if (now - date <= maxAge) {
      if (date !== current) {
        current = date
        groups.push({ date, items: [] })
      }
      groups.at(-1)!.items.push(item)
    }
  }

  return groups
}

const dateFormatter = new Intl.DateTimeFormat(undefined, {
  dateStyle: 'short',
})

interface AdsHistoryItemViewProps {
  item: AdsHistoryItem
  onToggleLike: (item: AdsHistoryItem) => void
  onToggleDislike: (item: AdsHistoryItem) => void
  onToggleInappropriate: (item: AdsHistoryItem) => void
}

function AdsHistoryItemView(props: AdsHistoryItemViewProps) {
  const { getString } = useLocaleContext()
  const tabOpener = React.useContext(TabOpenerContext)
  const [showMenu, setShowMenu] = React.useState(false)
  const { item } = props

  React.useEffect(() => {
    if (!showMenu) {
      return
    }
    const listener = () => {
      setShowMenu(false)
    }
    document.body.addEventListener('click', listener)
    return () => document.body.removeEventListener('click', listener)
  }, [showMenu])

  return (
    <div className='item'>
      <button
        className='ad-info'
        onClick={() => tabOpener.openTab(item.url)}
      >
        <div className='name single-line'>{item.name}</div>
        <div className='text single-line'>{item.text}</div>
        <div className='domain'>{item.domain}</div>
      </button>
      <div className='actions'>
        <button
          className={item.likeStatus === 'liked' ? 'on' : ''}
          onClick={() => props.onToggleLike(item)}
        >
          <Icon name='thumb-up' />
        </button>
        <button
          className={item.likeStatus === 'disliked' ? 'on' : ''}
          onClick={() => props.onToggleDislike(item)}
        >
          <Icon name='thumb-down' />
        </button>
        <button
          className='more'
          onClick={(event) => {
            event.stopPropagation()
            setShowMenu(!showMenu)
          }}
        >
          <Icon name='more-vertical' />
        </button>
        {showMenu && (
          <div className='more-menu'>
            <button onClick={() => props.onToggleInappropriate(item)}>
              {getString('adsHistoryMarkInappropriateLabel')}
              <Icon
                name={
                  item.inappropriate ? 'checkbox-checked' : 'checkbox-unchecked'
                }
              />
            </button>
          </div>
        )}
      </div>
    </div>
  )
}

interface Props {
  onClose: () => void
}

export function AdsHistoryModal(props: Props) {
  const { getString } = useLocaleContext()
  const model = React.useContext(AppModelContext)

  const [adsHistory, setAdsHistory] = React.useState<AdsHistoryItem[] | null>(
    null,
  )

  React.useEffect(() => {
    model.getAdsHistory().then(setAdsHistory)
  }, [model])

  const groups = groupItems(adsHistory || [])

  function updateItem(item: AdsHistoryItem, data: Partial<AdsHistoryItem>) {
    Object.assign(item, data)
    setAdsHistory([...(adsHistory || [])])

    // An action that updates a single Ad history item may have an effect on
    // other history items. Update the Ads history to pick up these changes.
    model.getAdsHistory().then(setAdsHistory)
  }

  function toggleLike(item: AdsHistoryItem) {
    const likeStatus = item.likeStatus === 'liked' ? '' : 'liked'
    model.setAdLikeStatus(item.id, likeStatus)
    updateItem(item, { likeStatus })
  }

  function toggleDislike(item: AdsHistoryItem) {
    const likeStatus = item.likeStatus === 'disliked' ? '' : 'disliked'
    model.setAdLikeStatus(item.id, likeStatus)
    updateItem(item, { likeStatus })
  }

  function toggleInappropriate(item: AdsHistoryItem) {
    const value = !item.inappropriate
    model.setAdInappropriate(item.id, value)
    updateItem(item, { inappropriate: value })
  }

  function renderGroup(group: Group) {
    return (
      <section key={group.date}>
        <label>{dateFormatter.format(new Date(group.date))}</label>
        <div className='items'>
          {group.items.map((item) => (
            <AdsHistoryItemView
              key={item.id}
              item={item}
              onToggleLike={toggleLike}
              onToggleDislike={toggleDislike}
              onToggleInappropriate={toggleInappropriate}
            />
          ))}
        </div>
      </section>
    )
  }

  function renderHistory() {
    return (
      <>
        <p>
          {groups.length === 0
            ? getString('adsHistoryEmptyText')
            : getString('adsHistoryText')}
        </p>
        {groups.map(renderGroup)}
      </>
    )
  }

  if (!adsHistory) {
    return null
  }

  return (
    <Modal onEscape={props.onClose}>
      <Modal.Header
        title={getString('adsHistoryTitle')}
        onClose={props.onClose}
      />
      <div data-css-scope={style.scope}>{renderHistory()}</div>
    </Modal>
  )
}

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ButtonMenu from '@brave/leo/react/buttonMenu'

import { TabOpenerContext } from '../../../shared/components/new_tab_link'
import { useLocaleContext } from '../../lib/locale_strings'
import { useCallbackWrapper } from '../../lib/callback_wrapper'
import { AdsHistoryItem } from '../../lib/app_model'
import { AppModelContext } from '../../lib/app_model_context'
import { Modal } from '../modal'

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
  dateStyle: 'short'
})

interface Props {
  onClose: () => void
}

export function AdsHistoryModal(props: Props) {
  const { getString } = useLocaleContext()
  const model = React.useContext(AppModelContext)
  const tabOpener = React.useContext(TabOpenerContext)
  const wrapCallback = useCallbackWrapper()

  const [adsHistory, setAdsHistory] =
    React.useState<AdsHistoryItem[] | null>(null)

  React.useEffect(() => {
    model.getAdsHistory().then(wrapCallback(setAdsHistory))
  }, [model])

  const groups = groupItems(adsHistory || [])

  function updateItem(item: AdsHistoryItem, data: Partial<AdsHistoryItem>) {
    Object.assign(item, data)
    setAdsHistory([...(adsHistory || [])])

    // An action that updates a single Ad history item may have an effect on
    // other history items. Update the Ads history to pick up these changes.
    model.getAdsHistory().then(wrapCallback(setAdsHistory))
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

  function renderItem(item: AdsHistoryItem) {
    return (
      <div key={item.id} className='item'>
        <button className='ad-info' onClick={() => tabOpener.openTab(item.url)}>
          <div className='name single-line'>{item.name}</div>
          <div className='text single-line'>{item.text}</div>
          <div className='domain'>{item.domain}</div>
        </button>
        <div className='actions'>
          <button
            className={item.likeStatus === 'liked' ? 'on' : ''}
            onClick={() => toggleLike(item)}
          >
            <Icon name='thumb-up' />
          </button>
          <button
            className={item.likeStatus === 'disliked' ? 'on' : ''}
            onClick={() => toggleDislike(item)}
          >
            <Icon name='thumb-down' />
          </button>
          <ButtonMenu>
            <button slot='anchor-content'>
              <Icon name='more-vertical' />
            </button>
            <leo-menu-item onClick={() => toggleInappropriate(item)}>
              {getString('adsHistoryMarkInappropriateLabel')}
              <Icon
                name={item.inappropriate ?
                  'checkbox-checked' :
                  'checkbox-unchecked'}
              />
            </leo-menu-item>
          </ButtonMenu>
        </div>
      </div>
    )
  }

  function renderGroup(group: Group) {
    return (
      <section key={group.date}>
        <label>{dateFormatter.format(new Date(group.date))}</label>
        <div className='items'>
          {group.items.map(renderItem)}
        </div>
      </section>
    )
  }

  function renderHistory() {
    return <>
      <p>
        {
          groups.length === 0 ?
            getString('adsHistoryEmptyText') :
            getString('adsHistoryText')
        }
      </p>
      {groups.map(renderGroup)}
    </>
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
      <div {...style}>
        {renderHistory()}
      </div>
    </Modal>
  )
}

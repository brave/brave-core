/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { Popover } from '../common/popover'
import { Link } from '../common/link'
import { useConversationList } from './use_conversation_list'

import { style } from './sidebar.style'

interface Props {
  isOpen: boolean
  onClose: () => void
}

export function Sidebar(props: Props) {
  const [filterText, setFilterText] = React.useState('')

  const conversations = useConversationList().filter((c) => {
    if (!filterText) {
      return true
    }
    const title = c.title.toLocaleLowerCase()
    return title.includes(filterText.toLocaleLowerCase())
  })

  React.useEffect(() => {
    if (!props.isOpen) {
      setFilterText('')
    }
  }, [props.isOpen])

  return (
    <div data-css-scope={style.scope}>
      <Popover
        className='sidebar'
        isOpen={props.isOpen}
        onClose={props.onClose}
      >
        <div className='header'>
          <button onClick={props.onClose}>
            <Icon name='window-tabs-vertical-expanded' />
          </button>
          <span className='logo' />
          <button>
            <Icon name='edit-box' />
          </button>
        </div>
        <div className='filter'>
          <div className='input-container'>
            <Icon name='search' />
            <input
              type='text'
              value={filterText}
              onChange={(e) => { setFilterText(e.target.value) }}
              placeholder='Search'
            />
          </div>
          <button>
            <Icon name='filter' />
          </button>
        </div>
        <div className='history-label'>
          <Icon name='history' />
          <span>History</span>
        </div>
        <div className='conversation-list'>
          {
            conversations.map((c) => (
              <Link url={`chrome://leo-ai/${c.uuid}`} key={c.uuid}>
                {c.title}
              </Link>
            ))
          }
        </div>
      </Popover>
    </div>
  )
}

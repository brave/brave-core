/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { getString } from '../../lib/strings'
import { Popover } from '../common/popover'

import { style } from './remove_toast.style'

interface Props {
  isOpen: boolean
  onUndo: () => void
  onClose: () => void
}

export function RemoveToast(props: Props) {
  const timeout: React.MutableRefObject<number> = React.useRef(0)

  function cancelTimer() {
    if (timeout.current) {
      clearTimeout(timeout.current)
      timeout.current = 0
    }
  }

  React.useEffect(() => {
    if (!props.isOpen) {
      return
    }
    timeout.current = setTimeout(props.onClose, 4000) as any
    return cancelTimer
  }, [props.isOpen, props.onClose])

  return (
    <div data-css-scope={style.scope}>
      <Popover className='toast' isOpen={props.isOpen} onClose={props.onClose}>
        <div
          className='content'
          onFocus={cancelTimer}
          onMouseDown={cancelTimer}
        >
          <Icon name='info-filled' />
          <div className='text'>
            <p>{getString(S.NEW_TAB_TOP_SITE_REMOVED_TEXT)}</p>
          </div>
          <Button onClick={props.onUndo}>
            {getString(S.NEW_TAB_UNDO_BUTTON_LABEL)}
          </Button>
          <button className='close' onClick={props.onClose}>
            <Icon name='close' />
          </button>
        </div>
      </Popover>
    </div>
  )
}

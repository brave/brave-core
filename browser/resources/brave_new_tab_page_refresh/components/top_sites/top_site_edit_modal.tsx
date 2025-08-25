/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Input from '@brave/leo/react/input'

import { getString } from '../../lib/strings'
import { TopSite } from '../../state/top_sites_state'
import { Modal } from '../common/modal'

import { style } from './top_site_edit_modal.style'

function parseURL(url: string) {
  try {
    return new URL(url)
  } catch {
    return null
  }
}

function maybeAddProtocol(url: string) {
  if (!parseURL(url)) {
    const httpsURL = `https://${url}`
    if (parseURL(httpsURL)) {
      return httpsURL
    }
  }
  return url
}

interface Props {
  topSite: TopSite | null
  isOpen: boolean
  onClose: () => void
  onSave: (url: string, title: string) => void
}

export function TopSiteEditModal(props: Props) {
  const [title, setTitle] = React.useState('')
  const [url, setURL] = React.useState('')

  React.useEffect(() => {
    if (props.isOpen) {
      setTitle(props.topSite?.title ?? '')
      setURL(props.topSite?.url ?? '')
    }
  }, [props.isOpen, props.topSite])

  function isValidInput() {
    return title && Boolean(parseURL(maybeAddProtocol(url)))
  }

  function onSubmit() {
    if (isValidInput()) {
      props.onSave(maybeAddProtocol(url), title)
    }
  }

  function maybeSubmitOnKeyDown(event: React.KeyboardEvent) {
    if (event.key === 'Enter') {
      let target = event.target as HTMLElement
      if (target.matches('leo-input')) {
        onSubmit()
      }
    }
  }

  return (
    <Modal isOpen={props.isOpen} showClose onClose={props.onClose}>
      <div data-css-scope={style.scope} onKeyDown={maybeSubmitOnKeyDown}>
        <h4>
          {props.topSite ?
              getString(S.NEW_TAB_EDIT_TOP_SITE_TITLE) :
              getString(S.NEW_TAB_ADD_TOP_SITE_TITLE)
          }
        </h4>
        <Input value={title} onInput={(detail) => setTitle(detail.value)}>
          <span className='label'>
            {getString(S.NEW_TAB_TOP_SITES_TITLE_LABEL)}
          </span>
        </Input>
        <Input value={url} onInput={(detail) => setURL(detail.value)}>
          <span className='label'>
            {getString(S.NEW_TAB_TOP_SITES_URL_LABEL)}
          </span>
        </Input>
        <div className='actions'>
          <Button kind='outline' onClick={props.onClose}>
            {getString(S.NEW_TAB_CANCEL_BUTTON_LABEL)}
          </Button>
          <Button type='submit' isDisabled={!isValidInput()} onClick={onSubmit}>
            {getString(S.NEW_TAB_SAVE_CHANGES_BUTTON_LABEL)}
          </Button>
        </div>
      </div>
    </Modal>
  )
}

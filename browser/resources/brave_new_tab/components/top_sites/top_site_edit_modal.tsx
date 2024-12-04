/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Input from '@brave/leo/react/input'

import { useLocale } from '../locale_context'
import { TopSite } from '../../models/top_sites_model'
import { Modal } from '../modal'

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
  const { getString } = useLocale()
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
      <div {...style} onKeyDown={maybeSubmitOnKeyDown}>
        <h4>
          {props.topSite ?
              getString('editTopSiteTitle') :
              getString('addTopSiteTitle')
          }
        </h4>
        <Input value={title} onInput={(detail) => setTitle(detail.value)}>
          <span className='label'>{getString('topSitesTitleLabel')}</span>
        </Input>
        <Input value={url} onInput={(detail) => setURL(detail.value)}>
          <span className='label'>{getString('topSitesURLLabel')}</span>
        </Input>
        <div className='actions'>
          <Button kind='outline' onClick={props.onClose}>
            {getString('cancelButtonLabel')}
          </Button>
          <Button type='submit' isDisabled={!isValidInput()} onClick={onSubmit}>
            {getString('saveChangesButtonLabel')}
          </Button>
        </div>
      </div>
    </Modal>
  )
}

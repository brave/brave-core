/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Dialog from '@brave/leo/react/dialog'
import { getLocale } from '$web-common/locale'
import styles from './style.module.scss'
import getAPI from '../../api'
import {
  IGNORE_EXTERNAL_LINK_WARNING_KEY //
} from '../../../common/constants'

export default function OpenExternalLinkModal() {
  // API
  const api = getAPI()

  // State
  const [ignoreChecked, setIgnoreChecked] = React.useState(false)
  const [openingExternalLinkURL, setOpeningExternalLinkURL] = React.useState('')

  // Local storage
  const ignoreExternalLinkWarning = JSON.parse(
    localStorage.getItem(IGNORE_EXTERNAL_LINK_WARNING_KEY) ?? 'false'
  )

  // Methods
  const handleOpenExternalLink = (url: string) => {
    if (chrome.tabs !== undefined) {
      chrome.tabs.create({ url }, () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      })
    } else {
      // Tabs.create is desktop specific. Using window.open for android.
      window.open(url, '_blank', 'noopener noreferrer')
    }
  }

  const onOpenClicked = React.useCallback(() => {
    if (ignoreChecked) {
      localStorage.setItem(IGNORE_EXTERNAL_LINK_WARNING_KEY, 'true')
    }
    handleOpenExternalLink(openingExternalLinkURL)
    setOpeningExternalLinkURL('')
  }, [ignoreChecked, openingExternalLinkURL])

  // Listen for setOpeningExternalLinkURL requests from the child frame
  React.useEffect(() => {
    async function handleSetOpeningExternalLinkURL(url: string) {
      // If the user has ignored the warning, open the link immediately.
      if (ignoreExternalLinkWarning) {
        handleOpenExternalLink(url)
        return
      }
      // Otherwise, set the URL to be opened in the modal.
      setOpeningExternalLinkURL(url)
    }

    const listenerId =
      api.conversationEntriesFrameObserver.setOpeningExternalLinkURL.addListener(
        handleSetOpeningExternalLinkURL
      )

    return () => {
      api.conversationEntriesFrameObserver.removeListener(listenerId)
    }
  }, [api, handleOpenExternalLink, ignoreExternalLinkWarning])

  return (
    <Dialog
      isOpen={openingExternalLinkURL !== ''}
      showClose
      onClose={() => setOpeningExternalLinkURL('')}
      className={styles.dialog}
    >
      <div
        slot='title'
        className={styles.dialogTitle}
      >
        {getLocale('openExternalLink')}
      </div>
      <div className={styles.description}>
        {getLocale('openExternalLinkInfo')}
        <Checkbox
          checked={ignoreChecked}
          onChange={({ checked }) => setIgnoreChecked(checked)}
        >
          <span>{getLocale('openExternalLinkCheckboxLabel')}</span>
        </Checkbox>
      </div>
      <div
        slot='actions'
        className={styles.actionsRow}
      >
        <div className={styles.buttonsWrapper}>
          <Button
            kind='plain-faint'
            size='medium'
            onClick={() => setOpeningExternalLinkURL('')}
          >
            {getLocale('cancelButtonLabel')}
          </Button>
          <Button
            kind='filled'
            size='medium'
            onClick={onOpenClicked}
          >
            {getLocale('openLabel')}
          </Button>
        </div>
      </div>
    </Dialog>
  )
}

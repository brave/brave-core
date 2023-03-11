// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../../common/locale'

import {
  CloseButton,
  StyledDialogWrapper,
  StyledDialog,
  DialogTitle,
  StyledInputLabel,
  StyledInput,
  StyledButtonsContainer,
  StyledButton
} from './style'
import CloseIcon from './assets/close-icon'
import { useCallback, useEffect, useState, useRef } from 'react'

interface Props {
  targetTopSiteForEditing?: NewTab.Site
  textDirection: string
  onClose: () => void
  onSave: (title: string, url: string, newUrl: string) => void
}

export default function EditTopSite ({ targetTopSiteForEditing, textDirection, onClose, onSave }: Props) {
  const dialogRef = useRef<HTMLDivElement>(null)
  const [title, setTitle] = useState(targetTopSiteForEditing?.title ?? '')
  const [url, setUrl] = useState(targetTopSiteForEditing?.url ?? '')

  const saveTopSite = useCallback(() => {
    if (!url) return
    onSave(title, targetTopSiteForEditing?.url ?? '', url)
  }, [url, title, targetTopSiteForEditing, onSave])

  useEffect(() => {
    const handleKeyPress = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose()
      if (e.key === 'Enter') saveTopSite()
    }

    const handleClickOutside = (e: Event) => {
      if (dialogRef.current && !dialogRef.current.contains(e.target as Node)) {
        onClose()
      }
    }

    document.addEventListener('keydown', handleKeyPress)
    document.addEventListener('mousedown', handleClickOutside)

    return () => {
      document.removeEventListener('keydown', handleKeyPress)
      document.removeEventListener('mousedown', handleClickOutside)
    }
  }, [saveTopSite, onClose])

  return <StyledDialogWrapper textDirection={textDirection}>
    <StyledDialog
      ref={dialogRef}
      textDirection={textDirection}>
      <DialogTitle>
        {targetTopSiteForEditing
          ? getLocale('editTopSiteDialogTitle')
          : getLocale('addTopSiteDialogTitle')}
      </DialogTitle>
      <CloseButton onClick={onClose}>
        <CloseIcon />
      </CloseButton>
      <StyledInputLabel>
        {getLocale('addTopSiteDialogNameLabel')}
      </StyledInputLabel>
      <StyledInput
        autoFocus={true}
        type='text'
        value={title}
        onChange={e => setTitle(e.target.value)}
        placeholder={getLocale('addTopSiteDialogNameInputPlaceHolder')} />
      <StyledInputLabel>
        {getLocale('addTopSiteDialogURLLabel')}
      </StyledInputLabel>
      <StyledInput
        type='url'
        value={url}
        onChange={e => setUrl(e.target.value)}
        placeholder={getLocale('addTopSiteDialogURLInputPlaceHolder')} />
      <StyledButtonsContainer>
        <StyledButton
          text={getLocale('addTopSiteDialogCancelButtonLabel')}
          level={'secondary'}
          size={'small'}
          onClick={onClose} />
        <StyledButton
          text={getLocale('addTopSiteDialogSaveButtonLabel')}
          level={'primary'}
          type={'accent'}
          size={'small'}
          disabled={!url}
          onClick={saveTopSite} />
      </StyledButtonsContainer>
    </StyledDialog>
  </StyledDialogWrapper>
}

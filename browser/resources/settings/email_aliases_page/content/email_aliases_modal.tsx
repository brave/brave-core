// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Button from "@brave/leo/react/button"
import Icon from "@brave/leo/react/icon"
import { getLocale } from '$web-common/locale'
import formatMessage from '$web-common/formatMessage'
import * as React from 'react'
import { ViewState, ViewMode, MappingService, MAX_ALIASES } from "./types"
import { Col, Row } from "./style"
import Input, { InputEventDetail } from "@brave/leo/react/input"
import styled from "styled-components"

const Modal = styled(Col)`
  border-radius: var(--cr-card-border-radius);
  background-color: white;
  z-index: 2;
  border: none;
  opacity: 100%;
  position: fixed;
  top: 50%;
  left: 50%;
  transform: translate(-50%,-50%);
  justify-content: flex-start;
`

const InnerModal = styled.div`
  width: 42em;
  margin: 1em 2em;
`

const ModalSectionCol = styled(Col)`
  margin: 1em 0em;
  & h3 {
    margin: 0.25em;
  }
  & leo-input {
    margin: 0.25em 0em;
  }
`

const ButtonRow = styled(Row)<{ bubble: boolean }>`
  justify-content: ${props => props.bubble ? 'space-between' : 'end'};
  margin: 1em 0em;
  & leo-button {
    flex-grow: 0;
  }
  & span {
    display: flex;
    flex-direction: row;
    column-gap: 1em;
  }
`

const CloseButton = styled.span`
  cursor: pointer;
  position: absolute;
  top: 1.75em;
  right: 1.75em;
`

const GeneratedEmailContainer = styled(Row)`
  font-size: 135%;
  background-color:var(--leo-color-neutral-variant-10);
  border-radius: 0.5em;
  padding: 0em 0em 0em 0.75em;
  margin: 0.25em 0em;
  justify-content: space-between;
  height: 2.6em;
  & leo-button {
    flex-grow: 0;
  }
`

const ButtonWrapper = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  @keyframes spin {
    0% { transform: rotate(0deg); }
    100% { transform: rotate(360deg); }
  }
  & .waiting {
    animation: spin 1s linear infinite;
  }
  width: 4em;
  height: 100%;
`

const onEnterKey = (onSubmit: () => void) =>
  (e: InputEventDetail) => {
    const innerEvent = e.innerEvent as unknown as KeyboardEvent
    if (innerEvent.key === 'Enter') {
      onSubmit()
    }
  }

const RefreshButton = ({ onClicked }: { onClicked: () => Promise<void> }) => {
  const [waiting, setWaiting] = React.useState(false)
  const onClick = async () => {
    setWaiting(true)
    await onClicked()
    setWaiting(false)
  }
  return <ButtonWrapper title={getLocale('emailAliasesRefreshButtonTitle')}>
    {waiting ? <Icon className='waiting' name="loading-spinner" /> :
      <Button title={getLocale('emailAliasesGeneratingNewAlias')}
        onClick={onClick}
        kind="plain" >
        <Icon name="refresh" />
      </Button>}
  </ButtonWrapper>
}

export const ModalWithCloseButton = ({ children, onReturnToMain }: React.PropsWithChildren & { onReturnToMain: () => void }) =>
  <Modal>
    <CloseButton onClick={onReturnToMain}><Icon name='close' /></CloseButton>
    {children}
  </Modal>

export const EmailAliasModal = (
  { onReturnToMain, viewState, email, mode, mappingService, bubble }:
    {
      onReturnToMain: () => void,
      viewState?: ViewState,
      bubble?: boolean,
      mode: ViewMode,
      email: string,
      mappingService: MappingService
    }
) => {
  const [limitReached, setLimitReached] = React.useState<boolean>(false)
  const [mainEmail, setMainEmail] = React.useState<string>(email)
  const [proposedAlias, setProposedAlias] = React.useState<string>(viewState?.alias?.email ?? '')
  const [proposedNote, setProposedNote] = React.useState<string>(viewState?.alias?.note ?? '')
  const createOrSave = async () => {
    if (proposedAlias) {
      if (mode === 'Create') {
        await mappingService.createAlias(proposedAlias, proposedNote)
        await mappingService.fillField(proposedAlias)
      } else {
        await mappingService.updateAlias(proposedAlias, proposedNote, true)
      }
      onReturnToMain()
    }
  }
  const regenerateAlias = async () => {
    const newEmailAlias = await mappingService.generateAlias()
    setProposedAlias(newEmailAlias)
  }
  React.useEffect(() => {
    if (mode === 'Create') {
      regenerateAlias()
    }
    mappingService.getAccountEmail().then(email => setMainEmail(email ?? ''))
    if (bubble) {
      mappingService.getAliases().then(aliases => {
        setLimitReached(aliases.length >= MAX_ALIASES)
      })
    }
  }, [mode])
  return (
    <InnerModal>
      <h2>{mode === 'Create' ? getLocale('emailAliasesCreateAliasTitle') : getLocale('emailAliasesEditAliasTitle')}</h2>
      {bubble && <div>{getLocale('emailAliasesBubbleDescription')}</div>}
      {(bubble && limitReached) ?
        <h3>{getLocale('emailAliasesBubbleLimitReached')}</h3> :
        <span>
          <ModalSectionCol>
            <h3>{getLocale('emailAliasesAliasLabel')}</h3>
            <GeneratedEmailContainer>
              <div>{proposedAlias}</div>
              {mode === 'Create' && <RefreshButton onClicked={regenerateAlias} />}
            </GeneratedEmailContainer>
            <div>{formatMessage(getLocale('emailAliasesEmailsWillBeForwardedTo'), { placeholders: { $1: mainEmail } })}</div>
          </ModalSectionCol>
          <ModalSectionCol>
            <h3>{getLocale('emailAliasesNoteLabel')}</h3>
            <Input id='note-input'
              type='text'
              placeholder={getLocale('emailAliasesEditNotePlaceholder')}
              maxlength={255}
              value={proposedNote}
              onChange={(detail) => setProposedNote(detail.value)}
              onKeyDown={onEnterKey(createOrSave)}>
            </Input>
            {mode === 'Edit' && viewState?.alias?.domains &&
             <div>
               {formatMessage(getLocale('emailAliasesUsedBy'),
                              { placeholders: { $1: viewState?.alias?.domains?.join(', ') } })}
              </div>}
          </ModalSectionCol>
        </span>
      }
      <ButtonRow bubble={bubble ?? false}>
        <span>
          {bubble && <Button onClick={mappingService.showSettingsPage} kind='plain'>
            {getLocale('emailAliasesManageButton')}
          </Button>}
        </span>
        <span>
          <Button onClick={onReturnToMain} kind='plain'>
            {getLocale('emailAliasesCancelButton')}
          </Button>
          <Button
            kind='filled'
            isDisabled={limitReached}
            onClick={createOrSave}>
            {mode === 'Create' ? getLocale('emailAliasesCreateAliasButton') : getLocale('emailAliasesSaveAliasButton')}
          </Button>
        </span>
      </ButtonRow>
    </InnerModal>
  )
}
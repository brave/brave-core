// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  color,
  font,
  radius,
  spacing,
  typography,
} from '@brave/leo/tokens/css/variables'
import { formatLocale, getLocale } from '$web-common/locale'
import { onEnterKeyForInput } from './on_enter_key'
import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import Col from './styles/Col'
import Icon from '@brave/leo/react/icon'
import Input from '@brave/leo/react/input'
import ProgressRing from '@brave/leo/react/progressRing'
import Row from './styles/Row'
import styled from 'styled-components'
import {
  Alias,
  EmailAliasesServiceInterface,
  MAX_ALIASES,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const ModalCol = styled(Col)`
  row-gap: ${spacing['2Xl']};
`

const ModalTitle = styled.h4`
  color: ${color.text.secondary};
  font: ${font.heading.h4};
  margin: 0;
  line-height: ${typography.heading.h4.lineHeight};
`

const ModalDescription = styled.div`
  font: ${font.default.regular};
  color: ${color.text.primary};
`

const ModalSectionCol = styled(Col)`
  row-gap: ${spacing.s};
`

const ModalLabel = styled.div`
  font: ${font.small.semibold};
  line-height: ${typography.lineHeight.small};
  margin: 0;
  padding: 0 ${spacing.s};
`

const GeneratedEmailContainer = styled(Row)`
  align-items: center;
  background-color: ${color.neutralVariant[10]};
  border-radius: ${radius.m};
  font: ${font.default.regular};
  height: 44px;
  justify-content: space-between;
  padding: 0 ${spacing.m};
  & div {
    padding: 0 ${spacing.s};
  }
  & leo-button {
    flex-grow: 0;
  }
`

const ButtonWrapper = styled.div`
  --leo-button-color: ${color.icon.default};
  display: flex;
  justify-content: center;
  align-items: center;
  width: ${spacing['4Xl']};
  height: 100%;
  @keyframes spin {
    0% {
      transform: rotate(0deg);
    }
    100% {
      transform: rotate(360deg);
    }
  }
  & .waiting {
    animation: spin 1s linear infinite;
  }
`

const ModalDetails = styled.div`
  font: ${font.small.regular};
  color: ${color.text.tertiary};
  padding: 0 4px;
`

const NoteInput = styled(Input)`
  font: ${font.default.regular};
  padding: 0 0;
  margin 0;
`

const WarningText = styled.div`
  font: ${font.default.semibold};
`

const ButtonRow = styled(Row)<{ bubble?: boolean }>`
  justify-content: ${(props) => (props.bubble ? 'space-between' : 'end')};
  & leo-button {
    flex-grow: 0;
  }
  & span {
    display: flex;
    flex-direction: row;
    gap: ${spacing.m};
  }
`

const LoadingIcon = styled(ProgressRing)`
  --leo-progressring-color: ${color.icon.default};
  --leo-progressring-size: 24px;
`

export type EditMode = 'None' | 'Create' | 'Edit' | 'Delete'

const RefreshButton = ({
  onClick,
  waiting,
}: {
  onClick: () => Promise<void>
  waiting: boolean
}) => {
  return (
    <ButtonWrapper>
      {waiting ? (
        <LoadingIcon />
      ) : (
        <Button
          title={getLocale('emailAliasesRefreshButtonTitle')}
          onClick={onClick}
          kind='plain'
        >
          <Icon name='refresh' />
        </Button>
      )}
    </ButtonWrapper>
  )
}

export const DeleteAliasModal = ({
  onReturnToMain,
  alias,
  emailAliasesService,
}: {
  onReturnToMain: () => void
  alias: Alias
  emailAliasesService: EmailAliasesServiceInterface
}) => {
  const [deleting, setDeleting] = React.useState<boolean>(false)
  const [deleteErrorMessage, setDeleteErrorMessage] = React.useState<
    string | null
  >(null)
  const onDeleteAlias = async () => {
    setDeleteErrorMessage(null)
    setDeleting(true)
    try {
      await emailAliasesService.deleteAlias(alias.email)
      onReturnToMain()
    } catch (errorMessage) {
      setDeleteErrorMessage(errorMessage as string)
    }
    setDeleting(false)
  }
  return (
    <ModalCol>
      <ModalTitle>{getLocale('emailAliasesDeleteAliasTitle')}</ModalTitle>
      <ModalDescription>
        {formatLocale('emailAliasesDeleteAliasDescription', {
          $1: <b>{alias.email}</b>,
        })}
      </ModalDescription>
      <Alert type='warning'>{getLocale('emailAliasesDeleteWarning')}</Alert>
      <ButtonRow>
        <span>
          <Button
            onClick={onReturnToMain}
            kind='plain-faint'
          >
            {getLocale('emailAliasesCancelButton')}
          </Button>
          <Button
            onClick={onDeleteAlias}
            kind='filled'
            isDisabled={deleting}
          >
            {getLocale('emailAliasesDeleteAliasButton')}
          </Button>
        </span>
      </ButtonRow>
      {deleteErrorMessage && <Alert>{deleteErrorMessage}</Alert>}
    </ModalCol>
  )
}

export const EmailAliasModal = ({
  onReturnToMain,
  editing,
  editAlias,
  mainEmail,
  aliasCount,
  emailAliasesService,
  bubble,
}: {
  onReturnToMain: () => void
  editing: boolean
  editAlias?: Alias
  bubble?: boolean
  mainEmail: string
  aliasCount: number
  emailAliasesService: EmailAliasesServiceInterface
}) => {
  const [limitReached, setLimitReached] = React.useState<boolean>(false)
  const [proposedNote, setProposedNote] = React.useState<string>(
    editAlias?.note ?? '',
  )
  const [awaitingProposedAlias, setAwaitingProposedAlias] =
    React.useState<boolean>(true)
  const [awaitingUpdate, setAwaitingUpdate] = React.useState<boolean>(false)
  const [generateAliasResult, setGenerateAliasResult] = React.useState<{
    aliasEmail: string
    errorMessage?: string
  }>({
    aliasEmail: editAlias?.email ?? '',
    errorMessage: undefined,
  })
  const [updateErrorMessage, setUpdateErrorMessage] = React.useState<
    string | null
  >(null)
  const createOrSave = async () => {
    setUpdateErrorMessage(null)
    setAwaitingUpdate(true)
    try {
      await emailAliasesService.updateAlias(
        generateAliasResult.aliasEmail,
        proposedNote,
      )
      onReturnToMain()
    } catch (errorMessage) {
      setUpdateErrorMessage(errorMessage as string)
    }
    setAwaitingUpdate(false)
  }
  const regenerateAlias = async () => {
    setAwaitingProposedAlias(true)
    setGenerateAliasResult({ aliasEmail: '', errorMessage: undefined })
    try {
      // We have to do a cast because the mojom generated code produces the
      // wrong type in its JSDoc.
      // TODO(https://github.com/brave/brave-browser/issues/48960): fix the
      // JSDoc generation issue so that this cast is not needed.
      const proposedEmail =
        (await emailAliasesService.generateAlias()) as unknown as string
      setGenerateAliasResult({
        aliasEmail: proposedEmail,
        errorMessage: undefined,
      })
    } catch (errorMessage) {
      setGenerateAliasResult({
        aliasEmail: '',
        errorMessage: errorMessage as string,
      })
    }
    setAwaitingProposedAlias(false)
  }
  React.useEffect(() => {
    if (bubble) {
      setLimitReached(aliasCount >= MAX_ALIASES)
    }
    if (!editing) {
      regenerateAlias()
    }
  }, [editing])
  return (
    <ModalCol>
      <ModalTitle>
        {!editing
          ? getLocale('emailAliasesCreateAliasTitle')
          : getLocale('emailAliasesEditAliasTitle')}
      </ModalTitle>
      {bubble && (
        <ModalDescription>
          {getLocale('emailAliasesBubbleDescription')}
        </ModalDescription>
      )}
      {bubble && limitReached ? (
        <WarningText>{getLocale('emailAliasesBubbleLimitReached')}</WarningText>
      ) : (
        <ModalCol>
          <ModalSectionCol>
            <ModalLabel>{getLocale('emailAliasesAliasLabel')}</ModalLabel>
            <GeneratedEmailContainer>
              <div data-testid='generated-email'>
                {generateAliasResult.aliasEmail}
              </div>
              {!editing && (
                <RefreshButton
                  data-testid='regenerate-button'
                  onClick={regenerateAlias}
                  waiting={awaitingProposedAlias}
                />
              )}
            </GeneratedEmailContainer>
            {generateAliasResult.errorMessage && (
              <Alert>{generateAliasResult.errorMessage}</Alert>
            )}
            <ModalDetails>
              {formatLocale('emailAliasesEmailsWillBeForwardedTo', {
                $1: mainEmail,
              })}
            </ModalDetails>
          </ModalSectionCol>
          <ModalSectionCol>
            <ModalLabel>{getLocale('emailAliasesNoteLabel')}</ModalLabel>
            <NoteInput
              type='text'
              placeholder={getLocale('emailAliasesEditNotePlaceholder')}
              maxlength={255}
              value={proposedNote}
              onInput={(detail) => setProposedNote(detail.value)}
              onKeyDown={onEnterKeyForInput(createOrSave)}
            ></NoteInput>
            {editing && editAlias?.domains && (
              <div>
                {formatLocale('emailAliasesUsedBy', {
                  $1: editAlias?.domains?.join(', '),
                })}
              </div>
            )}
          </ModalSectionCol>
        </ModalCol>
      )}
      <ButtonRow bubble={bubble}>
        <span>
          <Button
            onClick={onReturnToMain}
            kind='plain-faint'
          >
            {getLocale('emailAliasesCancelButton')}
          </Button>
          <Button
            kind='filled'
            isDisabled={
              awaitingUpdate
              || (!editing
                && (limitReached
                  || awaitingProposedAlias
                  || !generateAliasResult?.aliasEmail))
            }
            onClick={createOrSave}
          >
            {!editing
              ? getLocale('emailAliasesCreateAliasButton')
              : getLocale('emailAliasesSaveAliasButton')}
          </Button>
        </span>
      </ButtonRow>
      {updateErrorMessage && <Alert>{updateErrorMessage}</Alert>}
    </ModalCol>
  )
}

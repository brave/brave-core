// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { color, font, radius, spacing, typography } from
  "@brave/leo/tokens/css/variables"
import { formatLocale, getLocale } from '$web-common/locale'
import { onEnterKeyForInput } from "./on_enter_key"
import * as React from 'react'
import Alert from "@brave/leo/react/alert"
import Button from "@brave/leo/react/button"
import Col from "./styles/Col"
import Icon from "@brave/leo/react/icon"
import Input from "@brave/leo/react/input"
import ProgressRing from "@brave/leo/react/progressRing"
import Row from "./styles/Row"
import styled from "styled-components"
import { Alias, EmailAliasesServiceInterface, MAX_ALIASES }
  from "gen/brave/components/email_aliases/email_aliases.mojom.m"

const ModalCol = styled(Col)`
  row-gap: ${spacing["2Xl"]};
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
  width: ${spacing["4Xl"]};
  height: 100%;
  @keyframes spin {
    0% { transform: rotate(0deg); }
    100% { transform: rotate(360deg); }
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
  justify-content: ${props => props.bubble ? 'space-between' : 'end'};
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

type EditMode =
  | 'None'
  | 'Create'
  | 'Edit'

export type EditState = {
  mode: EditMode,
  alias?: Alias
}

const RefreshButton = ({ onClick, waiting }:
  { onClick: () => Promise<void>, waiting: boolean }) => {
  return <ButtonWrapper>
    {waiting
      ? <LoadingIcon/>
      : <Button title={getLocale('emailAliasesRefreshButtonTitle')}
        onClick={onClick}
        kind="plain" >
        <Icon name="refresh" />
      </Button>}
  </ButtonWrapper>
}

export const EmailAliasModal = (
  { onReturnToMain, editState, mainEmail, aliasCount, emailAliasesService,
    bubble }:
    {
      onReturnToMain: () => void,
      editState: EditState,
      bubble?: boolean,
      mainEmail: string,
      aliasCount: number,
      emailAliasesService: EmailAliasesServiceInterface
    }
) => {
  const [limitReached, setLimitReached] = React.useState<boolean>(false)
  const [proposedAlias, setProposedAlias] = React.useState<string>(
    editState?.alias?.email ?? '')
  const [proposedNote, setProposedNote] = React.useState<string>(
    editState?.alias?.note ?? '')
  const [awaitingProposedAlias, setAwaitingProposedAlias] =
    React.useState<boolean>(true)
  const [generationErrorMessage, setGenerationErrorMessage] =
    React.useState<string | null>(null)
  const createOrSave = async () => {
    if (proposedAlias) {
      emailAliasesService.updateAlias(proposedAlias, proposedNote)
      onReturnToMain()
    }
  }
  const regenerateAlias = async () => {
    setAwaitingProposedAlias(true)
    setProposedAlias('')
    setGenerationErrorMessage(null)
    const { result: { errorMessage, aliasEmail } } =
      await emailAliasesService.generateAlias()
    if (errorMessage) {
      setGenerationErrorMessage(errorMessage)
    } else {
      if (aliasEmail) {
        setProposedAlias(aliasEmail)
      }
    }
    setAwaitingProposedAlias(false)
  }
  React.useEffect(() => {
    if (bubble) {
      setLimitReached(aliasCount >= MAX_ALIASES)
    }
    if (editState.mode === 'Create') {
      regenerateAlias()
    }
  }, [editState.mode])
  return (
    <ModalCol>
      <ModalTitle>{editState.mode === 'Create'
        ? getLocale('emailAliasesCreateAliasTitle')
        : getLocale('emailAliasesEditAliasTitle')}</ModalTitle>
      {bubble && <ModalDescription>
                    {getLocale('emailAliasesBubbleDescription')}
                 </ModalDescription>}
      {(bubble && limitReached)
        ? <WarningText>
            {getLocale('emailAliasesBubbleLimitReached')}
          </WarningText>
        : <ModalCol>
            <ModalSectionCol>
              <ModalLabel>{getLocale('emailAliasesAliasLabel')}</ModalLabel>
              <GeneratedEmailContainer>
                <div data-testid='generated-email'>{proposedAlias}</div>
                {editState.mode === 'Create' &&
                 <RefreshButton data-testid='regenerate-button'
                                onClick={regenerateAlias}
                                waiting={awaitingProposedAlias} />}
              </GeneratedEmailContainer>
              {generationErrorMessage &&
                <Alert>
                  {generationErrorMessage}
                </Alert>}
              <ModalDetails>
                {formatLocale('emailAliasesEmailsWillBeForwardedTo',
                  { $1: mainEmail })}
              </ModalDetails>
            </ModalSectionCol>
            <ModalSectionCol>
              <ModalLabel>{getLocale('emailAliasesNoteLabel')}</ModalLabel>
              <NoteInput
                type='text'
                placeholder={getLocale('emailAliasesEditNotePlaceholder')}
                maxlength={255}
                value={proposedNote}
                onChange={(detail) => setProposedNote(detail.value)}
                onKeyDown={onEnterKeyForInput(createOrSave)}>
              </NoteInput>
              {editState.mode === 'Edit' && editState?.alias?.domains &&
                <div>
                {formatLocale('emailAliasesUsedBy',
                              { $1: editState?.alias?.domains?.join(', ') })}
                </div>}
            </ModalSectionCol>
          </ModalCol>
      }
      <ButtonRow bubble={bubble}>
        <span>
          <Button onClick={onReturnToMain} kind='plain-faint'>
            {getLocale('emailAliasesCancelButton')}
          </Button>
          <Button
            kind='filled'
            isDisabled={editState.mode === 'Create'
                         && (limitReached || awaitingProposedAlias ||
                             !proposedAlias)}
            onClick={createOrSave}>
            {editState.mode === 'Create'
              ? getLocale('emailAliasesCreateAliasButton')
              : getLocale('emailAliasesSaveAliasButton')}
          </Button>
        </span>
      </ButtonRow>
    </ModalCol>
  )
}

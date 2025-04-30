// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { color, font, radius, spacing, typography } from
  "@brave/leo/tokens/css/variables"
import { getLocale } from '$web-common/locale'
import { MAX_ALIASES } from "./constant"
import { onEnterKeyForInput } from "./on_enter_key"
import { ViewState, MappingService } from "./types"
import * as React from 'react'
import Button from "@brave/leo/react/button"
import Col from "./styles/Col"
import formatMessage from '$web-common/formatMessage'
import Icon from "@brave/leo/react/icon"
import Input from "@brave/leo/react/input"
import Row from "./styles/Row"
import styled from "styled-components"
import LoadingIcon from "./LoadingIcon"

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
  font: ${font.default.regular};
  background-color: ${color.neutralVariant[10]};
  border-radius: ${radius.m};
  padding: 0 ${spacing.m};
  justify-content: space-between;
  align-items: center;
  transform: scale(1);
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

const RefreshButton = ({ onClick, waiting }:
  { onClick: () => Promise<void>, waiting: boolean }) => {
  return <ButtonWrapper>
    {waiting
      ? <LoadingIcon data-testid='loading-icon'/>
      : <Button title={getLocale('emailAliasesRefreshButtonTitle')}
        onClick={onClick}
        kind="plain" >
        <Icon name="refresh" />
      </Button>}
  </ButtonWrapper>
}

export const EmailAliasModal = (
  { onReturnToMain, viewState, email, mappingService, bubble }:
    {
      onReturnToMain: () => void,
      viewState: ViewState,
      bubble?: boolean,
      email: string,
      mappingService: MappingService
    }
) => {
  const [limitReached, setLimitReached] = React.useState<boolean>(false)
  const [mainEmail, setMainEmail] = React.useState<string>(email)
  const [proposedAlias, setProposedAlias] = React.useState<string>(
    viewState?.alias?.email ?? '')
  const [proposedNote, setProposedNote] = React.useState<string>(
    viewState?.alias?.note ?? '')
  const [awaitingProposedAlias, setAwaitingProposedAlias] =
    React.useState<boolean>(true)
  const createOrSave = async () => {
    if (proposedAlias) {
      if (viewState.mode === 'Create') {
        await mappingService.createAlias(proposedAlias, proposedNote)
        await mappingService.fillField(proposedAlias)
      } else {
        await mappingService.updateAlias(proposedAlias, proposedNote, true)
      }
      onReturnToMain()
    }
  }
  const regenerateAlias = async () => {
    setAwaitingProposedAlias(true)
    const newEmailAlias = await mappingService.generateAlias()
    if (viewState.mode === 'Create' || viewState.mode === 'Edit') {
      setProposedAlias(newEmailAlias)
      setAwaitingProposedAlias(false)
    }
  }
  React.useEffect(() => {
    mappingService.getAccountEmail().then(email => setMainEmail(email ?? ''))
    if (bubble) {
      mappingService.getAliases().then(aliases => {
        setLimitReached(aliases.length >= MAX_ALIASES)
      })
      return
    }
    if (viewState.mode === 'Create') {
      regenerateAlias()
    }
  }, [viewState.mode])
  return (
    <ModalCol>
      <ModalTitle>{viewState.mode === 'Create'
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
                <div>{proposedAlias}</div>
                {viewState.mode === 'Create' &&
                 <RefreshButton data-testid='regenerate-button'
                                onClick={regenerateAlias}
                                waiting={awaitingProposedAlias} />}
              </GeneratedEmailContainer>
              <ModalDetails>
                {formatMessage(getLocale('emailAliasesEmailsWillBeForwardedTo'),
                  { placeholders: { $1: mainEmail } })}
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
              {viewState.mode === 'Edit' && viewState?.alias?.domains &&
                <div>
                {formatMessage(getLocale('emailAliasesUsedBy'),
                  { placeholders:
                    { $1: viewState?.alias?.domains?.join(', ') } })}
                </div>}
            </ModalSectionCol>
          </ModalCol>
      }
      <ButtonRow bubble={bubble}>
        <span>
          {bubble && <Button kind='plain-faint'
                        onClick={mappingService.showSettingsPage}>
            {getLocale('emailAliasesManageButton')}
          </Button>}
        </span>
        <span>
          <Button onClick={onReturnToMain} kind='plain-faint'>
            {getLocale('emailAliasesCancelButton')}
          </Button>
          <Button
            kind='filled'
            isDisabled={viewState.mode === 'Create'
                         && (limitReached || awaitingProposedAlias)}
            onClick={createOrSave}>
            {viewState.mode === 'Create'
              ? getLocale('emailAliasesCreateAliasButton')
              : getLocale('emailAliasesSaveAliasButton')}
          </Button>
        </span>
      </ButtonRow>
    </ModalCol>
  )
}

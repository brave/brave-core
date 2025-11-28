// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AliasItem } from './email_aliases_item'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import {
  DeleteAliasModal,
  EditMode,
  EmailAliasModal,
} from './email_aliases_modal'
import { getLocale } from '$web-common/locale'
import * as React from 'react'
import Button from '@brave/leo/react/button'
import Col from './styles/Col'
import Description from './styles/Description'
import Dialog from '@brave/leo/react/dialog'
import Row from './styles/Row'
import styled from 'styled-components'
import {
  Alias,
  EmailAliasesServiceInterface,
  MAX_ALIASES,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const AliasListIntro = styled(Row)`
  justify-content: space-between;
  padding: ${spacing.l} ${spacing['2Xl']};
  & leo-button {
    flex-grow: 0;
    font: ${font.components.buttonSmall};
  }
`

const DivWithTopDivider = styled.div`
  border-top: ${color.divider.subtle} 2px solid;
`

const AliasDialog = styled(Dialog)`
  --leo-dialog-backdrop-background: ${color.dialogs.scrimBackground};
  --leo-dialog-padding: ${spacing['2Xl']};
`

export type EditState = {
  mode: EditMode
  alias?: Alias
}

export const ListIntroduction = ({
  aliasesCount,
  onCreateClicked,
}: {
  aliasesCount: number
  onCreateClicked: () => void
}) => (
  <AliasListIntro>
    <Col>
      <h4>{getLocale(S.SETTINGS_EMAIL_ALIASES_LIST_TITLE)}</h4>
      <Description>
        {getLocale(S.SETTINGS_EMAIL_ALIASES_CREATE_DESCRIPTION)}
      </Description>
    </Col>
    <Button
      id='create-new-item-button'
      isDisabled={aliasesCount >= MAX_ALIASES}
      kind='filled'
      size='small'
      name='create-alias'
      title={getLocale(S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_TITLE)}
      onClick={onCreateClicked}
    >
      {getLocale(S.SETTINGS_EMAIL_ALIASES_CREATE_ALIAS_LABEL)}
    </Button>
  </AliasListIntro>
)

export const AliasList = ({
  aliases,
  authEmail,
  emailAliasesService,
}: {
  emailAliasesService: EmailAliasesServiceInterface
  aliases: Alias[]
  authEmail: string
}) => {
  const [editState, setEditState] = React.useState<EditState>({ mode: 'None' })
  return (
    <DivWithTopDivider>
      <ListIntroduction
        aliasesCount={aliases.length}
        onCreateClicked={() => setEditState({ mode: 'Create' })}
      />
      {aliases.map((alias) => (
        <AliasItem
          key={alias.email}
          alias={alias}
          onEdit={() => setEditState({ mode: 'Edit', alias: alias })}
          onDelete={() => setEditState({ mode: 'Delete', alias: alias })}
        ></AliasItem>
      ))}
      <AliasDialog
        isOpen={editState.mode !== 'None'}
        onClose={() => setEditState({ mode: 'None' })}
        backdropClickCloses
        modal
        showClose
      >
        {editState.mode === 'Delete' && editState.alias && (
          <DeleteAliasModal
            onReturnToMain={() => setEditState({ mode: 'None' })}
            alias={editState.alias}
            emailAliasesService={emailAliasesService}
          />
        )}
        {(editState.mode === 'Create' || editState.mode === 'Edit') && (
          <EmailAliasModal
            onReturnToMain={() => setEditState({ mode: 'None' })}
            editing={editState.mode === 'Edit'}
            editAlias={editState.alias}
            mainEmail={authEmail}
            aliasCount={aliases.length}
            emailAliasesService={emailAliasesService}
          />
        )}
      </AliasDialog>
    </DivWithTopDivider>
  )
}

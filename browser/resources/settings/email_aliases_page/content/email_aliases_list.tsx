// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EditState, EmailAliasModal } from './email_aliases_modal'
import { color, spacing } from '@brave/leo/tokens/css/variables'
import { font } from '@brave/leo/tokens/css/variables'
import { getLocale } from '$web-common/locale'
import { onEnterKeyForDiv } from './on_enter_key'
import * as React from 'react'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'
import Col from './styles/Col'
import Description from './styles/Description'
import Dialog from '@brave/leo/react/dialog'
import { formatLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Row from './styles/Row'
import styled from 'styled-components'
import Tooltip from '@brave/leo/react/tooltip'
import { Alias, EmailAliasesServiceInterface, MAX_ALIASES }
  from "gen/brave/components/email_aliases/email_aliases.mojom.m"

const AliasItemRow = styled(Row)`
  font: ${font.default.regular};
  padding: ${spacing.l} ${spacing['2Xl']};
  border-top: ${color.divider.subtle} 1px solid;
  justify-content: space-between;
`

const AliasAnnotation = styled.div`
  font: ${font.small.regular};
  color: ${color.text.secondary};
`

const AliasListIntro = styled(Row)`
  justify-content: space-between;
  padding: ${spacing.l} ${spacing['2Xl']};
  & leo-button {
    flex-grow: 0;
    font: ${font.components.buttonSmall};
  }
`

const EmailContainer = styled.div`
  cursor: pointer;
  font: ${font.default.regular};
`

const AliasControls = styled(Row)`
  user-select: none;
  column-gap: ${spacing.m};
  * {
    --leo-icon-color: ${color.icon.default};
  }
`

const DivWithTopDivider = styled.div`
  border-top: ${color.divider.subtle} 2px solid;
`

const AliasMenuItemContent = styled(Row)`
  gap: ${spacing.m};
  padding: 0;
`

const AliasDialog = styled(Dialog)`
  --leo-dialog-backdrop-background: ${color.dialogs.scrimBackground};
  --leo-dialog-padding: ${spacing['2Xl']};
`

const AliasMenuItem = ({ onClick, iconName, text }:
  { onClick: EventListener, iconName: string, text: string }) =>
  <leo-menu-item
    onClick={onClick}>
    <AliasMenuItemContent>
      <Icon name={iconName} />
      <span>{text}</span>
    </AliasMenuItemContent>
  </leo-menu-item>

const CopyToast = ({ text, tabIndex, children }:
  { text: string, tabIndex?: number, children: React.ReactNode }) => {
  const [copied, setCopied] = React.useState<boolean>(false)
  const copy = () => {
    navigator.clipboard.writeText(text)
    setCopied(true)
    setTimeout(() => setCopied(false), 1000)
  }
  return <div
    tabIndex={tabIndex}
    data-testid="copy-toast"
    onKeyDown={onEnterKeyForDiv(copy)}
    onClick={copy}>
    <Tooltip text={copied ? getLocale('emailAliasesCopiedToClipboard') : ''}
             mode="mini" visible={copied}>
      {children}
    </Tooltip>
  </div>
}

const AliasItem = ({ alias, onEdit, onDelete }:
  { alias: Alias, onEdit: () => void, onDelete: () => void }) =>
  <AliasItemRow>
    <Col>
      <CopyToast text={alias.email}>
        <EmailContainer title={getLocale('emailAliasesClickToCopyAlias')}>
            {alias.email}
          </EmailContainer>
        </CopyToast>
        {(alias.note || alias.domains) &&
          <AliasAnnotation>
            {alias.note}
            {alias.domains && alias.note && <span>. </span>}
            {alias.domains && formatLocale('emailAliasesUsedBy',
              { $1: alias.domains?.join(", ") })}
          </AliasAnnotation>}
        </Col>
      <AliasControls>
        <CopyToast text={alias.email} tabIndex={0}>
          <Button
            fab
            title={getLocale('emailAliasesClickToCopyAlias')}
            kind='plain-faint'
            size='medium'>
            <Icon name="copy"/>
          </Button>
        </CopyToast>
        <ButtonMenu anchor-content="more-vertical">
          <Button
            fab
            slot='anchor-content'
            name='more'
            kind='plain-faint'
            size="large">
            <Icon name="more-vertical" />
          </Button>
          <AliasMenuItem
            iconName="edit-pencil"
            text={getLocale('emailAliasesEdit')}
            onClick={onEdit} />
          <AliasMenuItem
            iconName="trash"
            text={getLocale('emailAliasesDelete')}
            onClick={onDelete} />
        </ButtonMenu>
      </AliasControls>
    </AliasItemRow>

export const AliasList = ({
  aliases, authEmail, emailAliasesService }: {
    emailAliasesService: EmailAliasesServiceInterface,
    aliases: Alias[],
    authEmail: string
  }) => {
  const [editState, setEditState] = React.useState<EditState>({ mode: 'None' })
  const onEditStateChange = (editState: EditState) => setEditState(editState)
  return (
    <DivWithTopDivider>
      <AliasListIntro>
        <Col>
          <h4>{getLocale('emailAliasesListTitle')}</h4>
          <Description>
            {getLocale('emailAliasesCreateDescription')}
          </Description>
        </Col>
        <Button
          isDisabled={aliases.length >= MAX_ALIASES}
          kind='filled'
          size='small'
          name='create-alias'
          title={getLocale('emailAliasesCreateAliasTitle')}
          onClick={
            () => {
              onEditStateChange({ mode: 'Create' })
            }
          }>
          {getLocale('emailAliasesCreateAliasLabel')}
        </Button>
      </AliasListIntro>
      {aliases.map(
        alias =>
          <AliasItem
            key={alias.email}
            alias={alias}
            onEdit={() => setEditState({ mode: 'Edit', alias: alias })}
            onDelete={() => emailAliasesService.deleteAlias(alias.email)}>
          </AliasItem>)}
      {(editState.mode === 'Create' || editState.mode === 'Edit') &&
        <AliasDialog
          isOpen
          onClose={() => setEditState({ mode: 'None' })}
          backdropClickCloses
          modal
          showClose>
          <EmailAliasModal
            onReturnToMain={() => setEditState({ mode: 'None' })}
            editState={editState}
            mainEmail={authEmail}
            aliasCount={aliases.length}
            emailAliasesService={emailAliasesService} />
        </AliasDialog>}
    </DivWithTopDivider>
  )
}

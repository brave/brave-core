// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Alias, MappingService, MAX_ALIASES, ViewState } from './types'
import { Card, Col, Row } from './style'
import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import { color } from '@brave/leo/tokens/css/variables'
import Tooltip from '@brave/leo/react/tooltip'
import { getLocale } from '$web-common/locale'
import formatMessage from '$web-common/formatMessage'
import Button from '@brave/leo/react/button'
import ButtonMenu from '@brave/leo/react/buttonMenu'

const AliasLeoMenuItem = styled('leo-menu-item')`
  & ${Row} { font-size: 90%; }
  & ${Row} span { margin-inline-start: 0.5em; }
`

const AliasItemRow = styled(Row)`
  margin: 0px;
  font-size: 125%;
  padding: 18px 0px 18px 25px;
  border-top: ${color.legacy.divider1} 1px solid;
  justify-content: space-between;
`

const AliasAnnotation = styled.div`
  font-size: 80%;
  font-weight: 400;
  padding-top: 0.25em;
  color: var(--leo-color-neutral-variant-50);
`

const AliasControls = styled(Row)`
  height: 1.5em;
  user-select: none;
  align-items: top;
`

const AliasListIntro = styled(Row)`
  margin-bottom: 20px;
  justify-content: space-between;
  & leo-button {
    flex-grow: 0;
  }
`

const EmailContainer = styled.div`
  cursor: pointer;
`

const CopyButtonWrapper = styled.div`
  cursor: pointer;
  color: var(--leo-color-neutral-variant-60);
  &:hover {
    background-color: var(--leo-color-desktopbrowser-toolbar-button-hover);
  }
  &:active {
    background-color: var(--leo-color-desktopbrowser-toolbar-button-active);
  }
  padding: 0.25em;
  border-radius: 0.5em;
`

const MenuButton = styled(Button)`
  --leo-button-padding: 0;
  flex-grow: 0;
  width: 1.5em;
`

const BorderedCard = styled(Card)`
  border-top: ${color.material.divider} 1px solid;
`

const AliasMenuItem = ({ onClick, iconName, text }:
  { onClick: EventListener, iconName: string, text: string }) =>
  <AliasLeoMenuItem
    onClick={onClick}>
    <Row>
      <Icon name={iconName} />
      <span>{text}</span>
    </Row>
  </AliasLeoMenuItem>

const CopyToast = ({ children }: React.PropsWithChildren) => {
  const [copied, setCopied] = React.useState<boolean>(false)
  return <div onClick={() => {
    setCopied(true)
    setTimeout(() => setCopied(false), 1000)
  }}>
    <Tooltip text={copied ? getLocale('emailAliasesCopiedToClipboard') : ''} mode="mini" visible={copied}>
      {children}
    </Tooltip>
  </div>
}

const AliasItem = ({ alias, onEdit, onDelete }: { alias: Alias, onEdit: () => void, onDelete: () => void }) =>
  <AliasItemRow>
    <Col>
      <CopyToast>
        <EmailContainer title={getLocale('emailAliasesClickToCopyAlias')}
            onClick={(event) => navigator.clipboard.writeText(alias.email)}>
            {alias.email}
          </EmailContainer>
        </CopyToast>
        {(alias.note || alias.domains) &&
          <AliasAnnotation>
            {alias.note}
            {alias.domains && alias.note && <span>. </span>}
            {alias.domains && formatMessage(getLocale('emailAliasesUsedBy'),
              { placeholders: { $1: alias.domains?.join(", ") } })}
          </AliasAnnotation>}
        </Col>
      <AliasControls>
        <CopyToast>
          <CopyButtonWrapper
            title={getLocale('emailAliasesClickToCopyAlias')}
            onClick={() => {
              navigator.clipboard.writeText(alias.email)
            }}>
            <Icon name="copy"/>
          </CopyButtonWrapper>
        </CopyToast>
        <ButtonMenu>
          <MenuButton slot='anchor-content' kind='plain-faint' size="large">
            <Icon name="more-vertical" />
          </MenuButton>
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

export const AliasList = ({ aliases, onViewChange, onListChange, mappingService }: {
    mappingService: MappingService,
    aliases: Alias[],
    onViewChange: (viewState: ViewState) => void,
    onListChange: () => void
  }) =>
  <BorderedCard>
    <AliasListIntro>
      <Col>
        <h2>{getLocale('emailAliasesListTitle')}</h2>
        <div>
          {getLocale('emailAliasesCreateDescription')}
        </div>
      </Col>
      <Button
        isDisabled={aliases.length >= MAX_ALIASES}
        title={getLocale('emailAliasesCreateAliasTitle')}
        id='add-alias'
        onClick={
          async () => {
            onViewChange({ mode: 'Create' })
            const newEmailAlias = await mappingService.generateAlias()
            onViewChange({ mode: 'Create', alias: { email: newEmailAlias } })
          }
        }>
        {getLocale('emailAliasesCreateAliasLabel')}
      </Button>
    </AliasListIntro>
    {aliases.map(
      alias => <AliasItem
        key={alias.email}
        alias={alias}
        onEdit={() => onViewChange({ mode: 'Edit', alias: alias })}
        onDelete={async () => {
          await mappingService.deleteAlias(alias.email)
          onListChange()
        }}></AliasItem>)}
  </BorderedCard>
// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client';
import { color, spacing, font } from '@brave/leo/tokens/css/variables'
import { Alias, MappingService, ViewState } from './content/types'
import { Col, Row, Card, BraveIconCircle } from './content/style'
import Icon from '@brave/leo/react/icon'
import { getLocale } from '$web-common/locale'
import SecureLink from '$web-common/SecureLink'
import { EmailAliasModal, ModalWithCloseButton } from './content/email_aliases_modal';
import { MainEmailEntryForm } from './content/email_aliases_signin_page'
import { AliasList } from './content/email_aliases_list'
import styled, { StyleSheetManager } from 'styled-components'

const MainEmailTextContainer = styled(Col)`
  justify-content: center;
  cursor: default;
  user-select: none;
`

const MainEmailDescription = styled.div`
  font: ${font.default.regular};
`

const MainEmail = styled.div`
  font: ${font.large.semibold};
  padding-bottom: ${spacing.s};
`

const AccountRow = styled(Row)`
  justify-content: space-between;
  align-items: center;
  padding: ${spacing.l} 0px ${spacing.l};
`

const ManageAccountButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  font: ${font.default.regular};
  align-items: center;
  color: ${color.text.secondary};
  text-decoration: none;
  background: none;
  border: none;
  border-radius: 0.5em;
  padding: 0.5em;
  & > span {
    margin: 0.25em;
  }
  cursor: pointer;
  &:hover {
    background-color: var(--leo-color-desktopbrowser-toolbar-button-hover);
  }
  &:active {
    background-color: var(--leo-color-desktopbrowser-toolbar-button-active);
  }
`

const PageCol = styled(Col)`
  padding: ${spacing.l};
`

const Introduction = () =>
  <Card>
    <h2>{getLocale('emailAliasesShortDescription')}</h2>
    <div>{getLocale('emailAliasesDescription')} &emsp;
      {/* TODO: Link to the email aliases support page */}
      <SecureLink href="https://support.brave.com" target='_blank'>
        {getLocale('emailAliasesLearnMore')}
      </SecureLink>
    </div>
  </Card>

const MainEmailDisplay = ({ email, onLogout }: { email: string, onLogout: () => void }) =>
  <Card>
    <AccountRow>
      <Row>
        <BraveIconCircle name='brave-icon-release-color' />
        <MainEmailTextContainer>
          <MainEmail>{email === '' ? getLocale('emailAliasesConnectingToBraveAccount') : email}</MainEmail>
          <MainEmailDescription>{getLocale('emailAliasesBraveAccount')}</MainEmailDescription>
        </MainEmailTextContainer>
      </Row>
      <ManageAccountButton
        title={getLocale('emailAliasesSignOutTitle')}
        onClick={(e) => { e.preventDefault(); onLogout() }}>
        <Icon name="outside" />
        <span>{getLocale('emailAliasesSignOut')}</span>
      </ManageAccountButton>
    </AccountRow>
  </Card>


const MainView = ({
  viewState, mainEmail, onLogout, aliasesState, setViewState,
  mappingService, onListChange
}: {
  viewState: ViewState,
  mainEmail: string,
  onLogout: () => void,
  aliasesState: Alias[],
  setViewState: (viewState: ViewState) => void,
  mappingService: MappingService,
  onListChange: () => void
}) =>
  (viewState.mode === 'Startup' ?
    <Row style={{ margin: '1em', flexGrow: 1, justifyContent: 'center', alignItems: 'center' }}><Icon name='loading-spinner' />
      <h3>{getLocale('emailAliasesConnectingToBraveAccount')}</h3>
    </Row> :
    <span>
      <MainEmailDisplay onLogout={onLogout} email={mainEmail} />
      <AliasList aliases={aliasesState} onViewChange={setViewState}
        mappingService={mappingService}
        onListChange={onListChange}/>
    </span>)

export const GrayOverlay = styled.div`
  background-color: var(--leo-color-dialogs-scrim-background);
  position: fixed;
  z-index: 1;
  inset: 0;
`

export const ManagePage = ({ mappingService }:
  {
    mappingService: MappingService
  }) => {
  const [viewState, setViewState] = React.useState<ViewState>({ mode: 'Startup' })
  const [mainEmail, setMainEmail] = React.useState<string>('')
  const [aliasesState, setAliasesState] = React.useState<Alias[]>([]);
  const onEmailChange = async () => {
    const email = await mappingService.getAccountEmail()
    setMainEmail(email ?? '')
    setViewState({ mode: email ? 'Main' : 'SignUp' })
  }
  const onListChange = async () => {
    const aliases = await mappingService.getAliases()
    setAliasesState(aliases)
  }
  const onMainEmailSubmitted = async (email: string) => {
    setMainEmail(email)
    await mappingService.requestAccount(email)
    setViewState({ mode: 'AwaitingAuthorization' })
    const accountReady = await mappingService.onAccountReady()
    if (accountReady) {
      setViewState({ mode: 'Main' })
      await onListChange()
    }
  }
  const onLogout = () => {
    mappingService.logout()
    setViewState({ mode: 'SignUp' })
  }
  const onRestart = async () => {
    await mappingService.cancelAccountRequest()
    setViewState({ mode: 'SignUp' })
  }
  const onReturnToMain = () => {
    setViewState({ mode: 'Main' })
    onListChange()
  }
  React.useEffect(() => {
    onEmailChange();
    onListChange();
    document.addEventListener('visibilitychange', () => {
      if (document.visibilityState === 'visible') {
        onEmailChange();
        onListChange();
      }
    })
    return () => {
      document.removeEventListener('visibilitychange', () => {})
    }
  }, [] /* Only run at mount. */)
  return (
    <PageCol>
      <Introduction />
      {viewState.mode === 'SignUp' || viewState.mode === 'AwaitingAuthorization' ?
        <MainEmailEntryForm viewState={viewState} mainEmail={mainEmail} onEmailSubmitted={onMainEmailSubmitted} onRestart={onRestart} /> :
        <MainView viewState={viewState} mainEmail={mainEmail} onLogout={onLogout} aliasesState={aliasesState} setViewState={setViewState} mappingService={mappingService} onListChange={onListChange} />}
      {(viewState.mode === 'Create' || viewState.mode === 'Edit') &&
        <span>
          <GrayOverlay onClick={onReturnToMain}>&nbsp;</GrayOverlay>
          <ModalWithCloseButton onReturnToMain={onReturnToMain}>
            <EmailAliasModal
              onReturnToMain={onReturnToMain}
              viewState={viewState}
              email={mainEmail}
              mode={viewState.mode}
              mappingService={mappingService} />
          </ModalWithCloseButton>
        </span>}
    </PageCol>
  )
}

export const mount = (at: HTMLElement, mappingService: MappingService) => {
  const root = createRoot(at);
  root.render(
    <StyleSheetManager target={at}>
      <ManagePage mappingService={mappingService} />
    </StyleSheetManager>
  )
}


export const mountModal = (at: HTMLElement, mappingService: MappingService) => {
  const root = createRoot(at);
  root.render(
    <StyleSheetManager target={at}>
      <EmailAliasModal
        onReturnToMain={mappingService.closeBubble}
        bubble={true}
        mode={'Create'}
        email={'test@test.com'}
        mappingService={mappingService} />
    </StyleSheetManager>
  )
}

; (window as any).mountEmailAliases = mount
; (window as any).mountModal = mountModal

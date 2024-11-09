// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Alias, MappingService, ViewState } from './content/types'
import { AliasList } from './content/email_aliases_list'
import { color, spacing, font, radius, typography } from '@brave/leo/tokens/css/variables'
import { createRoot } from 'react-dom/client';
import { EmailAliasModal } from './content/email_aliases_modal';
import { getLocale } from '$web-common/locale'
import { MainEmailEntryForm } from './content/email_aliases_signin_page'
import { RemoteMappingService } from './content/remote_mapping_service'
import * as React from 'react'
import BraveIconCircle from './content/styles/BraveIconCircle'
import Button from '@brave/leo/react/button'
import Card from './content/styles/Card'
import Col from './content/styles/Col'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import LoadingIcon from './content/LoadingIcon'
import Row from './content/styles/Row'
import SecureLink from '$web-common/SecureLink'
import styled, { StyleSheetManager } from 'styled-components'

const PageCol = styled(Col)`
  font: ${font.default.regular};
  padding: 0;
  margin: 0;
  & h4 {
    font: ${font.heading.h4};
    line-height: ${typography.heading.h4.lineHeight};
    margin: 0;
  }
`

const AliasDialog = styled(Dialog)`
  --leo-dialog-backdrop-background: ${color.dialogs.scrimBackground};
  --leo-dialog-padding: ${spacing['2Xl']};
`

const SectionTitle = styled(Card)`
  border-radius: ${radius.m};
  padding: ${spacing['2Xl']} ${spacing['2Xl']} ${spacing.l} ${spacing['2Xl']};
  display: flex;
  flex-direction: column;
  row-gap: ${spacing.m};
`

const MainEmailTextContainer = styled(Col)`
  justify-content: center;
  cursor: default;
  user-select: none;
`

const MainEmail = styled.div`
  font: ${font.default.semibold};
`

const MainEmailDescription = styled.div`
  font: ${font.small.regular};
`

const AccountRow = styled(Row)`
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0;
  & * {
    flex-grow: 0;
    column-gap: ${spacing.xl};
  }
`

const Introduction = () =>
  <SectionTitle>
      <div>
      <h4>{getLocale('emailAliasesShortDescription')}</h4>
      </div>
      <div>
        {getLocale('emailAliasesDescription')}  {
           /* TODO(https://github.com/brave/brave-browser/issues/45408):
           // Link to the email aliases support page */}
        <SecureLink href="https://support.brave.com" target='_blank'>
          {getLocale('emailAliasesLearnMore')}
        </SecureLink>
      </div>
  </SectionTitle>

const MainEmailDisplay = ({ email, onLogout }: { email: string, onLogout: () => void }) =>
  <Card>
    <AccountRow>
      <Row>
        <BraveIconCircle name='social-brave-release-favicon-fullheight-color' />
        <MainEmailTextContainer>
          <MainEmail>{email === '' ? getLocale('emailAliasesConnectingToBraveAccount') : email}</MainEmail>
          <MainEmailDescription>{getLocale('emailAliasesBraveAccount')}</MainEmailDescription>
        </MainEmailTextContainer>
      </Row>
      <Button
        kind='plain-faint'
        title={getLocale('emailAliasesSignOutTitle')}
        size='small'
        onClick={(e) => {
          onLogout()
        }}>
        <Icon slot='icon-before' name="outside" />
        <span>{getLocale('emailAliasesSignOut')}</span>
      </Button>
    </AccountRow>
  </Card>

const SpacedRow = styled(Row)`
  gap: ${spacing.m};
  justify-content: center;
  align-items: center;
  font: ${font.default.semibold};
`

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
}) => (viewState.mode === 'Startup'
        ? <SpacedRow>
            <LoadingIcon />
            <div>{getLocale('emailAliasesConnectingToBraveAccount')}</div>
          </SpacedRow>
        : <span>
            <MainEmailDisplay onLogout={onLogout} email={mainEmail} />
            <AliasList aliases={aliasesState}
              onViewChange={setViewState}
              mappingService={mappingService}
              onListChange={onListChange} />
          </span>)

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
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        onEmailChange();
        onListChange();
      }
    }
    document.addEventListener('visibilitychange', onVisibilityChange)
    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [] /* Only run at mount. */)
  return (
    <PageCol>
      <Introduction />
      {viewState.mode === 'SignUp' || viewState.mode === 'AwaitingAuthorization'
        ? <MainEmailEntryForm
            viewState={viewState}
            mainEmail={mainEmail}
            onEmailSubmitted={onMainEmailSubmitted}
            onRestart={onRestart} />
        : <MainView
            viewState={viewState}
            mainEmail={mainEmail}
            onLogout={onLogout}
            aliasesState={aliasesState}
            setViewState={setViewState}
            mappingService={mappingService}
            onListChange={onListChange} />}
      {(viewState.mode === 'Create' || viewState.mode === 'Edit') &&
      <AliasDialog
        isOpen
        onClose={onReturnToMain}
        backdropClickCloses
        modal
        showClose>
        <EmailAliasModal
          onReturnToMain={onReturnToMain}
            viewState={viewState}
            email={mainEmail}
            mode={viewState.mode}
            mappingService={mappingService} />
        </AliasDialog>}
    </PageCol>
  )
}

export const mount = (at: HTMLElement) => {
  const root = createRoot(at);
  root.render(
    <StyleSheetManager target={at}>
      <ManagePage mappingService={new RemoteMappingService()} />
    </StyleSheetManager>
  )
}

export const mountModal = (at: HTMLElement) => {
  const root = createRoot(at);
  const mappingService = new RemoteMappingService()
  root.render(
    <StyleSheetManager target={at}>
      <EmailAliasModal
        onReturnToMain={mappingService.closeBubble}
        bubble
        mode='Create'
        email='test@test.com'
        mappingService={mappingService} />
    </StyleSheetManager>
  )
}

  ; (window as any).mountEmailAliases = mount
  ; (window as any).mountEmailAliasesModal = mountModal

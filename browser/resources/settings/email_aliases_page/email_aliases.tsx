// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ViewState } from './content/types'
import { AliasList } from './content/email_aliases_list'
import { color, spacing, font, radius, typography } from
  '@brave/leo/tokens/css/variables'
import { createRoot } from 'react-dom/client';
import { EmailAliasModal } from './content/email_aliases_modal';
import { getLocale } from '$web-common/locale'
import { MainEmailEntryForm } from './content/email_aliases_signin_page'
import * as React from 'react'
import BraveIconCircle from './content/styles/brave_icon_circle'
import Button from '@brave/leo/react/button'
import Card from './content/styles/Card'
import Col from './content/styles/Col'
import Dialog from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Row from './content/styles/Row'
import SecureLink from '$web-common/SecureLink'
import styled, { StyleSheetManager } from 'styled-components'
import {
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceRemote,
  EmailAliasesServiceObserverRemote,
  EmailAliasesServiceObserverInterface
} from "gen/brave/components/email_aliases/email_aliases.mojom.m"

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

const MainEmailDisplay = ({ email, emailAliasesService }:
  { email: string, emailAliasesService: EmailAliasesServiceInterface }) =>
  <Card>
    <AccountRow>
      <Row>
        <BraveIconCircle name='social-brave-release-favicon-fullheight-color' />
        <MainEmailTextContainer>
          <MainEmail>
            {email === ''
              ? getLocale('emailAliasesConnectingToBraveAccount')
              : email}
          </MainEmail>
          <MainEmailDescription>
            {getLocale('emailAliasesBraveAccount')}
          </MainEmailDescription>
        </MainEmailTextContainer>
      </Row>
      <Button
        kind='plain-faint'
        title={getLocale('emailAliasesSignOutTitle')}
        size='small'
        onClick={() => {
          emailAliasesService.logout()
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
  viewState, mainEmail, aliasesState, setViewState,
  emailAliasesService
}: {
  viewState: ViewState,
  mainEmail: string,
  aliasesState: Alias[],
  setViewState: (viewState: ViewState) => void,
  emailAliasesService: EmailAliasesServiceInterface
}) => (viewState.mode === 'Startup'
        ? <SpacedRow>
            <ProgressRing />
            <div>{getLocale('emailAliasesConnectingToBraveAccount')}</div>
          </SpacedRow>
        : <span>
            <MainEmailDisplay
              email={mainEmail}
              emailAliasesService={emailAliasesService} />
            <AliasList aliases={aliasesState}
              onViewChange={setViewState}
              emailAliasesService={emailAliasesService} />
          </span>)

export const ManagePage = ({ emailAliasesService, bindObserver }:
  {
    emailAliasesService: EmailAliasesServiceInterface,
    bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
  }) => {
  const [viewState, setViewState] = React.useState<ViewState>(
    { mode: 'Startup' })
  const [mainEmail, setMainEmail] = React.useState<string>('')
  const [aliasesState, setAliasesState] = React.useState<Alias[]>([]);
  const onReturnToMain = () => {
    setViewState({ mode: 'Main' })
  }
  React.useEffect(() => {
    const observer : EmailAliasesServiceObserverInterface = {
      onAliasesUpdated: (aliases: Alias[]) => {
        setAliasesState(aliases)
      },
      onLoggedIn: (email: string) => {
        setMainEmail(email)
        setViewState({ mode: 'Main' })
      },
      onLoggedOut: () => {
        setMainEmail('')
        setViewState({ mode: 'SignUp' })
      },
      onVerificationPending: (email: string) => {
        setViewState({ mode: 'AwaitingAuthorization' })
        setMainEmail(email)
      }
    }
    return bindObserver(observer)
  }, [] /* Only run at mount. */)
  return (
    <PageCol>
      <Introduction />
      {viewState.mode === 'SignUp' || viewState.mode === 'AwaitingAuthorization'
        ? <MainEmailEntryForm
            viewState={viewState}
            mainEmail={mainEmail}
            emailAliasesService={emailAliasesService} />
        : <MainView
            viewState={viewState}
            mainEmail={mainEmail}
            aliasesState={aliasesState}
            setViewState={setViewState}
            emailAliasesService={emailAliasesService} />}
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
            mainEmail={mainEmail}
            aliasCount={aliasesState.length}
            emailAliasesService={emailAliasesService} />
        </AliasDialog>}
    </PageCol>
  )
}

export const mount = (at: HTMLElement) => {
  const root = createRoot(at);
  const emailAliasesService = new EmailAliasesServiceRemote()
  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    emailAliasesService.addObserver(
      new EmailAliasesServiceObserverRemote(observer))
    return () => {
      emailAliasesService.removeObserver(
        new EmailAliasesServiceObserverRemote(observer))
    }
  }
  root.render(
    <StyleSheetManager target={at}>
      <ManagePage emailAliasesService={emailAliasesService}
                  bindObserver={bindObserver} />
    </StyleSheetManager>
  )
}

  ; (window as any).mountEmailAliases = mount

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { formatLocale, getLocale } from "$web-common/locale";
import { onEnterKeyForInput } from "./on_enter_key"
import { ViewState } from "./types"
import * as React from 'react'
import BraveIconCircle from "./styles/brave_icon_circle"
import Button from '@brave/leo/react/button'
import Card from "./styles/Card"
import Col from "./styles/Col"
import formatMessage from '$web-common/formatMessage'
import Input from '@brave/leo/react/input'
import Row from "./styles/Row"
import styled from 'styled-components'
import { spacing } from "@brave/leo/tokens/css/variables";
import { EmailAliasesServiceInterface }
  from "gen/brave/components/email_aliases/email_aliases.mojom.m";

const SignupRow = styled(Row)`
  justify-content: space-between;
  align-items: start;
  gap: ${spacing.xl};
`

const StretchyInput = styled(Input)`
  flex-grow: 1;
`

const LoginRow = styled(Row)`
  align-items: center;
  gap: ${spacing.m};
  & leo-button {
    flex-grow: 0;
  }
`

const SpacedCol = styled(Col)`
  gap: ${spacing.l};
  flex-grow: 1;
`

const BeforeSendingEmailForm = ({ initEmail, emailAliasesService }:
  { initEmail: string, emailAliasesService: EmailAliasesServiceInterface }) => {
  const [email, setEmail] = React.useState<string>(initEmail)
  return <SpacedCol>
    <h4>{getLocale('emailAliasesSignInOrCreateAccount')}</h4>
    <div>{getLocale('emailAliasesEnterEmailToGetLoginLink')}</div>
    <LoginRow>
      <StretchyInput autofocus
        onChange={(detail) => setEmail(detail.value)}
        onKeyDown={onEnterKeyForInput((value) =>
                    emailAliasesService.requestPrimaryEmailVerification(value))}
        name='email'
        type='text'
        placeholder={getLocale('emailAliasesEmailAddressPlaceholder')}
        value={email} />
      <Button
        onClick={() =>
                  emailAliasesService.requestPrimaryEmailVerification(email)}
        type='submit' kind='filled'>
        {getLocale('emailAliasesGetLoginLinkButton')}
      </Button>
    </LoginRow>
  </SpacedCol>
}

const AfterSendingEmailMessage = ({ mainEmail, emailAliasesService }:
  { mainEmail: string, emailAliasesService: EmailAliasesServiceInterface }) => {
  const onClick = (e: React.MouseEvent<HTMLAnchorElement>) => {
    emailAliasesService.cancelPrimaryEmailVerification()
  }
  return <SpacedCol>
    <h4>{formatLocale('emailAliasesLoginEmailOnTheWay', { $1: mainEmail })}</h4>
    <div>{getLocale('emailAliasesClickOnSecureLogin')}</div>
    <div>
      {formatMessage(getLocale('emailAliasesDontSeeEmail'),
       { tags: { $1: (content) => <a href='#'
                                     onClick={onClick}>{content}</a> } })}
    </div>
  </SpacedCol>
}

export const MainEmailEntryForm = (
  { viewState, mainEmail, emailAliasesService }:
  { viewState: ViewState, mainEmail: string,
    emailAliasesService: EmailAliasesServiceInterface }) =>
  <Card>
    <SignupRow>
      <BraveIconCircle name='social-brave-release-favicon-fullheight-color' />
      {viewState.mode === 'SignUp' ?
        <BeforeSendingEmailForm
          initEmail={mainEmail}
          emailAliasesService={emailAliasesService} /> :
        <AfterSendingEmailMessage
          mainEmail={mainEmail}
          emailAliasesService={emailAliasesService} />}
    </SignupRow>
  </Card>
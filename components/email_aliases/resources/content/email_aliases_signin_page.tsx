// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { formatLocale, getLocale } from '$web-common/locale'
import { onEnterKeyForInput } from './on_enter_key'
import { spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import BraveIconCircle from './styles/brave_icon_circle'
import Button from '@brave/leo/react/button'
import Card from './styles/Card'
import Col from './styles/Col'
import Input from '@brave/leo/react/input'
import Row from './styles/Row'
import styled from 'styled-components'
import {
  AuthenticationStatus,
  AuthState,
  EmailAliasesServiceInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

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

const BeforeSendingEmailForm = ({
  suggestedAuthEmail,
  emailAliasesService,
}: {
  suggestedAuthEmail: string
  emailAliasesService: EmailAliasesServiceInterface
}) => {
  const [email, setEmail] = React.useState<string>(suggestedAuthEmail)
  const [errorMessage, setErrorMessage] = React.useState<string | null>(null)
  const requestAuthentication = async () => {
    setErrorMessage(null)
    try {
      await emailAliasesService.requestAuthentication(email)
    } catch (errorMessage) {
      setErrorMessage(errorMessage.toString())
    }
  }

  const emailRef = React.useRef<HTMLInputElement>()
  React.useEffect(() => {
    const input = emailRef.current
    if (!input) {
      return
    }
    setTimeout(() => input.focus())
  }, [])

  return (
    <SpacedCol>
      <h4>{getLocale(S.SETTINGS_EMAIL_ALIASES_SIGN_IN_OR_CREATE_ACCOUNT)}</h4>
      <div>
        {getLocale(S.SETTINGS_EMAIL_ALIASES_ENTER_EMAIL_TO_GET_LOGIN_LINK)}
      </div>
      <LoginRow>
        <StretchyInput
          id="email-input"
          ref={emailRef}
          onInput={(detail) => setEmail(detail.value)}
          onKeyDown={onEnterKeyForInput(requestAuthentication)}
          name='email'
          type='text'
          placeholder={getLocale(
            S.SETTINGS_EMAIL_ALIASES_EMAIL_ADDRESS_PLACEHOLDER,
          )}
          value={email}
        />
        <Button
          id="get-login-link-button"
          onClick={requestAuthentication}
          type='submit'
          kind='filled'
        >
          {getLocale(S.SETTINGS_EMAIL_ALIASES_GET_LOGIN_LINK_BUTTON)}
        </Button>
      </LoginRow>
      {errorMessage && <Alert>{errorMessage}</Alert>}
    </SpacedCol>
  )
}

const AfterSendingEmailMessage = ({
  authEmail,
  emailAliasesService,
  errorMessage,
}: {
  authEmail: string
  emailAliasesService: EmailAliasesServiceInterface
  errorMessage: string | undefined
}) => {
  return (
    <SpacedCol>
      <h4>
        {formatLocale(S.SETTINGS_EMAIL_ALIASES_LOGIN_EMAIL_ON_THE_WAY, {
          $1: authEmail,
        })}
      </h4>
      <div>{getLocale(S.SETTINGS_EMAIL_ALIASES_CLICK_ON_SECURE_LOGIN)}</div>
      <div>{getLocale(S.SETTINGS_EMAIL_ALIASES_DONT_SEE_EMAIL)}</div>
      {errorMessage && <Alert>{errorMessage}</Alert>}
      <div>
        <Button
          onClick={() => emailAliasesService.cancelAuthenticationOrLogout()}
          kind='filled'
        >
          {getLocale(S.SETTINGS_EMAIL_ALIASES_AUTH_TRY_AGAIN_BUTTON)}
        </Button>
      </div>
    </SpacedCol>
  )
}

export const MainEmailEntryForm = ({
  authState,
  emailAliasesService,
}: {
  authState: AuthState
  emailAliasesService: EmailAliasesServiceInterface
}) => (
  <Card>
    <SignupRow>
      <BraveIconCircle name='social-brave-release-favicon-fullheight-color' />
      {authState.status === AuthenticationStatus.kUnauthenticated ? (
        <BeforeSendingEmailForm
          suggestedAuthEmail={authState.email}
          emailAliasesService={emailAliasesService}
        />
      ) : (
        <AfterSendingEmailMessage
          authEmail={authState.email}
          errorMessage={authState.errorMessage}
          emailAliasesService={emailAliasesService}
        />
      )}
    </SignupRow>
  </Card>
)

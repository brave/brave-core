// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from "$web-common/locale";
import { BraveIconCircle, Card, Col, Row } from "./style"
import { ViewState } from "./types"
import * as React from 'react'
import Input, { InputEventDetail } from '@brave/leo/react/input'
import Button from '@brave/leo/react/button'
import formatMessage from '$web-common/formatMessage'
import styled from 'styled-components'

const onEnterKey = (onSubmit: () => void) =>
  (e: InputEventDetail) => {
    const innerEvent = e.innerEvent as unknown as KeyboardEvent
    if (innerEvent.key === 'Enter') {
      onSubmit()
    }
  }

const SignupRow = styled(Row)`
  justify-content: space-between;
  align-items: start;
  & leo-icon {
    margin-top: 1em;
  }
`

const InstructionsDiv = styled.div`
  margin-bottom: 1em;
`

const StretchyInput = styled(Input)`
  flex-grow: 4;
`

const BeforeSendingEmailForm = ({ initEmail, onSubmit }: { initEmail: string, onSubmit: (email: string) => void }) => {
  const [email, setEmail] = React.useState<string>(initEmail)
  return <Col>
    <h3>{getLocale('emailAliasesSignInOrCreateAccount')}</h3>
    <InstructionsDiv>{getLocale('emailAliasesEnterEmailToGetLoginLink')}</InstructionsDiv>
    <Row>
      <StretchyInput autofocus
        onChange={(detail) => setEmail(detail.value)}
        onKeyDown={onEnterKey(() => onSubmit(email))}
        name='email'
        type='text'
        placeholder={getLocale('emailAliasesEmailAddressPlaceholder')}
        value={email}
      ></StretchyInput>
      <Button onClick={() => onSubmit(email)} type='submit' kind='filled'>{getLocale('emailAliasesGetLoginLinkButton')}</Button>
    </Row>
  </Col>
}

const AfterSendingEmailMessage = ({ mainEmail, tryAgain }: { mainEmail: string, tryAgain: () => void }) => {
  const onClick = (e: React.MouseEvent<HTMLAnchorElement>) => {
    e.preventDefault()
    tryAgain()
  }
  return <Col>
    <h3>{formatMessage(getLocale('emailAliasesLoginEmailOnTheWay'), { placeholders: { $1: mainEmail } })}</h3>
    <InstructionsDiv>{getLocale('emailAliasesClickOnSecureLogin')}</InstructionsDiv>
    <InstructionsDiv>
      {formatMessage(getLocale('emailAliasesDontSeeEmail'),
       { tags: { $1: (content) => <a href='#' onClick={onClick}>{content}</a> } })}
    </InstructionsDiv>
  </Col>
}

export const MainEmailEntryForm = (
  { viewState, mainEmail, onEmailSubmitted, onRestart }:
  { viewState: ViewState, mainEmail: string, onEmailSubmitted: (email: string) => void, onRestart: () => void }) =>
  <Card>
    <SignupRow>
      <BraveIconCircle name='brave-icon-release-color' />
      {viewState.mode === 'SignUp' ?
        <BeforeSendingEmailForm initEmail={mainEmail} onSubmit={onEmailSubmitted} /> :
        <AfterSendingEmailMessage mainEmail={mainEmail} tryAgain={onRestart} />}
    </SignupRow>
  </Card>
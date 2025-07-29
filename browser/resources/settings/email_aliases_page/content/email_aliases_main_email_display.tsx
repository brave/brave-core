// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EmailAliasesServiceInterface }
  from "gen/brave/components/email_aliases/email_aliases.mojom.m"
import { font, spacing } from "@brave/leo/tokens/css/variables"
import { getLocale } from '$web-common/locale'
import * as React from 'react'
import BraveIconCircle from "./styles/brave_icon_circle"
import Button from '@brave/leo/react/button'
import Card from "./styles/Card"
import Col from "./styles/Col"
import Icon from '@brave/leo/react/icon'
import Row from "./styles/Row"
import styled from "styled-components"

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

export const MainEmailDisplay = ({ email, emailAliasesService }:
  { email: string, emailAliasesService: EmailAliasesServiceInterface }) => {
  const [cancelAuthenticationOrLogoutPending,
    setCancelAuthenticationOrLogoutPending] = React.useState(false)
  return (
    <Card>
      <AccountRow>
        <Row>
          <BraveIconCircle
            name='social-brave-release-favicon-fullheight-color' />
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
          disabled={cancelAuthenticationOrLogoutPending}
          onClick={async () => {
            setCancelAuthenticationOrLogoutPending(true)
            await emailAliasesService.cancelAuthenticationOrLogout()
            setCancelAuthenticationOrLogoutPending(false)
          }}>
          <Icon slot='icon-before' name="outside" />
          <span>{getLocale('emailAliasesSignOut')}</span>
        </Button>
      </AccountRow>
    </Card>
  )
}

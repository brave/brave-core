// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  color,
  font,
  radius,
  spacing,
  typography,
} from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import { formatLocale, getLocale } from '$web-common/locale'
import Alert from '@brave/leo/react/alert'
import Col from './styles/Col'
import styled from 'styled-components'
import { Alias } from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import './strings'

const SectionCol = styled(Col)`
  row-gap: ${spacing['2Xl']};
`

const LimitAlertTitle = styled.div`
  font: ${font.heading.h4};
  letter-spacing: ${typography.letterSpacing.large};
`

const LimitDescription = styled.div`
  font: ${font.default.regular};
`

const ListLabel = styled.div`
  font: ${font.small.semibold};
  line-height: ${typography.lineHeight.small};
  margin: 0;
  padding: 0 ${spacing.s};
`

const AliasListBox = styled.div`
  border: 1px solid ${color.divider.subtle};
  border-radius: ${radius.xl};
  overflow: hidden auto;
  max-height: 208px;
`

const AliasRow = styled.div<{ showDivider: boolean }>`
  font-weight: ${font.default.semibold};
  padding: ${spacing.m} ${spacing.l};
  gap: 8px;
  border-bottom: ${(p) =>
    p.showDivider ? `1px solid ${color.divider.subtle}` : 'none'};
`

export const EmailAliasLimitReached = ({
  aliases,
  aliasLimit,
}: {
  aliases: Alias[]
  aliasLimit: number
}) => {
  const count = aliases.length
  return (
    <SectionCol>
      <Alert type='warning'>
        <LimitAlertTitle>
          {getLocale(S.SETTINGS_EMAIL_ALIASES_LIMIT_REACHED_ALERT_TITLE)}
        </LimitAlertTitle>
        <LimitDescription>
          {formatLocale(S.SETTINGS_EMAIL_ALIASES_LIMIT_REACHED_ALERT_BODY, {
            $1: String(aliasLimit),
          })}
        </LimitDescription>
      </Alert>
      <div>
        <ListLabel>
          {formatLocale(S.SETTINGS_EMAIL_ALIASES_YOUR_ALIASES_COUNT_LABEL, {
            $1: String(count),
            $2: String(aliasLimit),
          })}
        </ListLabel>
        <AliasListBox>
          {aliases.map((alias, index) => (
            <AliasRow
              key={alias.email}
              showDivider={index < aliases.length - 1}
            >
              {alias.email}
            </AliasRow>
          ))}
        </AliasListBox>
      </div>
    </SectionCol>
  )
}

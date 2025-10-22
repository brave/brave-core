// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'
import { radius, spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import Card from './styles/Card'
import SecureLink from '$web-common/SecureLink'
import styled from 'styled-components'

const SectionTitle = styled(Card)`
  border-radius: ${radius.m};
  padding: ${spacing['2Xl']} ${spacing['2Xl']} ${spacing.l} ${spacing['2Xl']};
  display: flex;
  flex-direction: column;
  row-gap: ${spacing.m};
`

export const Introduction = () =>
  <SectionTitle>
      <div>
      <h4>{getLocale('emailAliasesShortDescription')}</h4>
      </div>
      <div>
        {getLocale('emailAliasesDescription')}  {
           /* TODO(https://github.com/brave/brave-browser/issues/45408):
           // Link to the email aliases support page */}
        <SecureLink href="https://support.brave.app" target='_blank'>
          {getLocale('emailAliasesLearnMore')}
        </SecureLink>
      </div>
  </SectionTitle>

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  DeleteAliasModal,
  EmailAliasModal,
} from '../content/email_aliases_modal'
import { StubEmailAliasesService, demoData } from './utils/stubs'
import { AuthenticationStatus } from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

const stubEmailAliasesServiceAccountReadyInstance = new StubEmailAliasesService(
  {
    status: AuthenticationStatus.kAuthenticated,
    email: demoData.email,
    errorMessage: undefined,
  },
)

export const NewAliasDialog = () => {
  return (
    <div style={{ width: '420px', margin: '0 auto' }}>
      <EmailAliasModal
        aliasCount={demoData.aliases.length}
        onReturnToMain={() => {}}
        editing={false}
        mainEmail={demoData.email}
        // @ts-expect-error https://github.com/brave/brave-browser/issues/48960
        emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      />
    </div>
  )
}

export const EditAliasDialog = () => {
  return (
    <div style={{ width: '420px', margin: '0 auto' }}>
      <EmailAliasModal
        editAlias={demoData.aliases[0]}
        aliasCount={demoData.aliases.length}
        onReturnToMain={() => {}}
        editing
        mainEmail={demoData.email}
        // @ts-expect-error https://github.com/brave/brave-browser/issues/48960
        emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      />
    </div>
  )
}

export const DeleteAliasDialog = () => {
  return (
    <div style={{ width: '420px', margin: '0 auto' }}>
      <DeleteAliasModal
        onReturnToMain={() => {}}
        alias={demoData.aliases[0]}
        // @ts-expect-error https://github.com/brave/brave-browser/issues/48960
        emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      />
    </div>
  )
}

export const Panel = () => {
  return (
    <div style={{ width: '420px', margin: '0 auto' }}>
      <EmailAliasModal
        aliasCount={demoData.aliases.length}
        onReturnToMain={() => {}}
        editing={false}
        mainEmail={demoData.email}
        bubble={true}
        // @ts-expect-error https://github.com/brave/brave-browser/issues/48960
        emailAliasesService={stubEmailAliasesServiceAccountReadyInstance}
      />
    </div>
  )
}

export default {
  title: 'Email Aliases/EmailAliasModal',
}

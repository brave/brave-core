// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  defaultContext,
  UntrustedConversationContext,
  UntrustedConversationReactContext
} from './untrusted_conversation_context'

export default function MockContext(
  props: React.PropsWithChildren<Partial<UntrustedConversationContext>>
) {
  return (
    <UntrustedConversationReactContext.Provider
      value={{
        ...defaultContext,
        ...props
      }}
    >
      {props.children}
    </UntrustedConversationReactContext.Provider>
  )
}
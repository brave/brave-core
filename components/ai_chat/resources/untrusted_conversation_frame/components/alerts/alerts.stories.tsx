// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import UntrustedMockContext from '../../mock_untrusted_conversation_context'
import ErrorConnection from './error_connection'
import ErrorConversationEnd from './error_conversation_end'
import ErrorInvalidAPIKey from './error_invalid_api_key'
import ErrorInvalidEndpointURL from './error_invalid_endpoint_url'
import ErrorRateLimit from './error_rate_limit'
import ErrorServiceOverloaded from './error_service_overloaded'
import LongConversationInfo from './long_conversation_info'
import WarningPremiumDisconnected from './warning_premium_disconnected'
import styles from '../../../page/stories/style.module.scss'

export default {
  title: 'AI Chat/Alerts',
} as Meta

export const _Alerts = {
  render: () => {
    return (
      <UntrustedMockContext>
        <div className={`${styles.container} ${styles.containerAlerts}`}>
          <ErrorConnection />
          <ErrorConversationEnd />
          <ErrorInvalidAPIKey />
          <ErrorInvalidEndpointURL />
          <ErrorRateLimit />
          <ErrorServiceOverloaded />
          <LongConversationInfo />
          <WarningPremiumDisconnected />
        </div>
      </UntrustedMockContext>
    )
  },
}

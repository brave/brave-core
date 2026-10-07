/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { getLocale, formatLocale } from '$web-common/locale'
import { mojoTimeToJSDate } from '$web-common/mojomUtils'
import * as Mojom from '../../../common/mojom'
import { useUntrustedConversationContext } from '../../untrusted_conversation_context'
import PremiumSuggestion from '../premium_suggestion'
import styles from './alerts.module.scss'

const MS_PER_MINUTE = 60 * 1000
const MS_PER_HOUR = 60 * 60 * 1000

interface Props {
  _testIsCurrentModelLeo?: boolean
  // Either RateLimitReached or ModelRateLimitReached (the premium rate limit
  // was reached for the current model only).
  apiError: Mojom.APIError
  errorDetails?: Mojom.APIErrorDetails | null
}

function formatDurationUntil(date: Date) {
  // Round up so the limit never appears to have already reset.
  const duration = Temporal.Now.instant().until(
    Temporal.Instant.fromEpochMilliseconds(date.getTime()),
    { largestUnit: 'hours', smallestUnit: 'minutes', roundingMode: 'ceil' },
  )
  return new Intl.DurationFormat(undefined, { style: 'long' }).format(duration)
}

function getModelRateLimitMessage(errorDetails?: Mojom.APIErrorDetails | null) {
  if (!errorDetails?.rateLimitExpiresAt) {
    return getLocale(S.CHAT_UI_ERROR_MODEL_RATE_LIMIT)
  }
  const expiresAt = mojoTimeToJSDate(errorDetails.rateLimitExpiresAt)
  if (expiresAt.getTime() <= Date.now()) {
    return getLocale(S.CHAT_UI_ERROR_MODEL_RATE_LIMIT)
  }
  const expiresTime = new Intl.DateTimeFormat(undefined, {
    dateStyle: 'medium',
    timeStyle: 'short',
  }).format(expiresAt)
  return formatLocale(S.CHAT_UI_ERROR_MODEL_RATE_LIMIT_WITH_TIME, {
    $1: formatDurationUntil(expiresAt),
    $2: expiresTime,
  })
}

function ErrorRateLimit(props: Props) {
  const context = useUntrustedConversationContext()
  const state = context.api.useState().data
  const { isPremiumUser } = context.api.useGetPremiumStatusData()
  const expiresAt = props.errorDetails?.rateLimitExpiresAt

  // Re-render every minute while the rate limit has an expiry so the duration
  // in the message stays current and retry button visibility is re-evaluated.
  const [, setTick] = React.useState(0)
  React.useEffect(() => {
    if (!expiresAt) {
      return
    }
    const intervalId = window.setInterval(
      () => setTick((tick) => tick + 1),
      MS_PER_MINUTE,
    )
    return () => window.clearInterval(intervalId)
  }, [expiresAt])

  // Respond to BYOM scenarios
  if (!state.isLeoModel || props._testIsCurrentModelLeo === false) {
    return (
      <div className={styles.alert}>
        <Alert type='warning'>
          {getLocale(S.CHAT_UI_ERROR_OAI_RATE_LIMIT)}
          <Button
            slot='actions'
            kind='filled'
            onClick={() => context.conversationHandler.retryAPIRequest()}
          >
            {getLocale(S.CHAT_UI_RETRY_BUTTON_LABEL)}
          </Button>
        </Alert>
      </div>
    )
  }

  // Respond to Leo (i.e., non-BYOM) scenarios
  if (!isPremiumUser) {
    return (
      <PremiumSuggestion
        title={getLocale(S.CHAT_UI_RATE_LIMIT_REACHED_TITLE)}
        description={getLocale(S.CHAT_UI_RATE_LIMIT_REACHED_DESC)}
        secondaryActionButton={
          <Button
            kind='plain-faint'
            onClick={() => context.parentUiFrame.handleResetError()}
          >
            {getLocale(S.AI_CHAT_MAYBE_LATER_LABEL)}
          </Button>
        }
      />
    )
  }

  // Hide the retry button for model rate limits that won't expire for over an
  // hour, since retrying before then will fail.
  const isModelRateLimit =
    props.apiError === Mojom.APIError.ModelRateLimitReached
  const showRetry =
    !isModelRateLimit
    || !expiresAt
    || mojoTimeToJSDate(expiresAt).getTime() - Date.now() <= MS_PER_HOUR

  return (
    <div className={styles.alert}>
      <Alert type='warning'>
        {isModelRateLimit
          ? getModelRateLimitMessage(props.errorDetails)
          : getLocale(S.CHAT_UI_ERROR_RATE_LIMIT)}
        {showRetry && (
          <Button
            slot='actions'
            kind='filled'
            onClick={() => context.conversationHandler.retryAPIRequest()}
          >
            {getLocale(S.CHAT_UI_RETRY_BUTTON_LABEL)}
          </Button>
        )}
      </Alert>
    </div>
  )
}

export default ErrorRateLimit

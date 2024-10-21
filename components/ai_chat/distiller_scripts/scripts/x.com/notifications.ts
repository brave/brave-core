/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getXProps, store } from './data'
import { getUserSignature } from './user'
import { wrapLines } from './utils'

export function distillNotificationElement(element: HTMLElement) {
  const notificationXProps = getXProps(element.parentElement)
  const notificationId = notificationXProps?.children?.props?.entry?.content?.id
  const notificationURL =
    notificationXProps?.children?.props?.entry?.content?.url?.url
  const notificationData = store.access('notifications', notificationId)

  const distilled: Array<string | null> = []

  if (notificationData) {
    const message = distillNotificationData(notificationData)
    distilled.push(
      message && `Notification: ${message}`,
      notificationURL && `URL: ${notificationURL}`
    )
  }

  return distilled.filter(Boolean).join('\n')
}

function distillNotificationData(notification: any) {
  // TODO (Sampson): Possibly handle other notification templates
  if (notification.template?.aggregateUserActionsV1) {
    return distillNotificationDataAggregateUserActionsV1(notification)
  }
  return null
}

function distillNotificationDataAggregateUserActionsV1(notification: any) {
  const {
    message: { text, entities },
    template: { aggregateUserActionsV1 }
  } = notification

  let distilled: string = text

  if (entities.length > 0) {
    /**
     * Walk through the entities in reverse order so as not to cause
     * issues with indexes as we modify the string.
     * TODO (Sampson): Indexes do not account for code points, so this
     * could potentially cause issues with multi-byte characters. We'll
     * need to revisit this to make sure signatures with emojis are
     * handled correctly.
     */
    for (let i = entities.length - 1; i >= 0; i--) {
      const { fromIndex, toIndex, format, ref } = entities[i]

      if (typeof format === 'string') {
        if (format.toLowerCase() === 'strong') {
          // We'll convert strong text to uppercase
          const strong = distilled.slice(fromIndex, toIndex).toUpperCase()
          const front = distilled.slice(0, fromIndex)
          const back = distilled.slice(toIndex)
          distilled = front + strong + back
        } else {
          console.warn(`Unhandled format: ${format}`)
        }
      }

      if (ref) {
        if (ref.user) {
          const user = store.access('users', ref.user.id)
          if (user) {
            const signature = getUserSignature(user)
            const front = distilled.slice(0, fromIndex)
            const back = distilled.slice(toIndex)
            distilled = front + signature + back
          }
        } else {
          console.warn(`Unhandled ref: ${ref}`)
        }
      }
    }
  }

  /**
   * For now we'll just pull in the text of the first associated post (if any).
   */
  if (aggregateUserActionsV1.targetObjects?.length > 0) {
    const first = aggregateUserActionsV1.targetObjects[0]
    if (first.tweet) {
      const tweet = store.access('tweets', first.tweet)
      if (tweet) {
        const text = wrapLines(tweet.full_text)
        if (text) {
          distilled += `\n\n"${text}"`
        }
      }
    }
  }

  return distilled
}

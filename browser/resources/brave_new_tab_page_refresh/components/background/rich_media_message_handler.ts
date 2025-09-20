/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { openLink } from '../common/link'
import { useBackgroundActions } from '../../context/background_context'

import {
  RichMediaFrameHandle,
  handleRawIncomingMessage } from './rich_media_capabilities'

// Returns a callback that will handle messages received from the rich media
// background iframe.
export function useRichMediaMessageHandler(
  destinationUrl: string | undefined,
  frameHandle: RichMediaFrameHandle | undefined
) {
  const actions = useBackgroundActions()

  return React.useCallback((data: unknown) => {
    handleRawIncomingMessage(data, {
      notifyAdEvent: actions.notifySponsoredRichMediaEvent,
      openDestinationUrl: () => {
        if (destinationUrl) {
          openLink(destinationUrl)
        }
      }
    })
  }, [destinationUrl, frameHandle])
}

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import Button from '@brave/leo/react/button'

import { braveSearchHost } from '../../state/search_store'
import { SponsoredImageBackground } from '../../state/background_store'
import { useBackgroundState } from '../../context/background_context'
import { useSearchActions } from '../../context/search_context'
import { IframeBackground, IframeBackgroundHandle } from './iframe_background'

import {
  useRichMediaMessageHandler,
  useSafeAreaReporter,
} from './rich_media_capability_provider'

import { getString } from '../../lib/strings'

interface Props {
  background: SponsoredImageBackground
}

export function RichMediaBackground(props: Props) {
  const searchActions = useSearchActions()
  const sponsoredRichMediaBaseUrl = useBackgroundState(
    (s) => s.sponsoredRichMediaBaseUrl,
  )
  const [showMakeDefaultDialog, setShowMakeDefaultDialog] =
    React.useState(false)

  const [frameHandle, setFrameHandle] = React.useState<IframeBackgroundHandle>()

  const messageHandler = useRichMediaMessageHandler(frameHandle, {
    destinationUrl: props.background.logo?.destinationUrl,
    onMakeBraveSearchDefault: () => setShowMakeDefaultDialog(true),
  })

  useSafeAreaReporter(frameHandle)

  return (
    <>
      <IframeBackground
        url={props.background.imageUrl}
        expectedOrigin={new URL(sponsoredRichMediaBaseUrl).origin}
        onReady={setFrameHandle}
        onMessage={messageHandler}
      />
      <Dialog
        isOpen={showMakeDefaultDialog}
        showClose
        onClose={() => setShowMakeDefaultDialog(false)}
      >
        <h3 slot='title'>{getString(S.NEW_TAB_MAKE_SEARCH_DEFAULT_TITLE)}</h3>
        <p>{getString(S.NEW_TAB_MAKE_SEARCH_DEFAULT_TEXT)}</p>
        <div slot='actions'>
          <Button
            onClick={() => {
              searchActions.setDefaultSearchEngine(braveSearchHost)
              setShowMakeDefaultDialog(false)
            }}
          >
            {getString(S.NEW_TAB_MAKE_SEARCH_DEFAULT_CONFIRM_LABEL)}
          </Button>
          <Button
            kind='outline'
            onClick={() => setShowMakeDefaultDialog(false)}
          >
            {getString(S.NEW_TAB_CANCEL_BUTTON_LABEL)}
          </Button>
        </div>
      </Dialog>
    </>
  )
}

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import * as NTPBackgroundMediaMojom from 'gen/brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.m.js'
import * as NewTabTakeoverMojom from 'gen/brave/components/brave_new_tab_ui/new_tab_takeover/mojom/new_tab_takeover.mojom.m.js'
import * as BraveAdsMojom from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'
import { Url } from 'gen/url/mojom/url.mojom.m.js'

import {
  SponsoredRichMediaBackgroundInfo, SponsoredRichMediaBackground
} from '../containers/newTab/sponsored_rich_media_background'

export default function App(props: React.PropsWithChildren) {
  const [sponsoredRichMediaBackgroundInfo, setSponsoredRichMediaBackgroundInfo] = React.useState<SponsoredRichMediaBackgroundInfo | null>(null)
  const [sponsoredRichMediaAdEventHandler, setSponsoredRichMediaAdEventHandler] = React.useState<NTPBackgroundMediaMojom.SponsoredRichMediaAdEventHandlerRemote | null>(null)
  const [newTabTakeover, setNewTabTakeover] = React.useState<NewTabTakeoverMojom.NewTabTakeoverRemote | null>(null)
  const [richMediaHasLoaded, setRichMediaHasLoaded] = React.useState(false)

  React.useEffect(() => {
    const params = new URLSearchParams(window.location.search)
    const wallpaperUrl = params.get('wallpaperUrl')
    const creativeInstanceId = params.get('creativeInstanceId')
    const placementId = params.get('placementId')
    const targetUrl = params.get('targetUrl')

    if (!wallpaperUrl || !creativeInstanceId || !placementId) {
      return
    }

    const sponsoredRichMediaBackgroundInfo: SponsoredRichMediaBackgroundInfo = {
      url: wallpaperUrl,
      creativeInstanceId: creativeInstanceId,
      placementId: placementId,
      targetUrl: targetUrl ?? ''
    }
    setSponsoredRichMediaBackgroundInfo(sponsoredRichMediaBackgroundInfo)

    const newTabTakeover = NewTabTakeoverMojom.NewTabTakeover.getRemote();
    setNewTabTakeover(newTabTakeover)

    const sponsoredRichMediaAdEventHandler = new NTPBackgroundMediaMojom.SponsoredRichMediaAdEventHandlerRemote()
    newTabTakeover.setSponsoredRichMediaAdEventHandler(sponsoredRichMediaAdEventHandler.$.bindNewPipeAndPassReceiver())
    setSponsoredRichMediaAdEventHandler(sponsoredRichMediaAdEventHandler)

    return () => {
      setSponsoredRichMediaBackgroundInfo(null)
      setNewTabTakeover(null)
      setSponsoredRichMediaAdEventHandler(null)
    }
  }, [])

  if (!sponsoredRichMediaBackgroundInfo || !sponsoredRichMediaAdEventHandler || !newTabTakeover) {
    return null
  }

  return (
    <SponsoredRichMediaBackground
      sponsoredRichMediaBackgroundInfo={sponsoredRichMediaBackgroundInfo}
      richMediaHasLoaded={richMediaHasLoaded}
      onLoaded={() => { setRichMediaHasLoaded(true) }}
      onEventReported={(adEventType) => {
        sponsoredRichMediaAdEventHandler.reportRichMediaAdEvent(
          sponsoredRichMediaBackgroundInfo.creativeInstanceId,
          sponsoredRichMediaBackgroundInfo.placementId,
          adEventType)

        if (adEventType === BraveAdsMojom.NewTabPageAdEventType.kClicked) {
          const mojomUrl = new Url()
          mojomUrl.url = sponsoredRichMediaBackgroundInfo.targetUrl
          newTabTakeover.navigateToUrl(mojomUrl)
        }
      }}
    />
  )
}

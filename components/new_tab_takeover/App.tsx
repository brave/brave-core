// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import * as NTPBackgroundMediaMojom from 'gen/brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.m.js'
import * as NewTabTakeoverMojom from 'gen/brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.m.js'
import * as BraveAdsMojom from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'
import { Url } from 'gen/url/mojom/url.mojom.m.js'

import {
  SponsoredRichMediaBackgroundInfo, SponsoredRichMediaBackground
} from '../brave_new_tab_ui/containers/newTab/sponsored_rich_media_background'

export default function App(props: React.PropsWithChildren) {
  const [sponsoredRichMediaBackgroundInfo, setSponsoredRichMediaBackgroundInfo] = React.useState<SponsoredRichMediaBackgroundInfo | null>(null)
  const [sponsoredRichMediaAdEventHandler, setSponsoredRichMediaAdEventHandler] = React.useState<NTPBackgroundMediaMojom.SponsoredRichMediaAdEventHandlerRemote | null>(null)
  const [newTabTakeover, setNewTabTakeover] = React.useState<NewTabTakeoverMojom.NewTabTakeoverRemote | null>(null)
  const [richMediaHasLoaded, setRichMediaHasLoaded] = React.useState(false)

  const getCurrentWallpaper = React.useCallback(async () => {
    if (!newTabTakeover) {
      return
    }
    try {
      const response = await newTabTakeover.getCurrentWallpaper();
      if (!response || !response.url || !response.placementId ||
          !response.creativeInstanceId || !response.targetUrl) {
        return
      }

      const sponsoredRichMediaBackgroundInfo: SponsoredRichMediaBackgroundInfo = {
        url: response.url.url,
        creativeInstanceId: response.creativeInstanceId,
        placementId: response.placementId,
        targetUrl: response.targetUrl.url
      }
      setSponsoredRichMediaBackgroundInfo(sponsoredRichMediaBackgroundInfo)
    } catch (error) {
      console.error('Failed to get last displayed branded wallpaper:', error);
    }
  }, [newTabTakeover]);

  React.useEffect(() => {
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

  React.useEffect(() => {
    getCurrentWallpaper()
  }, [getCurrentWallpaper, newTabTakeover])

  return (
    <React.Fragment>
      {sponsoredRichMediaBackgroundInfo && sponsoredRichMediaAdEventHandler && newTabTakeover && (
        <SponsoredRichMediaBackground
          sponsoredRichMediaBackgroundInfo={sponsoredRichMediaBackgroundInfo}
          richMediaHasLoaded={richMediaHasLoaded}
          onLoaded={() => setRichMediaHasLoaded(true)}
          onEventReported={(adEventType) => {
            sponsoredRichMediaAdEventHandler.reportRichMediaAdEvent(
              sponsoredRichMediaBackgroundInfo.creativeInstanceId,
              sponsoredRichMediaBackgroundInfo.placementId,
              adEventType
            );

            if (adEventType === BraveAdsMojom.NewTabPageAdEventType.kClicked) {
              const mojomUrl = new Url();
              mojomUrl.url = sponsoredRichMediaBackgroundInfo.targetUrl;
              newTabTakeover.navigateToUrl(mojomUrl);
            }
          }}
        />
      )}
    </React.Fragment>
  )
}

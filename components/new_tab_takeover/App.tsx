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

function sanitizeId(value: string | null): string | null {
  if (!value || value.length === 0) {
    return null;
  }

  // Restrict input to alphanumeric characters and hyphens to prevent
  // potential injections.
  if (!/^[0-9a-fA-F-]+$/.test(value)) {
    return null;
  }

  return value;
}

function useParametersFromQuery(): { placementId: string | null;
                                     creativeInstanceId: string | null } {
  return React.useMemo(() => {
    const urlParams = new URLSearchParams(window.location.search);
    const placementId = urlParams.get('placementId');
    const creativeInstanceId = urlParams.get('creativeInstanceId');

    const sanitizedPlacementId = sanitizeId(placementId)
    const sanitizedCreativeInstanceId = sanitizeId(creativeInstanceId)
    console.log('SponsoredRichMedia: parsed params, placementId=', sanitizedPlacementId,
      'creativeInstanceId=', sanitizedCreativeInstanceId)

    return {
      placementId: sanitizedPlacementId,
      creativeInstanceId: sanitizedCreativeInstanceId
    };
  }, []);
}

export default function App(props: React.PropsWithChildren) {
  const { placementId, creativeInstanceId } = useParametersFromQuery();
  const [sponsoredRichMediaBackgroundInfo, setSponsoredRichMediaBackgroundInfo] = React.useState<SponsoredRichMediaBackgroundInfo | null>(null)
  const [sponsoredRichMediaAdEventHandler, setSponsoredRichMediaAdEventHandler] = React.useState<NTPBackgroundMediaMojom.SponsoredRichMediaAdEventHandlerRemote | null>(null)
  const [newTabTakeover, setNewTabTakeover] = React.useState<NewTabTakeoverMojom.NewTabTakeoverRemote | null>(null)
  const [richMediaHasLoaded, setRichMediaHasLoaded] = React.useState(false)

  const getCurrentWallpaper = React.useCallback(async () => {
    if (!newTabTakeover || !placementId || !creativeInstanceId) {
      console.warn('SponsoredRichMedia: skipping getCurrentWallpaper, prerequisites missing,'
        + ' newTabTakeover=', !!newTabTakeover,
        ' placementId=', placementId,
        ' creativeInstanceId=', creativeInstanceId)
      return
    }

    try {
      const response =
          await newTabTakeover.getCurrentWallpaper(creativeInstanceId);
      console.log('SponsoredRichMedia: getCurrentWallpaper response=', response)
      if (!response || !response.url || !response.targetUrl) {
        console.warn('SponsoredRichMedia: getCurrentWallpaper response missing required fields')
        return
      }

      const sponsoredRichMediaBackgroundInfo: SponsoredRichMediaBackgroundInfo = {
        url: response.url.url,
        placementId: placementId,
        creativeInstanceId: creativeInstanceId,
        metricType: response.metricType,
        targetUrl: response.targetUrl.url
      }
      console.log('SponsoredRichMedia: setting background info, url=',
        sponsoredRichMediaBackgroundInfo.url)
      setSponsoredRichMediaBackgroundInfo(sponsoredRichMediaBackgroundInfo)
    } catch (error) {
      console.error('Failed to get last displayed branded wallpaper:', error);
    }
  }, [newTabTakeover, placementId, creativeInstanceId]);

  React.useEffect(() => {
    const newTabTakeover = NewTabTakeoverMojom.NewTabTakeover.getRemote();
    setNewTabTakeover(newTabTakeover)

    const sponsoredRichMediaAdEventHandler = new NTPBackgroundMediaMojom.SponsoredRichMediaAdEventHandlerRemote()
    newTabTakeover.setSponsoredRichMediaAdEventHandler(sponsoredRichMediaAdEventHandler.$.bindNewPipeAndPassReceiver())
    setSponsoredRichMediaAdEventHandler(sponsoredRichMediaAdEventHandler)
    console.log('SponsoredRichMedia: Mojo setup initiated, handler receiver bound locally.')

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
          onLoaded={() => {
            console.log('SponsoredRichMedia: rich media loaded, setting richMediaHasLoaded=true.')
            setRichMediaHasLoaded(true)
          }}
          onEventReported={(adEventType) => {
            sponsoredRichMediaAdEventHandler.maybeReportRichMediaAdEvent(
              sponsoredRichMediaBackgroundInfo.placementId,
              sponsoredRichMediaBackgroundInfo.creativeInstanceId,
              sponsoredRichMediaBackgroundInfo.metricType,
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

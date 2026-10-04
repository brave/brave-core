// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react';
import * as NTPBackgroundMediaMojom from 'gen/brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.m.js'
import * as NewTabTakeoverMojom from 'gen/brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.m.js'
import * as BraveAdsMojom from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import type { RectF } from 'gen/ui/webui/resources/tsc/mojo/ui/gfx/geometry/mojom/geometry.mojom-webui'

import {
  DynamicNewTabTakeoverInfo, DynamicNewTabTakeover, DynamicContentSearchMatch
} from '../brave_new_tab_ui/containers/newTab/dynamic_new_tab_takeover'

// Mirrors `kBraveSearchHost` (brave/components/constants/url_constants.h).
// Not imported directly since that constant is only exposed to C++/desktop's
// separate webpack bundle.
const braveSearchHost = 'search.brave.com'

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

    return {
      placementId: sanitizeId(placementId),
      creativeInstanceId: sanitizeId(creativeInstanceId)
    };
  }, []);
}

export default function App(props: React.PropsWithChildren) {
  const { placementId, creativeInstanceId } = useParametersFromQuery();
  const [dynamicNewTabTakeoverInfo, setDynamicNewTabTakeoverInfo] = React.useState<DynamicNewTabTakeoverInfo | null>(null)
  const [sponsoredContentAdEventHandler, setSponsoredContentAdEventHandler] = React.useState<NTPBackgroundMediaMojom.SponsoredContentAdEventHandlerRemote | null>(null)
  const [newTabTakeover, setNewTabTakeover] = React.useState<NewTabTakeoverMojom.NewTabTakeoverRemote | null>(null)
  const [dynamicContentHasLoaded, setDynamicContentHasLoaded] = React.useState(false)
  const [safeArea, setSafeArea] = React.useState<RectF | undefined>()
  const [searchMatches, setSearchMatches] = React.useState<DynamicContentSearchMatch[] | undefined>(undefined)

  const getCurrentWallpaper = React.useCallback(async () => {
    if (!newTabTakeover || !placementId || !creativeInstanceId) {
      return
    }

    try {
      const response =
          await newTabTakeover.getCurrentWallpaper(creativeInstanceId);
      if (!response || !response.url || !response.targetUrl) {
        return
      }

      const dynamicNewTabTakeoverInfo: DynamicNewTabTakeoverInfo = {
        url: response.url.url,
        placementId: placementId,
        creativeInstanceId: creativeInstanceId,
        metricType: response.metricType,
        targetUrl: response.targetUrl.url
      }
      setDynamicNewTabTakeoverInfo(dynamicNewTabTakeoverInfo)
    } catch (error) {
      console.error('Failed to get last displayed branded wallpaper:', error);
    }
  }, [newTabTakeover, placementId, creativeInstanceId]);

  React.useEffect(() => {
    const newTabTakeover = NewTabTakeoverMojom.NewTabTakeover.getRemote();
    setNewTabTakeover(newTabTakeover)

    const pageCallbackRouter = new NewTabTakeoverMojom.NewTabTakeoverPageCallbackRouter()
    pageCallbackRouter.setSafeArea.addListener((safeArea: RectF) => {
      setSafeArea(safeArea)
    })
    newTabTakeover.setPage(pageCallbackRouter.$.bindNewPipeAndPassRemote())

    const sponsoredContentAdEventHandler = new NTPBackgroundMediaMojom.SponsoredContentAdEventHandlerRemote()
    newTabTakeover.setSponsoredContentAdEventHandler(sponsoredContentAdEventHandler.$.bindNewPipeAndPassReceiver())
    setSponsoredContentAdEventHandler(sponsoredContentAdEventHandler)

    return () => {
      pageCallbackRouter.$.close()
      setDynamicNewTabTakeoverInfo(null)
      setNewTabTakeover(null)
      setSponsoredContentAdEventHandler(null)
      setSafeArea(undefined)
    }
  }, [])

  React.useEffect(() => {
    getCurrentWallpaper()
  }, [getCurrentWallpaper, newTabTakeover])

  const onOpenBraveSearch = React.useCallback((query: string) => {
    if (!newTabTakeover) {
      return
    }
    const mojomUrl = new Url();
    mojomUrl.url = `https://${braveSearchHost}/search?q=${encodeURIComponent(query)}`;
    newTabTakeover.navigateToUrl(mojomUrl);
  }, [newTabTakeover])

  const onQueryAutocomplete = React.useCallback(async (query: string) => {
    if (!newTabTakeover) {
      return
    }
    try {
      const { matches } = await newTabTakeover.queryAutocomplete(query);
      setSearchMatches(matches.map((match): DynamicContentSearchMatch => ({
        contents: match.contents,
        description: match.description,
        destinationUrl: match.destinationUrl.url,
        iconUrl: match.iconUrl.url,
        imageUrl: match.imageUrl.url,
        allowedToBeDefaultMatch: match.allowedToBeDefaultMatch
      })))
    } catch (error) {
      console.error('Failed to query Brave Search autocomplete:', error);
    }
  }, [newTabTakeover])

  const onMakeBraveSearchDefault = React.useCallback(async () => {
    if (!newTabTakeover) {
      return
    }
    try {
      const { success } = await newTabTakeover.setDefaultSearchEngineAsBraveSearch();
      if (!success) {
        console.error('Failed to set Brave Search as the default search engine');
      }
    } catch (error) {
      console.error('Failed to set Brave Search as the default search engine:', error);
    }
  }, [newTabTakeover])

  const reportAdEvent = React.useCallback((adEventType: BraveAdsMojom.NewTabPageAdEventType) => {
    if (!sponsoredContentAdEventHandler || !dynamicNewTabTakeoverInfo) {
      return
    }
    sponsoredContentAdEventHandler.maybeReportSponsoredContentAdEvent(
      dynamicNewTabTakeoverInfo.placementId,
      dynamicNewTabTakeoverInfo.creativeInstanceId,
      dynamicNewTabTakeoverInfo.metricType,
      adEventType
    );
  }, [sponsoredContentAdEventHandler, dynamicNewTabTakeoverInfo])

  const onEventReported = React.useCallback((adEventType: BraveAdsMojom.NewTabPageAdEventType) => {
    reportAdEvent(adEventType)

    if (adEventType === BraveAdsMojom.NewTabPageAdEventType.kClicked && dynamicNewTabTakeoverInfo) {
      const mojomUrl = new Url();
      mojomUrl.url = dynamicNewTabTakeoverInfo.targetUrl;
      newTabTakeover?.navigateToUrl(mojomUrl);
    }
  }, [reportAdEvent, dynamicNewTabTakeoverInfo, newTabTakeover])

  return (
    <React.Fragment>
      {dynamicNewTabTakeoverInfo && sponsoredContentAdEventHandler && newTabTakeover && (
        <DynamicNewTabTakeover
          dynamicNewTabTakeoverInfo={dynamicNewTabTakeoverInfo}
          dynamicContentHasLoaded={dynamicContentHasLoaded}
          safeArea={safeArea}
          onLoaded={() => setDynamicContentHasLoaded(true)}
          onEventReported={onEventReported}
          onAdEventReported={reportAdEvent}
          searchMatches={searchMatches}
          onOpenBraveSearch={onOpenBraveSearch}
          onQueryAutocomplete={onQueryAutocomplete}
          onMakeBraveSearchDefault={onMakeBraveSearchDefault}
        />
      )}
    </React.Fragment>
  )
}

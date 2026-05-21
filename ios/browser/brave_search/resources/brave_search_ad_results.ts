// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {CrWebApi, gCrWeb} from '//ios/web/public/js_messaging/resources/gcrweb.js';

interface Creative {
  creativeInstanceId?: string;
  placementId?: string;
  creativeSetId?: string;
  campaignId?: string;
  advertiserId?: string;
  landingPage?: string;
  headlineText?: string;
  description?: string;
  rewardsValue?: string;
  conversionUrlPatternValue?: string;
  conversionAdvertiserPublicKeyValue?: string;
  conversionObservationWindowValue?: string;
}

const creativeFieldNamesMapping: Record<string, keyof Creative> = {
  'data-creative-instance-id': 'creativeInstanceId',
  'data-placement-id': 'placementId',
  'data-creative-set-id': 'creativeSetId',
  'data-campaign-id': 'campaignId',
  'data-advertiser-id': 'advertiserId',
  'data-landing-page': 'landingPage',
  'data-headline-text': 'headlineText',
  'data-description': 'description',
  'data-rewards-value': 'rewardsValue',
  'data-conversion-url-pattern-value': 'conversionUrlPatternValue',
  'data-conversion-advertiser-public-key-value':
    'conversionAdvertiserPublicKeyValue',
  'data-conversion-observation-window-value':
    'conversionObservationWindowValue',
};

function getCreatives(): string {
  const creatives: Creative[] = [];
  const scripts =
    document.querySelectorAll('script[type="application/ld+json"]');
  try {
    const jsonLdList =
      Array.from(scripts).map(script => JSON.parse(script.textContent || ''));

    jsonLdList.forEach(jsonLd => {
      if (jsonLd['@type'] === 'Product' && jsonLd.creatives) {
        jsonLd.creatives.forEach((creative: Record<string, string>) => {
          if (creative['@type'] === 'SearchResultAd') {
            const mapped: Creative = {};
            for (const key in creative) {
              const mappedKey = creativeFieldNamesMapping[key];
              if (mappedKey) {
                mapped[mappedKey] = creative[key];
              }
            }
            creatives.push(mapped);
          }
        });
      }
    });
  } catch {
  }
  return JSON.stringify(creatives);
}

const braveSearchAdResultsApi = new CrWebApi('braveSearchAdResults');
braveSearchAdResultsApi.addFunction('getCreatives', getCreatives);
gCrWeb.registerApi(braveSearchAdResultsApi);

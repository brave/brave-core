// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

'use strict';

window.__firefox__.includeOnce('BraveSearchResultAdScript', function($) {
  let sendMessage = $(function(creatives) {
    $.postNativeMessage('$<message_handler>', {
      "securityToken": SECURITY_TOKEN,
      "creatives": creatives
    });
  });

  let getJsonLdCreatives = () => {
    const scripts =
      document.querySelectorAll('script[type="application/ld+json"]');
    const jsonLdList =
      Array.from(scripts).map(script => JSON.parse(script.textContent));

    if (!jsonLdList) {
      return [];
    }

    const creativeFieldNamesMapping = {
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
        'conversionObservationWindowValue'
    };

    let jsonLdCreatives = [];
    jsonLdList.forEach(jsonLd => {
      if (jsonLd['@type'] === 'Product' && jsonLd.creatives) {
        jsonLd.creatives.forEach(creative => {
          if (creative['@type'] === 'SearchResultAd') {
            let jsonLdCreative = {};
            for (let key in creative) {
              if (creativeFieldNamesMapping[key]) {
                jsonLdCreative[creativeFieldNamesMapping[key]] = creative[key];
              }
            }
            jsonLdCreatives.push(jsonLdCreative);
          }
        });
      }
    });

    return jsonLdCreatives;
  };

  sendMessage(getJsonLdCreatives());
});

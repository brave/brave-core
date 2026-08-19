/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.geo;

import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.omnibox.OmniboxFeatureList;
import org.chromium.components.search_engines.TemplateUrlService;

/** Regression tests for GeolocationHeader access after TemplateUrlService native teardown. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
@DisableFeatures({OmniboxFeatureList.PLATFORM_AGNOSTIC_X_GEO})
public class BraveGeolocationHeaderUnitTest {
    private static final String TEST_URL = "https://search.brave.com/search?q=foo";

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfileMock;
    @Mock private TemplateUrlService mTemplateUrlServiceMock;

    @Test
    public void getGeoHeader_withoutNativeService_returnsNullWithoutJni() {
        when(mTemplateUrlServiceMock.hasNativeService()).thenReturn(false);

        assertNull(GeolocationHeader.getGeoHeader(TEST_URL, mProfileMock, mTemplateUrlServiceMock));

        verify(mTemplateUrlServiceMock, never())
                .isSearchResultsPageFromDefaultSearchProvider(any());
        verify(mTemplateUrlServiceMock, never()).isDefaultSearchEngineGoogle();
    }
}

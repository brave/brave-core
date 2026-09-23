/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MojoException;
import org.chromium.url_sanitizer.mojom.UrlSanitizerService;

import java.util.ArrayList;
import java.util.List;

/** Unit tests for {@link UrlSanitizerServiceFactory#sanitizeUrl}. */
@RunWith(BaseRobolectricTestRunner.class)
public class UrlSanitizerServiceFactoryUnitTest {
    private static final String ORIGINAL_URL = "https://example.com/?utm_source=test";
    private static final String SANITIZED_URL = "https://example.com/";

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private UrlSanitizerService.Proxy mService;
    @Mock private Handler mProxyHandler;

    private UrlSanitizerServiceFactory mFactory;

    @Before
    public void setUp() {
        mFactory = spy(UrlSanitizerServiceFactory.getInstance());
    }

    @Test
    public void testSanitizeUrl_serviceUnavailable_postsOriginalUrl() {
        doReturn(null).when(mFactory).getUrlSanitizerAndroidService(eq(mProfile), isNull());
        List<String> results = new ArrayList<>();

        mFactory.sanitizeUrl(mProfile, ORIGINAL_URL, results::add);

        assertEquals(List.of(), results);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        assertEquals(List.of(ORIGINAL_URL), results);
    }

    @Test
    public void testSanitizeUrl_connectionError_returnsOriginalUrl() {
        setService();
        List<String> results = new ArrayList<>();

        mFactory.sanitizeUrl(mProfile, ORIGINAL_URL, results::add);

        ArgumentCaptor<ConnectionErrorHandler> errorHandlerCaptor =
                ArgumentCaptor.forClass(ConnectionErrorHandler.class);
        verify(mProxyHandler).setErrorHandler(errorHandlerCaptor.capture());
        errorHandlerCaptor.getValue().onConnectionError(new MojoException(0));

        assertEquals(List.of(), results);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        assertEquals(List.of(ORIGINAL_URL), results);
        verify(mService).close();
    }

    @Test
    public void testSanitizeUrl_serviceResult_postsSanitizedUrl() {
        setService();
        List<String> results = new ArrayList<>();

        mFactory.sanitizeUrl(mProfile, ORIGINAL_URL, results::add);

        ArgumentCaptor<UrlSanitizerService.SanitizeUrl_Response> responseCaptor =
                ArgumentCaptor.forClass(UrlSanitizerService.SanitizeUrl_Response.class);
        verify(mService).sanitizeUrl(eq(ORIGINAL_URL), responseCaptor.capture());

        responseCaptor.getValue().call(SANITIZED_URL);

        assertEquals(List.of(), results);
        ShadowLooper.runUiThreadTasksIncludingDelayedTasks();
        assertEquals(List.of(SANITIZED_URL), results);
        verify(mService).close();
    }

    private void setService() {
        doReturn(mService).when(mFactory).getUrlSanitizerAndroidService(eq(mProfile), isNull());
        when(mService.getProxyHandler()).thenReturn(mProxyHandler);
    }
}

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// In this package to mock the package-private AddToHomescreenCoordinator.Natives.
package org.chromium.components.webapps;

import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.UnownedUserDataHost;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.browser.banners.AppMenuVerbiage;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.webapps.AppInstallMenuHandler;
import org.chromium.chrome.browser.webapps.WebappRegistry;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetControllerProvider;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.url.GURL;
import org.chromium.url.JUnitTestGURLs;

/** Unit tests for Brave's changes to {@link AppInstallMenuHandler#doUniversalInstall}. */
@RunWith(BaseRobolectricTestRunner.class)
@EnableFeatures(BraveFeatureList.BRAVE_ANDROID_MERGE_INSTALL_PWA_AND_CREATE_SHORTCUT)
public class BraveAppInstallMenuHandlerTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private AddToHomescreenCoordinator.Natives mAddToHomescreenJni;
    @Mock private AppBannerManager.Natives mAppBannerManagerJni;
    @Mock private BottomSheetController mBottomSheetController;
    @Mock private WindowAndroid mWindowAndroid;
    @Mock private ModalDialogManager mModalDialogManager;
    @Mock private Tab mTab;
    @Mock private WebContents mWebContents;

    private Activity mActivity;

    @Before
    public void setUp() {
        AddToHomescreenCoordinatorJni.setInstanceForTesting(mAddToHomescreenJni);
        AppBannerManagerJni.setInstanceForTesting(mAppBannerManagerJni);
        BottomSheetControllerProvider.setInstanceForTesting(mBottomSheetController);
        WebappRegistry.refreshSharedPrefsForTesting();

        mActivity = Robolectric.buildActivity(Activity.class).setup().get();
        when(mWindowAndroid.getUnownedUserDataHost()).thenReturn(new UnownedUserDataHost());
        when(mTab.getWebContents()).thenReturn(mWebContents);
    }

    private void setUrl(GURL url) {
        when(mTab.getUrl()).thenReturn(url);
        when(mWebContents.getVisibleUrl()).thenReturn(url);
    }

    private boolean doUniversalInstall() {
        return AppInstallMenuHandler.doUniversalInstall(
                mActivity, mWindowAndroid, mModalDialogManager, mTab);
    }

    @Test
    public void testSkipsBottomSheetAndAddsToHomescreen() {
        setUrl(JUnitTestGURLs.EXAMPLE_URL);

        assertTrue(doUniversalInstall());

        verify(mAddToHomescreenJni)
                .startForAppMenu(
                        any(),
                        eq(mWebContents),
                        eq(AppMenuVerbiage.APP_MENU_OPTION_ADD_TO_HOMESCREEN));
        verify(mBottomSheetController, never()).requestShowContent(any(), anyBoolean());
    }

    @Test
    public void testChromeSchemeStillInstalls() {
        setUrl(JUnitTestGURLs.CHROME_ABOUT);

        assertTrue(doUniversalInstall());

        verify(mAddToHomescreenJni)
                .startForAppMenu(
                        any(), eq(mWebContents), eq(AppMenuVerbiage.APP_MENU_OPTION_INSTALL));
        verify(mBottomSheetController, never()).requestShowContent(any(), anyBoolean());
    }
}

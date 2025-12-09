/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;

import android.app.Activity;
import android.content.pm.PackageManager;

import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.Shadows;
import org.robolectric.shadows.ShadowPackageManager;

import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.TestActivity;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.display.DisplayAndroid;
import org.chromium.ui.listmenu.ListMenuItemProperties;
import org.chromium.ui.modelutil.MVCListAdapter.ModelList;
import org.chromium.ui.widget.ViewRectProvider;
import org.chromium.url.GURL;

import java.util.function.BooleanSupplier;

/** Unit tests for {@link BraveToolbarLongPressMenuHandler}. */
@RunWith(BaseRobolectricTestRunner.class)
public final class BraveToolbarLongPressMenuHandlerUnitTest {
    @Rule public MockitoRule mockitoRule = MockitoJUnit.rule();

    @Rule
    public ActivityScenarioRule<TestActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Mock private ViewRectProvider mViewRectProvider;
    @Mock Profile mProfile;
    @Mock private WindowAndroid mWindowAndroid;
    @Mock private ActivityLifecycleDispatcher mActivityLifecycleDispatcher;
    @Mock private DisplayAndroid mDisplayAndroid;

    private ToolbarLongPressMenuHandler mToolbarLongPressMenuHandler;
    private ObservableSupplierImpl mProfileSupplier;

    private Activity mActivity;
    private boolean mShouldSuppress;
    private final BooleanSupplier mSuppressSupplier = () -> mShouldSuppress;
    private GURL mUrl;

    @Before
    public void setUp() throws Exception {
        mActivity = Robolectric.buildActivity(Activity.class).get();
        ShadowPackageManager shadowPackageManager = Shadows.shadowOf(mActivity.getPackageManager());
        shadowPackageManager.setSystemFeature(PackageManager.FEATURE_SENSOR_HINGE_ANGLE, false);

        mProfileSupplier = new ObservableSupplierImpl<>();
        mProfileSupplier.set(mProfile);

        doReturn(mDisplayAndroid).when(mWindowAndroid).getDisplay();
        doReturn(1.0f).when(mDisplayAndroid).getDipScale();
        doReturn(true).when(mActivityLifecycleDispatcher).isNativeInitializationFinished();
        mToolbarLongPressMenuHandler =
                new BraveToolbarLongPressMenuHandler(
                        mActivity,
                        mProfileSupplier,
                        false,
                        mSuppressSupplier,
                        mActivityLifecycleDispatcher,
                        mWindowAndroid,
                        () -> mUrl,
                        () -> mViewRectProvider);

        verify(mActivityLifecycleDispatcher).register(mToolbarLongPressMenuHandler);
    }

    @Test
    @SmallTest
    @Restriction({DeviceFormFactor.PHONE})
    public void testbuildMenuItems() {
        ModelList list = mToolbarLongPressMenuHandler.buildMenuItems(true);

        assertEquals(1, list.size());
        assertEquals(
                R.string.toolbar_move_to_the_bottom,
                list.get(0).model.get(ListMenuItemProperties.TITLE_ID));
        assertEquals(
                ToolbarLongPressMenuHandler.MenuItemType.MOVE_ADDRESS_BAR_TO,
                list.get(0).model.get(ListMenuItemProperties.MENU_ITEM_ID));
    }
}

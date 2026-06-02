/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.mock;

import static org.chromium.chrome.browser.hub.HubColorMixer.COLOR_MIXER;
import static org.chromium.chrome.browser.hub.HubToolbarProperties.IS_INCOGNITO;

import android.view.View;
import android.widget.FrameLayout;
import android.widget.Button;
import android.app.Activity;
import android.view.LayoutInflater;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import org.junit.Rule;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoRule;
import org.mockito.junit.MockitoJUnit;

import org.robolectric.ParameterizedRobolectricTestRunner;
import org.robolectric.ParameterizedRobolectricTestRunner.Parameter;
import org.robolectric.ParameterizedRobolectricTestRunner.Parameters;

import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.test.BaseRobolectricTestRule;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.base.BraveFeatureList;
import org.chromium.base.DeviceInfo;
import org.chromium.chrome.R;
import org.chromium.ui.base.TestActivity;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;

import org.chromium.base.Log;
/** Unit tests for {@link BraveHubToolbarView}. */
@RunWith(ParameterizedRobolectricTestRunner.class)
@EnableFeatures({
    BraveFeatureList.BRAVE_SHRED,
})
public class BraveHubToolbarViewUnitTest {

    @Parameters
    public static Collection<Object[]> data() {
        return Arrays.asList(new Object[][] {{true}, {false}});
    }
    
    @Parameter(0)
    public boolean mIsXrDevice;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public ActivityScenarioRule<TestActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(TestActivity.class);

    @Rule public BaseRobolectricTestRule mBaseRule = new BaseRobolectricTestRule();

    @Mock private Pane mPane;

    private SettableMonotonicObservableSupplier<Pane> mFocusedPaneSupplier;
    private FrameLayout mToolbarContainer;
    private Button mShredButton;
    private PropertyModel mPropertyModel;
    private Activity mActivity;
    private HubColorMixer mColorMixer;
    private HubToolbarView mHubToolbarView;

    @Before
    public void setUp() throws Exception {
        DeviceInfo.setIsXrForTesting(mIsXrDevice);

        mActivityScenarioRule.getScenario().onActivity(this::onActivity);
    }

    private void onActivity(TestActivity activity) {
        mPane = mock();
        mActivity = activity;
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);

        LayoutInflater inflater = LayoutInflater.from(mActivity);
        int layoutId = mIsXrDevice ? R.layout.hub_xr_toolbar_layout : R.layout.hub_toolbar_layout;
        mToolbarContainer = (FrameLayout) inflater.inflate(layoutId, null, false);
        mHubToolbarView = mToolbarContainer.findViewById(R.id.hub_toolbar);
        mShredButton = mToolbarContainer.findViewById(org.chromium.chrome.browser.brave_shields.R.id.shred_data_button);
        //     mActionButton = mToolbarContainer.findViewById(R.id.toolbar_action_button);
        // mPaneSwitcher = mToolbarContainer.findViewById(R.id.pane_switcher);
        // mMenuButtonContainer = mToolbarContainer.findViewById(R.id.menu_button_container);
        // mMenuButtonWrapper = mToolbarContainer.findViewById(R.id.menu_button_wrapper);
        // mSearchBox = mToolbarContainer.findViewById(R.id.search_box);
        // mSearchLoupe = mToolbarContainer.findViewById(R.id.search_loupe);
        // mSearchBoxText = mToolbarContainer.findViewById(R.id.search_box_text);
        // mHairline = mToolbarContainer.findViewById(R.id.toolbar_bottom_hairline);
        mActivity.setContentView(mToolbarContainer);

        mFocusedPaneSupplier = ObservableSuppliers.createMonotonic();
        mColorMixer =
                spy(
                        new HubColorMixerImpl(
                                mActivity, ObservableSuppliers.alwaysTrue(), mFocusedPaneSupplier));
        mPropertyModel =
                new PropertyModel.Builder(HubToolbarProperties.ALL_KEYS)
                        .with(COLOR_MIXER, mColorMixer)
                        .build();
        PropertyModelChangeProcessor.create(
                mPropertyModel,
                mHubToolbarView,
                HubToolbarViewBinder::bind);
        when(mPane.getColorScheme()).thenReturn(HubColorScheme.DEFAULT);
        mFocusedPaneSupplier.set(mPane);
        Log.i("SHRED", "[SHRED] setUp: Finished, mShredButton: " + mShredButton);
    }

    @Test
//    @EnableFeatures({BraveFeatureList.BRAVE_SHRED})
    public void testHideManualShredButtonInIncognitoMode() {
        mPropertyModel.set(IS_INCOGNITO, false);

        Log.i("SHRED", "[SHRED] testHideManualShredButtonInIncognitoMode: started shredButton: " + mShredButton);
//        assertTrue(mShredButton != null);
//        assertEquals(View.VISIBLE, mShredButton.getVisibility());
//        Log.i("SHRED", "[SHRED] #100 testHideManualShredButtonInIncognitoMode: visibility:" + mShredButton.getVisibility());
        mPropertyModel.set(IS_INCOGNITO, true);
//        Log.i("SHRED", "[SHRED] #101 testHideManualShredButtonInIncognitoMode: visibility:" + mShredButton.getVisibility());
      //  mHubToolbarView.updateIncognitoElements(true);

        assertEquals(View.INVISIBLE, mShredButton.getVisibility());

        mPropertyModel.set(IS_INCOGNITO, false);
      //  mHubToolbarView.updateIncognitoElements(false);
        assertEquals(View.VISIBLE, mShredButton.getVisibility());
    }
}

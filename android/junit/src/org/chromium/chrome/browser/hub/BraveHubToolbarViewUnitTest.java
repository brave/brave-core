/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import static org.chromium.chrome.browser.hub.HubColorMixer.COLOR_MIXER;
import static org.chromium.chrome.browser.hub.HubToolbarProperties.IS_INCOGNITO;

import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;

import androidx.test.ext.junit.rules.ActivityScenarioRule;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_shields.FirstPartyStorageCleanerInterface;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

/** Unit tests for {@link BraveHubToolbarView}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
@CommandLineFlags.Add({
    ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
    ChromeSwitches.DISABLE_NATIVE_INITIALIZATION
})
public class BraveHubToolbarViewUnitTest {
    @Rule
    public ActivityScenarioRule<ChromeTabbedActivity> mActivityScenarioRule =
            new ActivityScenarioRule<>(ChromeTabbedActivity.class);

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();
    @Mock private Pane mPane;

    private SettableMonotonicObservableSupplier<Pane> mFocusedPaneSupplier;
    private FrameLayout mToolbarContainer;
    private Button mShredButton;
    private PropertyModel mPropertyModel;
    @Spy private BraveActivity mActivity;
    private HubColorMixer mColorMixer;
    private BraveHubToolbarView mHubToolbarView;

    @Before
    public void setUp() throws Exception {
        mActivityScenarioRule.getScenario().onActivity(this::onActivity);
    }

    @After
    public void tearDown() {}

    private void onActivity(ChromeTabbedActivity activity) {
        mPane = mock();
        BraveActivity braveActivity = (BraveActivity) (Object) activity;
        mActivity = spy(braveActivity);
        mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);

        LayoutInflater inflater = LayoutInflater.from(mActivity);
        mToolbarContainer =
                (FrameLayout) inflater.inflate(R.layout.hub_toolbar_layout, null, false);
        mHubToolbarView = (BraveHubToolbarView) mToolbarContainer.findViewById(R.id.hub_toolbar);
        mShredButton =
                mToolbarContainer.findViewById(
                        org.chromium.chrome.browser.brave_shields.R.id.shred_data_button);
        mActivity.setContentView(mToolbarContainer);

        mHubToolbarView.setFirstPartyStorageCleanerForTesting(
                (FirstPartyStorageCleanerInterface) mActivity);

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
                mPropertyModel, mHubToolbarView, HubToolbarViewBinder::bind);
        when(mPane.getColorScheme()).thenReturn(HubColorScheme.DEFAULT);
        mFocusedPaneSupplier.set(mPane);
    }

    @Test
    @EnableFeatures({BraveFeatureList.BRAVE_SHRED})
    public void testHideManualShredButtonInIncognitoMode() {
        when(mActivity.isShredButtonVisible()).thenReturn(true);
        assertEquals(View.VISIBLE, mShredButton.getVisibility());

        when(mActivity.isShredButtonVisible()).thenReturn(false);
        mPropertyModel.set(IS_INCOGNITO, true);
        assertEquals(View.INVISIBLE, mShredButton.getVisibility());

        when(mActivity.isShredButtonVisible()).thenReturn(true);
        mPropertyModel.set(IS_INCOGNITO, false);
        assertEquals(View.VISIBLE, mShredButton.getVisibility());
    }

    @Test
    @DisableFeatures({BraveFeatureList.BRAVE_SHRED})
    public void testHideManualShredButtonIfFeatureFlagDisabled() {
        when(mActivity.isShredButtonVisible()).thenReturn(true);
        assertEquals(View.VISIBLE, mShredButton.getVisibility());

        when(mActivity.isShredButtonVisible()).thenReturn(false);
        mPropertyModel.set(IS_INCOGNITO, true);
        assertEquals(View.INVISIBLE, mShredButton.getVisibility());

        when(mActivity.isShredButtonVisible()).thenReturn(true);
        mPropertyModel.set(IS_INCOGNITO, false);
        assertEquals(View.INVISIBLE, mShredButton.getVisibility());
    }
}

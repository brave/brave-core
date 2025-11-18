/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.homepage.settings;

import static org.mockito.Mockito.when;

import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Lifecycle.State;
import androidx.test.core.app.ActivityScenario;
import androidx.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.homepage.HomepageManager;
import org.chromium.chrome.browser.homepage.HomepagePolicyManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithEditText;
import org.chromium.components.prefs.PrefService;
import org.chromium.content_public.browser.test.util.TouchCommon;
import org.chromium.ui.base.TestActivity;

/** Test for {@link BraveHomepageSettings} to check Brave related UI changes. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(shadows = {ShadowLooper.class})
public class BraveHomepageSettingsUnitTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock public HomepagePolicyManager mMockHomepagePolicyManager;
    @Mock public Profile mProfile;
    @Mock private BookmarkModel mBookmarkModel;

    private ActivityScenario<TestActivity> mActivityScenario;
    private TestActivity mActivity;

    private BraveRadioButtonGroupHomepagePreference mRadioGroupPreference;

    private RadioButtonWithDescription mChromeNtpRadioButton;
    private RadioButtonWithDescription mMobileBookmarksRadioButton;
    private RadioButtonWithEditText mCustomUriRadioButton;

    private static final long TEST_BOOKMARK_ID = 42;
    private static final BookmarkId MOBILE_BOOKMARKS_REASSIGNED_ID =
            new BookmarkId(TEST_BOOKMARK_ID, BookmarkType.NORMAL);

    // We are following example of upstream's {@link HomepageSettingsUnitTest} and use
    // ActivityScenario here. The reason of using ActivityScenario is likely to have an ability to
    // move to different activity states, that I couldn't find for ActivityScenarioRule.
    @SuppressWarnings("ActivityScenarioLaunch")
    @Before
    public void setUp() {
        HomepagePolicyManager.setInstanceForTests(mMockHomepagePolicyManager);
        mActivityScenario = ActivityScenario.launch(TestActivity.class);
        mActivityScenario.onActivity(
                activity -> {
                    mActivity = activity;
                    // Needed for BraveHomepageSettings to inflate correctly.
                    mActivity.setTheme(R.style.Theme_BrowserUI_DayNight);
                });
        ProfileManager.setLastUsedProfileForTesting(mProfile);
        HomepagePolicyManager.setPrefServiceForTesting(Mockito.mock(PrefService.class));

        BookmarkModel.setInstanceForTesting(mBookmarkModel);
        when(mBookmarkModel.getMobileFolderId()).thenReturn(MOBILE_BOOKMARKS_REASSIGNED_ID);
        when(mBookmarkModel.isBookmarkModelLoaded()).thenReturn(true);
    }

    @After
    public void tearDown() {
        mActivityScenario.close();
    }

    private void launchBraveHomepageSettings() {
        FragmentManager fragmentManager = mActivity.getSupportFragmentManager();
        BraveHomepageSettings fragment =
                (BraveHomepageSettings)
                        fragmentManager
                                .getFragmentFactory()
                                .instantiate(
                                        BraveHomepageSettings.class.getClassLoader(),
                                        BraveHomepageSettings.class.getName());
        fragment.setProfile(mProfile);
        fragmentManager.beginTransaction().replace(android.R.id.content, fragment).commit();

        mActivityScenario.moveToState(State.STARTED);
        mRadioGroupPreference =
                (BraveRadioButtonGroupHomepagePreference)
                        fragment.findPreference(HomepageSettings.PREF_HOMEPAGE_RADIO_GROUP);

        Assert.assertTrue(mRadioGroupPreference != null);

        mChromeNtpRadioButton = mRadioGroupPreference.getChromeNtpRadioButton();
        mMobileBookmarksRadioButton = mRadioGroupPreference.getMobileBookmarksRadioButton();
        mCustomUriRadioButton = mRadioGroupPreference.getCustomUriRadioButton();
    }

    private void finishSettingsActivity() {
        mActivityScenario.moveToState(State.DESTROYED);
    }

    @Test
    @SmallTest
    @Feature({"Homepage"})
    public void testMobileBookmarksClick() throws Exception {
        launchBraveHomepageSettings();

        HomepageManager homepageManager = HomepageManager.getInstance();

        // Check initial state.
        Assert.assertTrue(mChromeNtpRadioButton.isChecked());
        Assert.assertFalse(mMobileBookmarksRadioButton.isChecked());
        Assert.assertFalse(mCustomUriRadioButton.isChecked());
        Assert.assertEquals(
                RadioButtonGroupHomepagePreference.HomepageOption.ENTRY_CHROME_NTP,
                mRadioGroupPreference.getPreferenceValue().getCheckedOption());
        Assert.assertEquals("", mRadioGroupPreference.getPreferenceValue().getCustomURI());

        // Click `Mobile bookmarks` radio button.
        checkRadioButtonAndWait(mMobileBookmarksRadioButton);

        // Check results of the click.
        Assert.assertFalse(mChromeNtpRadioButton.isChecked());
        Assert.assertTrue(mMobileBookmarksRadioButton.isChecked());
        Assert.assertFalse(mCustomUriRadioButton.isChecked());
        Assert.assertEquals(
                mRadioGroupPreference.getPreferenceValue().getCheckedOption(),
                RadioButtonGroupHomepagePreference.HomepageOption.ENTRY_CUSTOM_URI);
        Assert.assertEquals(
                mRadioGroupPreference.getPreferenceValue().getCustomURI(),
                BraveRadioButtonGroupHomepagePreference.MOBILE_BOOKMARKS_PATH);

        // End the activity. The homepage should be the `Mobile bookmarks` path.
        finishSettingsActivity();
        Assert.assertEquals(
                "chrome-native://bookmarks/folder/" + TEST_BOOKMARK_ID,
                homepageManager.getHomepageGurl(false).getSpec());
    }

    private void checkRadioButtonAndWait(RadioButtonWithDescription radioButton) {
        TouchCommon.singleClickView(radioButton, 5, 5);
        ShadowLooper.idleMainLooper();
        Assert.assertTrue(radioButton.isChecked());
    }
}

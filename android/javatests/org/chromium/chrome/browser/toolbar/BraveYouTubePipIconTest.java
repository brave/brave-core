/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar;

import static org.mockito.Mockito.when;

import android.view.View;

import androidx.test.filters.MediumTest;

import org.hamcrest.Matchers;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentMatchers;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.test.util.Batch;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Criteria;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.Features.EnableFeatures;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.media.PictureInPicture;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelperJni;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content_public.browser.WebContents;

/**
 * Tests the YouTube PiP toolbar icon, which {@code BraveToolbarLayoutImpl#showYouTubePipIcon}
 * re-evaluates on navigation, page load and tab selection. Those call sites sit on upstream tab
 * observers, so these tests are meant to catch their silent removal during Chromium updates.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@EnableFeatures(BraveFeatureList.BRAVE_PICTURE_IN_PICTURE_FOR_YOUTUBE_VIDEOS)
@Batch(Batch.PER_CLASS)
public class BraveYouTubePipIconTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public final ChromeTabbedActivityTestRule mActivityTestRule =
            new ChromeTabbedActivityTestRule();

    @Mock private BraveYouTubeScriptInjectorNativeHelper.Natives mNativeHelperJniMock;

    @Before
    public void setUp() {
        BraveYouTubeScriptInjectorNativeHelperJni.setInstanceForTesting(mNativeHelperJniMock);
        mActivityTestRule.startMainActivityOnBlankPage();
        // The icon stays hidden when the OS does not allow Picture-in-Picture, which would make
        // every assertion below vacuous.
        Assume.assumeTrue(PictureInPicture.isEnabled(mActivityTestRule.getActivity()));
    }

    @Test
    @MediumTest
    public void testIconShownWhenPictureInPictureIsAvailable() {
        setPictureInPictureAvailable(true);

        mActivityTestRule.loadUrl("about:blank");

        waitForPipIconVisibility(View.VISIBLE);
    }

    @Test
    @MediumTest
    public void testIconHiddenWhenPictureInPictureIsNotAvailable() {
        setPictureInPictureAvailable(false);

        mActivityTestRule.loadUrl("about:blank");

        waitForPipIconVisibility(View.GONE);
    }

    @Test
    @MediumTest
    public void testIconFollowsSelectedTab() {
        setPictureInPictureAvailable(true);
        mActivityTestRule.loadUrl("about:blank");
        waitForPipIconVisibility(View.VISIBLE);

        // A tab switch must re-evaluate the icon for the newly selected tab, rather than leave the
        // previous tab's state on screen.
        setPictureInPictureAvailable(false);
        mActivityTestRule.loadUrlInNewTab("about:blank");

        waitForPipIconVisibility(View.GONE);
    }

    private void setPictureInPictureAvailable(boolean available) {
        when(mNativeHelperJniMock.isPictureInPictureAvailable(
                        ArgumentMatchers.any(WebContents.class)))
                .thenReturn(available);
    }

    private void waitForPipIconVisibility(int expectedVisibility) {
        CriteriaHelper.pollUiThread(
                () -> {
                    View pipLayout =
                            mActivityTestRule
                                    .getActivity()
                                    .findViewById(R.id.brave_youtube_pip_layout);
                    Criteria.checkThat(
                            "PiP icon layout is missing from the toolbar.",
                            pipLayout,
                            Matchers.notNullValue());
                    Criteria.checkThat(
                            "PiP icon visibility did not settle.",
                            pipLayout.getVisibility(),
                            Matchers.is(expectedVisibility));
                });
    }
}

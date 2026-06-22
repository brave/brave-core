/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.download.dialogs;

import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.assertion.ViewAssertions.matches;
import static androidx.test.espresso.matcher.RootMatchers.isDialog;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withText;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.annotation.StringRes;
import androidx.test.filters.MediumTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseActivityTestRule;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.base.test.util.Features.DisableFeatures;
import org.chromium.chrome.browser.download.DirectoryOption;
import org.chromium.chrome.browser.download.DownloadDialogBridge;
import org.chromium.chrome.browser.download.DownloadDialogBridgeJni;
import org.chromium.chrome.browser.download.DownloadDirectoryProvider;
import org.chromium.chrome.browser.download.DownloadLocationDialogType;
import org.chromium.chrome.browser.download.DownloadPromptStatus;
import org.chromium.chrome.browser.download.TestDownloadDirectoryProvider;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.R;
import org.chromium.components.browser_ui.modaldialog.AppModalPresenter;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.components.user_prefs.UserPrefsJni;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.test.util.BlankUiTestActivity;

import java.util.ArrayList;

/**
 * Tests for {@link BraveDownloadLocationDialogCoordinator}, the Brave override that shows the
 * download location dialog even when only a single storage directory is available, provided the
 * user has opted in via settings.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@DoNotBatch(reason = "Shows modal dialogs and overrides global JNI/provider state per test.")
@DisableFeatures(ChromeFeatureList.SMART_SUGGESTION_FOR_LARGE_DOWNLOADS)
public class BraveDownloadLocationDialogTest {
    private static final long TOTAL_BYTES = 1024L;
    private static final String SUGGESTED_PATH = "download.png";
    private static final String PRIMARY_STORAGE_PATH = "/sdcard";
    private static final String SECONDARY_STORAGE_PATH = "/android/Download";
    private static final long ASYNC_TIMEOUT_MS = 10000L;

    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Rule
    public BaseActivityTestRule<BlankUiTestActivity> mActivityTestRule =
            new BaseActivityTestRule<>(BlankUiTestActivity.class);

    @Mock private DownloadLocationDialogController mController;
    @Mock private Profile mProfileMock;
    @Mock private PrefService mPrefService;
    @Mock private UserPrefs.Natives mUserPrefsJniMock;
    @Mock private DownloadDialogBridge.Natives mDownloadDialogBridgeJniMock;

    private AppModalPresenter mAppModalPresenter;
    private ModalDialogManager mModalDialogManager;
    private BraveDownloadLocationDialogCoordinator mDialogCoordinator;

    @Before
    public void setUp() throws Exception {
        UserPrefsJni.setInstanceForTesting(mUserPrefsJniMock);
        when(mUserPrefsJniMock.get(any())).thenReturn(mPrefService);
        DownloadDialogBridgeJni.setInstanceForTesting(mDownloadDialogBridgeJniMock);
        when(mPrefService.getString(Pref.DOWNLOAD_DEFAULT_DIRECTORY))
                .thenReturn(PRIMARY_STORAGE_PATH);

        mActivityTestRule.launchActivity(null);

        mAppModalPresenter = new AppModalPresenter(mActivityTestRule.getActivity());
        mModalDialogManager =
                ThreadUtils.runOnUiThreadBlocking(
                        () ->
                                new ModalDialogManager(
                                        mAppModalPresenter,
                                        ModalDialogManager.ModalDialogType.APP));

        mDialogCoordinator = new BraveDownloadLocationDialogCoordinator();
        mDialogCoordinator.initialize(mController);
    }

    private DirectoryOption buildDirectoryOption(
            @DirectoryOption.DownloadLocationDirectoryType int type, String directoryPath) {
        return new DirectoryOption("Download", directoryPath, 1024000, 1024000, type);
    }

    /** Provide exactly one storage directory, mimicking a device with no SD card. */
    private void setSingleDirectory() {
        ArrayList<DirectoryOption> dirs = new ArrayList<>();
        dirs.add(
                buildDirectoryOption(
                        DirectoryOption.DownloadLocationDirectoryType.DEFAULT,
                        PRIMARY_STORAGE_PATH));
        setDirectories(dirs);
    }

    /** Provide two storage directories, mimicking a device with an SD card. */
    private void setTwoDirectories() {
        ArrayList<DirectoryOption> dirs = new ArrayList<>();
        dirs.add(
                buildDirectoryOption(
                        DirectoryOption.DownloadLocationDirectoryType.DEFAULT,
                        PRIMARY_STORAGE_PATH));
        dirs.add(
                buildDirectoryOption(
                        DirectoryOption.DownloadLocationDirectoryType.ADDITIONAL,
                        SECONDARY_STORAGE_PATH));
        setDirectories(dirs);
    }

    private void setDirectories(ArrayList<DirectoryOption> dirs) {
        ThreadUtils.runOnUiThreadBlocking(
                () ->
                        DownloadDirectoryProvider.getInstance()
                                .setDirectoryProviderForTesting(
                                        new TestDownloadDirectoryProvider(dirs)));
    }

    private void setDownloadPromptStatus(@DownloadPromptStatus int promptStatus) {
        when(mPrefService.getInteger(Pref.PROMPT_FOR_DOWNLOAD_ANDROID)).thenReturn(promptStatus);
    }

    private void showDialog(
            long totalBytes,
            @DownloadLocationDialogType int dialogType,
            String suggestedPath,
            Profile profile) {
        ThreadUtils.runOnUiThreadBlocking(
                () ->
                        mDialogCoordinator.showDialog(
                                mActivityTestRule.getActivity(),
                                mModalDialogManager,
                                totalBytes,
                                dialogType,
                                suggestedPath,
                                profile));
    }

    private void assertTitle(@StringRes int titleId) {
        onView(withText(mActivityTestRule.getActivity().getString(titleId)))
                .inRoot(isDialog())
                .check(matches(isDisplayed()));
    }

    /**
     * Regression test for the NPE crash where, on a single-directory device, the upstream skip
     * called {@code resetDialogState()} (nulling mProfile/mContext) before Brave's asynchronous
     * re-fetch re-entered {@code onDirectoryOptionsRetrieved}. With the user opted in, the dialog
     * must be forced to show without crashing.
     */
    @Test
    @MediumTest
    public void testSingleDirectoryForcesDialogWhenPromptEnabled() throws Exception {
        setSingleDirectory();
        setDownloadPromptStatus(DownloadPromptStatus.SHOW_PREFERENCE);

        showDialog(TOTAL_BYTES, DownloadLocationDialogType.DEFAULT, SUGGESTED_PATH, mProfileMock);

        // The dialog must appear. Before the fix this NPE'd in the async re-trigger and never
        // showed. The wrapper must not complete (skip) the dialog when it is forcing a re-show.
        assertTitle(R.string.download_location_dialog_title);
        verify(mController, never()).onDownloadLocationDialogComplete(any(), anyBoolean());
    }

    /** With more than one directory the dialog shows normally; the override must not interfere. */
    @Test
    @MediumTest
    public void testMultipleDirectoriesShowDialogWhenPromptEnabled() throws Exception {
        setTwoDirectories();
        setDownloadPromptStatus(DownloadPromptStatus.SHOW_PREFERENCE);

        showDialog(TOTAL_BYTES, DownloadLocationDialogType.DEFAULT, SUGGESTED_PATH, mProfileMock);

        assertTitle(R.string.download_location_dialog_title);
        verify(mController, never()).onDownloadLocationDialogComplete(any(), anyBoolean());
    }

    /**
     * When the user has not opted in, the override defers to upstream, which skips the dialog for a
     * single directory and completes directly. Guards against breaking the opt-out path.
     */
    @Test
    @MediumTest
    public void testSingleDirectorySkipsDialogWhenPromptDisabled() throws Exception {
        setSingleDirectory();
        setDownloadPromptStatus(DownloadPromptStatus.DONT_SHOW);

        showDialog(TOTAL_BYTES, DownloadLocationDialogType.DEFAULT, SUGGESTED_PATH, mProfileMock);

        verify(mController, timeout(ASYNC_TIMEOUT_MS))
                .onDownloadLocationDialogComplete(any(), eq(false));
    }
}

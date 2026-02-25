/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.download.dialogs;

import android.content.Context;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.download.DirectoryOption;
import org.chromium.chrome.browser.download.DownloadDialogBridge;
import org.chromium.chrome.browser.download.DownloadDirectoryProvider;
import org.chromium.chrome.browser.download.DownloadLocationDialogType;
import org.chromium.chrome.browser.download.DownloadPromptStatus;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.ArrayList;

/**
 * Brave override of {@link DownloadLocationDialogCoordinator} that always shows the download
 * location dialog when the user has opted in via settings, even when only one storage directory is
 * available (no SD card).
 *
 * <p>Upstream skips the dialog when there's only one storage directory by calling {@code
 * mController.onDownloadLocationDialogComplete(path, false)} directly. We intercept that skip by
 * temporarily wrapping the controller. When the skip fires, our wrapper re-triggers {@code
 * onDirectoryOptionsRetrieved} with a modified directory list ({@code dirs.size() > 1}) so the
 * parent's dialog display code runs normally. The actual directory UI is populated independently by
 * {@link org.chromium.chrome.browser.download.settings.DownloadLocationHelperImpl}.
 */
@NullMarked
public class BraveDownloadLocationDialogCoordinator extends DownloadLocationDialogCoordinator {
    // Shadows the parent's private field. The bytecode adapter deletes this declaration and
    // makes the parent's field protected, so references resolve to the parent's field at runtime.
    // Suppressed because this field is removed by bytecode rewriting and never actually exists.
    @SuppressWarnings("NullAway")
    private DownloadLocationDialogController mController;

    // Stub matching parent's private method. The bytecode adapter deletes this and makes the
    // parent's method public, so calls resolve to the parent's implementation at runtime.
    void onDirectoryOptionsRetrieved(ArrayList<DirectoryOption> dirs) {}

    @Override
    public void showDialog(
            Context context,
            ModalDialogManager modalDialogManager,
            long totalBytes,
            @DownloadLocationDialogType int dialogType,
            String suggestedPath,
            Profile profile) {
        @DownloadPromptStatus
        int promptStatus = DownloadDialogBridge.getPromptForDownloadAndroid(profile);
        boolean userWantsPrompt =
                promptStatus == DownloadPromptStatus.SHOW_PREFERENCE
                        || promptStatus == DownloadPromptStatus.SHOW_INITIAL;

        // If user hasn't opted in, or it's not a DEFAULT dialog type, or it's incognito,
        // defer to upstream behavior (which skips the dialog for single directory).
        if (!userWantsPrompt
                || dialogType != DownloadLocationDialogType.DEFAULT
                || profile.isOffTheRecord()) {
            super.showDialog(
                    context, modalDialogManager, totalBytes, dialogType, suggestedPath, profile);
            return;
        }

        // Transition SHOW_INITIAL → SHOW_PREFERENCE so the "Don't show again" checkbox
        // starts unchecked. With SHOW_INITIAL the checkbox defaults to checked, causing
        // the status to flip to DONT_SHOW after the first dialog — which makes the C++
        // layer (DownloadPrefs::PromptForDownload) skip the dialog entirely on subsequent
        // downloads.
        if (promptStatus == DownloadPromptStatus.SHOW_INITIAL) {
            DownloadDialogBridge.setPromptForDownloadAndroid(
                    profile, DownloadPromptStatus.SHOW_PREFERENCE);
        }

        // Temporarily wrap the controller to intercept the single-directory skip.
        // When the parent's onDirectoryOptionsRetrieved() decides to skip the dialog
        // (dirs.size() == 1), it calls mController.onDownloadLocationDialogComplete(path, false).
        // Our wrapper catches didUserConfirm=false (the skip), restores the original controller,
        // and re-triggers onDirectoryOptionsRetrieved with a modified list so the dialog is shown.
        final DownloadLocationDialogController originalController = mController;
        mController =
                new DownloadLocationDialogController() {
                    @Override
                    public void onDownloadLocationDialogComplete(
                            String path, boolean didUserConfirm) {
                        mController = originalController;
                        if (!didUserConfirm) {
                            // The parent skipped the dialog. Force it to show by
                            // re-fetching directories and ensuring dirs.size() > 1.
                            DownloadDirectoryProvider.getInstance()
                                    .getAllDirectoriesOptions(
                                            (ArrayList<DirectoryOption> dirs) -> {
                                                // Copy to avoid mutating the provider's
                                                // cached list, which would cause the
                                                // directory dropdown to show duplicates.
                                                ArrayList<DirectoryOption> copy =
                                                        new ArrayList<>(dirs);
                                                if (copy.size() == 1) {
                                                    copy.add(copy.get(0));
                                                }
                                                onDirectoryOptionsRetrieved(copy);
                                            });
                        } else {
                            originalController.onDownloadLocationDialogComplete(
                                    path, didUserConfirm);
                        }
                    }

                    @Override
                    public void onDownloadLocationDialogCanceled() {
                        mController = originalController;
                        originalController.onDownloadLocationDialogCanceled();
                    }
                };

        // Let upstream handle ALL showDialog logic (field setup, directory fetch, etc.).
        super.showDialog(
                context, modalDialogManager, totalBytes, dialogType, suggestedPath, profile);
    }
}

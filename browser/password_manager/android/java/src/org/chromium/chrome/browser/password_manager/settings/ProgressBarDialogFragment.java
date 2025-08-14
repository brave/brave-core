// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager.settings;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.View;

import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;

import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.password_manager.R;
import org.chromium.components.browser_ui.widget.MaterialProgressBar;

/**
 * Shows the dialog that informs the user about the progress of preparing passwords for export and
 * allows the user to cancel that operation.
 */
@NullMarked
public class ProgressBarDialogFragment extends DialogFragment {
    // This handler is used to perform the user-triggered cancellation of the password preparation.
    private DialogInterface.OnClickListener mHandler;

    @Initializer
    public void setCancelProgressHandler(DialogInterface.OnClickListener handler) {
        mHandler = handler;
    }

    /**
     * Opens the dialog with the progress bar, hooks up the cancel button handler and sets the
     * progress indicator to being indeterminate, because the background operation does not easily
     * allow to signal its own progress.
     */
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        View dialog =
                getActivity().getLayoutInflater().inflate(R.layout.passwords_progress_dialog, null);
        MaterialProgressBar bar = dialog.findViewById(R.id.passwords_progress_bar);
        bar.setIndeterminate(true);
        return new AlertDialog.Builder(
                        getActivity(), R.style.ThemeOverlay_BrowserUI_AlertDialog_NoActionBar)
                .setView(dialog)
                .setNegativeButton(R.string.cancel, mHandler)
                .setTitle(
                        getActivity()
                                .getResources()
                                .getString(R.string.settings_passwords_preparing_export))
                .create();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // If there is a |savedInstanceState|, then the dialog is being recreated
        // by Android and will lack the necessary click handler. Dismiss
        // immediately, the settings page will recreate it with the appropriate
        // click handler.
        if (savedInstanceState != null) {
            dismiss();
            return;
        }
    }
}

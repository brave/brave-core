/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.utils.confirm_dialog;

/**
 * Interface to handle dialog button click events. Implementing classes can provide custom behavior
 * for positive and negative button clicks.
 */
public interface OnConfirmationDialogListener {
    /** Called when the positive/confirm button is clicked */
    void onPositiveButtonClicked();

    /** Called when the negative/cancel button is clicked */
    void onNegativeButtonClicked();
}

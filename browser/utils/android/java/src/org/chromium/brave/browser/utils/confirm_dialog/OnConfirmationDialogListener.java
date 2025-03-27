/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.utils.confirm_dialog;

/**
 * Interface for handling confirmation dialog button clicks. Implementations can define custom
 * behavior for when the user confirms or cancels.
 */
public interface OnConfirmationDialogListener {
    /**
     * Called when the user clicks the confirm/positive button. Implementations should handle the
     * confirmation action.
     */
    void onPositiveButtonClicked();

    /**
     * Called when the user clicks the cancel/negative button. Implementations should handle the
     * cancellation action.
     */
    void onNegativeButtonClicked();
}

/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media;

import android.app.PictureInPictureParams;

import androidx.annotation.NonNull;

import org.chromium.chrome.browser.init.AsyncInitializationActivity;

public abstract class BravePictureInPictureActivity extends AsyncInitializationActivity {
    @Override
    public void setPictureInPictureParams(@NonNull PictureInPictureParams params) {
        try {
            super.setPictureInPictureParams(params);
        } catch (IllegalStateException ignored) {
            // TODO(sergz): It looks like the call has been made when
            // the Java environment or the application is not in an
            // appropriate state for the requested operation.
            // We can just ignore it.
        } catch (IllegalArgumentException ignored) {
        }
    }
}

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.ui.base;

import static android.content.ClipDescription.MIMETYPE_TEXT_PLAIN;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.os.Build;

import androidx.annotation.StringRes;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.ContextUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.build.annotations.NullMarked;
import org.chromium.ui.widget.Toast;

@NullMarked
public class BraveClipboardHelper {
    private static final int CLEAR_CLIPBOARD_INTERVAL = 60000; // In milliseconds

    /**
     * Saves a given text to clipboard, shows a toast and clears it again after 60 seconds.
     *
     * @param context Context used to retrieve the clipboard service.
     * @param textToCopy Text that will be copied to clipboard.
     * @param textToShow String resource ID to display in the toast, or -1 to disable the toast.
     * @param treatAsPasword {@code true} copy to the clipboard with
     *     ClipDescription.EXTRA_IS_SENSITIVE flag and then clear the clipboard after {@link
     *     #CLEAR_CLIPBOARD_INTERVAL}.
     */
    public static void saveTextToClipboard(
            final Context context,
            final String textToCopy,
            @StringRes final int textToShow,
            final boolean treatAsPasword) {
        if (treatAsPasword) {
            Clipboard.getInstance().setPassword(textToCopy);
        } else {
            Clipboard.getInstance().setText("" /* label */, textToCopy, false);
        }

        // Similar to ClipboardImpl.showToastIfNeeded
        // Conditionally show a toast to avoid duplicate notifications in Android 13+
        if (textToShow != -1 && Build.VERSION.SDK_INT <= Build.VERSION_CODES.S_V2) {
            Toast.makeText(context, textToShow, Toast.LENGTH_SHORT).show();
        }
        if (!treatAsPasword) {
            return;
        }

        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT, () -> clearClipboard(textToCopy), CLEAR_CLIPBOARD_INTERVAL);
    }

    /**
     * Clears the clipboard and replaces it with "***" if it matches a given text.
     *
     * @param textToCompare Text to compare that will trigger the clipboard clearing in case of a
     *     match.
     */
    public static void clearClipboard(final String textToCompare) {
        String clipboardText = getTextFromClipboard(ContextUtils.getApplicationContext());
        if (textToCompare.equals(clipboardText)) {
            BraveReflectionUtil.invokeMethod(Clipboard.class, Clipboard.getInstance(), "clear");
        }
    }

    public static String getTextFromClipboard(Context context) {
        ClipboardManager clipboard =
                (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clipData = clipboard.getPrimaryClip();
        if (clipData != null
                && clipData.getDescription().hasMimeType(MIMETYPE_TEXT_PLAIN)
                && clipData.getItemCount() > 0) {
            return clipData.getItemAt(0).getText().toString();
        }

        return "";
    }
}

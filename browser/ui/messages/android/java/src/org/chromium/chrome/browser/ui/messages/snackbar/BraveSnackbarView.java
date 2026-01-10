/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.messages.snackbar;

import android.app.Activity;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.StyleSpan;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.ui.base.WindowAndroid;

/** Brave's extension of SnackbarView. */
@NullMarked
public class BraveSnackbarView extends SnackbarView {
    private static final String TAG = "BraveSnackbarView";
    // Estimated space for favicon and container padding when TextView width is not yet available.
    // This is just a fallback - the actual width will be re-measured once the TextView is laid out.
    private static final int ESTIMATED_FAVICON_AND_PADDING_SPACE = 200;
    // Default fallback width when neither TextView nor container width is available.
    // This is just a fallback - the actual width will be re-measured once the TextView is laid out.
    private static final int DEFAULT_FALLBACK_WIDTH = 300;
    // Minimum width safety bound to ensure text truncation calculations don't fail with negative or
    // too small values.
    // Unlike the fallback values above, this is a permanent minimum bound, not a temporary
    // estimate.
    private static final int MINIMUM_AVAILABLE_WIDTH = 100;

    // Will be deleted in bytecode. Variable from the parent class will be used instead.
    @SuppressWarnings({"UnusedVariable", "HidingField"})
    protected @Nullable ViewGroup mContainerView;

    public BraveSnackbarView(
            Activity activity,
            SnackbarManager manager,
            Snackbar snackbar,
            ViewGroup parentView,
            @Nullable WindowAndroid windowAndroid,
            @Nullable EdgeToEdgeController edgeToEdgeSupplier,
            boolean isTablet) {
        super(activity, manager, snackbar, parentView, windowAndroid, edgeToEdgeSupplier, isTablet);
    }

    /**
     * Makes the entire snackbar clickable by setting an OnClickListener on the container view.
     *
     * @param clickCallback Callback to execute when the snackbar is clicked.
     */
    public void makeClickable(@Nullable Runnable clickCallback) {
        if (mContainerView == null) {
            Log.e(TAG, "makeClickable: mContainerView is null, cannot make snackbar clickable");
            return;
        }

        if (clickCallback == null) {
            Log.e(TAG, "makeClickable: clickCallback is null");
            return;
        }

        mContainerView.setOnClickListener(
                v -> {
                    clickCallback.run();
                });
        mContainerView.setClickable(true);
    }

    /**
     * Customizes the snackbar text to display title, page title, and URL in a formatted way. The
     * title appears on top (not bold), followed by page title (bold) and URL below.
     *
     * @param title The title text (e.g., "Get back to your most recent tab")
     * @param pageTitle The page title (displayed in bold)
     * @param url The URL to display
     */
    public void setCustomText(String title, String pageTitle, String url) {
        if (mContainerView == null) {
            Log.e(TAG, "setCustomText: mContainerView is null");
            return;
        }

        // Find the TextView that displays the message
        TextView messageTextView = findMessageTextView(mContainerView);
        if (messageTextView == null) {
            Log.e(TAG, "setCustomText: Could not find message TextView");
            return;
        }

        // Enable multi-line display (3 lines: title, page title, URL)
        messageTextView.setMaxLines(3);
        messageTextView.setSingleLine(false);

        // Measure available width for truncation
        // Get the TextView's width, accounting for padding and favicon space
        int availableWidth = messageTextView.getWidth();
        if (availableWidth <= 0) {
            // If TextView hasn't been laid out yet, use container width minus estimated space
            int containerWidth = mContainerView.getWidth();
            if (containerWidth > 0) {
                availableWidth = containerWidth - ESTIMATED_FAVICON_AND_PADDING_SPACE;
            } else {
                // Fallback: use a reasonable default width
                availableWidth = DEFAULT_FALLBACK_WIDTH;
            }
        }

        // Build and set formatted text
        buildFormattedText(messageTextView, title, pageTitle, url, availableWidth);

        // If TextView wasn't measured yet, update text after layout
        if (messageTextView.getWidth() <= 0) {
            messageTextView.post(
                    () -> {
                        // Re-measure and update text with correct truncation
                        int measuredWidth = messageTextView.getWidth();
                        if (measuredWidth > 0) {
                            buildFormattedText(
                                    messageTextView, title, pageTitle, url, measuredWidth);
                        }
                    });
        }
    }

    /**
     * Builds formatted text with title, page title, and URL, truncating as needed, and sets it on
     * the TextView.
     *
     * @param textView The TextView to set text on and measure text width
     * @param title The title text (e.g., "Get back to your most recent tab")
     * @param pageTitle The page title
     * @param url The URL to display
     * @param width The TextView width (will be adjusted for padding)
     */
    private void buildFormattedText(
            TextView textView, String title, String pageTitle, String url, int width) {
        // Calculate available width accounting for padding
        int padding = textView.getPaddingLeft() + textView.getPaddingRight();
        int availableWidth = Math.max(width - padding, MINIMUM_AVAILABLE_WIDTH);

        SpannableStringBuilder builder = new SpannableStringBuilder();

        // Add title on first line (not bold)
        if (!title.isEmpty()) {
            builder.append(title);
        }

        // Add page title on new line in bold (truncated if needed)
        if (!pageTitle.isEmpty()) {
            builder.append("\n");
            String truncatedPageTitle = truncateText(textView, pageTitle, availableWidth, true);
            SpannableString pageTitleSpan = new SpannableString(truncatedPageTitle);
            pageTitleSpan.setSpan(
                    new StyleSpan(Typeface.BOLD),
                    0,
                    truncatedPageTitle.length(),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            builder.append(pageTitleSpan);
        }

        // Add URL on new line (truncated if needed)
        if (!url.isEmpty()) {
            builder.append("\n");
            String truncatedUrl = truncateText(textView, url, availableWidth, false);
            builder.append(truncatedUrl);
        }

        textView.setText(builder);
    }

    /**
     * Truncates text to fit within the available width, adding ellipsis if needed.
     *
     * @param textView The TextView to measure text width
     * @param text The text to truncate
     * @param maxWidth The maximum width in pixels
     * @param isBold Whether the text will be displayed in bold (affects width measurement)
     * @return The truncated text with ellipsis if needed
     */
    private String truncateText(TextView textView, String text, int maxWidth, boolean isBold) {
        if (text == null || text.isEmpty() || maxWidth <= 0) {
            return text != null ? text : "";
        }

        // Create a separate Paint object for measurement to avoid modifying the TextView's Paint
        Paint paint = new Paint(textView.getPaint());
        if (isBold) {
            Typeface originalTypeface = paint.getTypeface();
            Typeface boldTypeface = Typeface.create(originalTypeface, Typeface.BOLD);
            paint.setTypeface(boldTypeface);
        }

        float textWidth = paint.measureText(text);

        // If text fits, return as is
        if (textWidth <= maxWidth) {
            return text;
        }

        // Calculate how many characters fit
        int low = 0;
        int high = text.length();
        String ellipsis = "...";

        while (low < high) {
            int mid = (low + high + 1) / 2;
            String testText = text.substring(0, mid) + ellipsis;
            float testWidth = paint.measureText(testText);

            if (testWidth <= maxWidth) {
                low = mid;
            } else {
                high = mid - 1;
            }
        }

        // Ensure we have at least 1 character before ellipsis
        if (low <= 0) {
            return ellipsis;
        }

        return text.substring(0, low) + ellipsis;
    }

    /**
     * Recursively finds the TextView that displays the snackbar message.
     *
     * @param viewGroup The view group to search in
     * @return The TextView if found, null otherwise
     */
    private @Nullable TextView findMessageTextView(ViewGroup viewGroup) {
        for (int i = 0; i < viewGroup.getChildCount(); i++) {
            View child = viewGroup.getChildAt(i);
            if (child instanceof TextView) {
                TextView textView = (TextView) child;
                // Check if this TextView has text (likely the message TextView)
                if (textView.getText() != null && textView.getText().length() > 0) {
                    return textView;
                }
            } else if (child instanceof ViewGroup) {
                TextView found = findMessageTextView((ViewGroup) child);
                if (found != null) {
                    return found;
                }
            }
        }
        return null;
    }
}

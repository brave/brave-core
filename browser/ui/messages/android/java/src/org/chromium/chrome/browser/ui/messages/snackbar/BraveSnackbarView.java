/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.messages.snackbar;

import android.app.Activity;
import android.content.Context;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.StyleSpan;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.base.Log;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.ui.messages.R;
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

    // Flag to track if we've already restructured the view hierarchy
    private boolean mViewRestructured;
    // Reference to the title TextView we add programmatically
    private @Nullable TextView mTitleTextView;
    // The end-aligned row holding the action button (and optional close button) in the
    // action-below-message layout. Non-null only while that layout is applied; tracked so it can be
    // reverted when this (reused) SnackbarView is updated for a different snackbar.
    private @Nullable LinearLayout mActionRow;
    // The action-below-message layout follows the snackbar that requested it, not this (reused)
    // view: it is re-applied when that snackbar is shown and reverted for any other snackbar.
    private @Nullable Snackbar mActionBelowSnackbar;
    private int mActionBelowCloseIconResId;
    private @Nullable String mActionBelowCloseContentDescription;
    private @Nullable Runnable mActionBelowCloseCallback;

    public BraveSnackbarView(
            Activity activity,
            SnackbarManager manager,
            Snackbar snackbar,
            ViewGroup parentView,
            @Nullable WindowAndroid windowAndroid,
            NonNullObservableSupplier<Integer> additionalBottomMarginPxSupplier) {
        super(
                activity,
                manager,
                snackbar,
                parentView,
                windowAndroid,
                additionalBottomMarginPxSupplier);
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
     * Switches the snackbar to a two-line layout where the action button drops onto its own line
     * below the message, aligned to the end (right in LTR). This matches the Material "longer
     * action" snackbar and is used for notices whose action label is too long to sit beside a
     * multi-line message. When {@code onCloseCallback} is non-null, a trailing close button is
     * added to the right of the action button.
     *
     * <pre>
     * ┌─────────────────────────────────────────────┐
     * │ Message line one                            │
     * │ Message line two                            │
     * │                          [Action]  [×]      │ ← action + close, end-aligned
     * └─────────────────────────────────────────────┘
     * </pre>
     *
     * @param closeIconResId Drawable resource for the close button (resolved in the caller's
     *     resource package). Ignored when {@code onCloseCallback} is null.
     * @param closeContentDescription Accessibility label for the close button, or null.
     * @param onCloseCallback Invoked when the close button is tapped; when null no close button is
     *     added.
     */
    public void setActionBelowMessage(
            int closeIconResId,
            @Nullable String closeContentDescription,
            @Nullable Runnable onCloseCallback) {
        // Remember the request so it can follow this snackbar across view reuse (see update()).
        mActionBelowSnackbar = mSnackbar;
        mActionBelowCloseIconResId = closeIconResId;
        mActionBelowCloseContentDescription = closeContentDescription;
        mActionBelowCloseCallback = onCloseCallback;
        applyActionBelowMessage();
    }

    /**
     * The {@link SnackbarView} is reused across snackbars (the manager updates it in place rather
     * than recreating it). After the base class repopulates the view, re-apply the
     * action-below-message layout if this is the snackbar that requested it, otherwise revert it so
     * the stacked layout and close button don't leak into an unrelated snackbar.
     */
    @Override
    boolean update(Snackbar snackbar) {
        boolean result = super.update(snackbar);
        if (snackbar == mActionBelowSnackbar) {
            applyActionBelowMessage();
        } else {
            resetActionBelowMessage();
        }
        return result;
    }

    private @Nullable LinearLayout getSnackbarLayout() {
        if (mContainerView == null) {
            return null;
        }
        View snackbarView = mContainerView.findViewById(R.id.snackbar);
        return snackbarView instanceof LinearLayout ? (LinearLayout) snackbarView : null;
    }

    private void applyActionBelowMessage() {
        LinearLayout snackbarLayout = getSnackbarLayout();
        if (snackbarLayout == null) {
            Log.e(TAG, "applyActionBelowMessage: snackbar layout not found");
            return;
        }
        Context context = snackbarLayout.getContext();

        // Start from a clean state in case this layout was already applied to this reused view.
        resetActionBelowMessage();

        // Stack the message and action vertically instead of side by side.
        snackbarLayout.setOrientation(LinearLayout.VERTICAL);

        // Let the message span the full width (it uses width=0/weight=1 for the horizontal layout).
        View messageView = snackbarLayout.findViewById(R.id.snackbar_message);
        if (messageView != null
                && messageView.getLayoutParams() instanceof LinearLayout.LayoutParams) {
            LinearLayout.LayoutParams lp =
                    (LinearLayout.LayoutParams) messageView.getLayoutParams();
            lp.width = LinearLayout.LayoutParams.MATCH_PARENT;
            lp.weight = 0;
            // updateInternal() zeroes the end margin when there's an action button (it normally
            // sits to the right). In this stacked layout the message spans the full width, so
            // restore a symmetric end margin to match the start margin.
            lp.setMarginEnd(
                    snackbarLayout
                            .getResources()
                            .getDimensionPixelSize(R.dimen.snackbar_text_view_margin));
            messageView.setLayoutParams(lp);
        }

        View buttonView = snackbarLayout.findViewById(R.id.snackbar_button);
        if (buttonView == null) {
            Log.e(TAG, "applyActionBelowMessage: action button not found");
            return;
        }

        // Build a trailing, end-aligned row that holds the action button (and an optional close
        // button to its right) on its own line below the message.
        LinearLayout actionRow = new LinearLayout(context);
        actionRow.setOrientation(LinearLayout.HORIZONTAL);
        actionRow.setGravity(android.view.Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams rowParams =
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.WRAP_CONTENT,
                        LinearLayout.LayoutParams.WRAP_CONTENT);
        rowParams.gravity = android.view.Gravity.END;
        actionRow.setLayoutParams(rowParams);

        // Move the existing action button into the row.
        snackbarLayout.removeView(buttonView);
        buttonView.setLayoutParams(
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.WRAP_CONTENT,
                        LinearLayout.LayoutParams.WRAP_CONTENT));
        actionRow.addView(buttonView);

        if (mActionBelowCloseCallback != null && mActionBelowCloseIconResId != 0) {
            final Runnable closeCallback = mActionBelowCloseCallback;
            ImageButton closeButton = new ImageButton(context);
            closeButton.setImageResource(mActionBelowCloseIconResId);
            // Borderless, icon-only button.
            closeButton.setBackground(null);
            if (mActionBelowCloseContentDescription != null) {
                closeButton.setContentDescription(mActionBelowCloseContentDescription);
            }
            // Tint the icon to match the action button text so the two read as a pair.
            if (buttonView instanceof TextView) {
                closeButton.setColorFilter(((TextView) buttonView).getCurrentTextColor());
            }
            // 14dp padding around the 24dp icon yields a ~52dp (>=48dp) touch target.
            int padding =
                    snackbarLayout
                            .getResources()
                            .getDimensionPixelSize(R.dimen.snackbar_message_margin);
            closeButton.setPadding(padding, padding, padding, padding);
            closeButton.setOnClickListener(v -> closeCallback.run());
            actionRow.addView(closeButton);
        }

        snackbarLayout.addView(actionRow);
        mActionRow = actionRow;
    }

    /** Undoes {@link #applyActionBelowMessage()}, restoring the stock horizontal layout. */
    private void resetActionBelowMessage() {
        if (mActionRow == null) {
            return;
        }
        LinearLayout snackbarLayout = getSnackbarLayout();
        if (snackbarLayout == null) {
            mActionRow = null;
            return;
        }

        // Pull the action button back out of our row so it becomes a direct child again.
        View buttonView = mActionRow.findViewById(R.id.snackbar_button);
        mActionRow.removeAllViews();
        snackbarLayout.removeView(mActionRow);
        mActionRow = null;

        snackbarLayout.setOrientation(LinearLayout.HORIZONTAL);

        if (buttonView != null) {
            LinearLayout.LayoutParams buttonLp =
                    new LinearLayout.LayoutParams(
                            LinearLayout.LayoutParams.WRAP_CONTENT,
                            LinearLayout.LayoutParams.WRAP_CONTENT);
            buttonLp.gravity = android.view.Gravity.CENTER_VERTICAL;
            buttonView.setLayoutParams(buttonLp);
            snackbarLayout.addView(buttonView);
        }

        // Restore the message's original width/weight (matching floating_snackbar.xml); its margins
        // are reset by the base class's updateInternal().
        View messageView = snackbarLayout.findViewById(R.id.snackbar_message);
        if (messageView != null
                && messageView.getLayoutParams() instanceof LinearLayout.LayoutParams) {
            LinearLayout.LayoutParams lp =
                    (LinearLayout.LayoutParams) messageView.getLayoutParams();
            lp.width = 0;
            lp.weight = 1;
            messageView.setLayoutParams(lp);
        }
    }

    /**
     * Customizes the snackbar text to display title, page title, and URL in a formatted way. The
     * title appears on top of the entire snackbar (above favicon), followed by page title (bold)
     * and URL next to the favicon.
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

        // Restructure the view hierarchy to add title on top (only once)
        if (!mViewRestructured) {
            restructureViewHierarchy();
        }

        // Set the title text
        if (mTitleTextView != null && !title.isEmpty()) {
            mTitleTextView.setText(title);
            mTitleTextView.setVisibility(View.VISIBLE);
        }

        // Find the TextView that displays the message
        TextView messageTextView = findMessageTextView(mContainerView);
        if (messageTextView == null) {
            Log.e(TAG, "setCustomText: Could not find message TextView");
            return;
        }

        // Enable multi-line display (2 lines: page title, URL)
        messageTextView.setMaxLines(2);
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
        buildFormattedText(messageTextView, pageTitle, url, availableWidth);

        // If TextView wasn't measured yet, update text after layout
        if (messageTextView.getWidth() <= 0) {
            messageTextView.post(
                    () -> {
                        // Re-measure and update text with correct truncation
                        int measuredWidth = messageTextView.getWidth();
                        if (measuredWidth > 0) {
                            buildFormattedText(messageTextView, pageTitle, url, measuredWidth);
                        }
                    });
        }
    }

    /**
     * Restructures the snackbar view hierarchy to add a title TextView on top of the content. The
     * original horizontal layout (favicon + message + button) becomes the second row.
     *
     * <pre>
     * ┌─────────────────────────────────────────────┐
     * │ Get back to your most recent tab            │ ← Title (full width, on top)
     * ├─────────────────────────────────────────────┤
     * │ [Favicon] │ Page Title (bold)               │ ← Content row
     * │           │ [URL] (truncated if needed)     │
     * └─────────────────────────────────────────────┘
     * </pre>
     */
    private void restructureViewHierarchy() {
        if (mContainerView == null) {
            return;
        }

        // Find the snackbar LinearLayout (R.id.snackbar)
        View snackbarView = mContainerView.findViewById(R.id.snackbar);
        if (!(snackbarView instanceof LinearLayout)) {
            Log.e(TAG, "restructureViewHierarchy: snackbar view is not a LinearLayout");
            return;
        }

        LinearLayout snackbarLayout = (LinearLayout) snackbarView;

        // Save the original children
        int childCount = snackbarLayout.getChildCount();
        View[] originalChildren = new View[childCount];
        for (int i = 0; i < childCount; i++) {
            originalChildren[i] = snackbarLayout.getChildAt(i);
        }

        // Remove all children from the snackbar layout
        snackbarLayout.removeAllViews();

        // Change orientation to vertical
        snackbarLayout.setOrientation(LinearLayout.VERTICAL);

        // Create title TextView
        mTitleTextView = new TextView(snackbarLayout.getContext());
        mTitleTextView.setTextAppearance(R.style.TextAppearance_TextMedium_Primary);
        // Use same left padding as favicon margin to align the title with favicon
        int titlePaddingLeft =
                snackbarLayout
                        .getResources()
                        .getDimensionPixelSize(R.dimen.snackbar_profile_image_margin_start);
        int titlePaddingTop =
                snackbarLayout
                        .getResources()
                        .getDimensionPixelSize(R.dimen.snackbar_message_margin);
        // No bottom padding - content row already has its own top margin
        mTitleTextView.setPadding(titlePaddingLeft, titlePaddingTop, titlePaddingLeft, 0);
        mTitleTextView.setVisibility(View.GONE); // Will be shown when text is set

        // Add title as first child
        snackbarLayout.addView(mTitleTextView);

        // Create a horizontal LinearLayout for the original content (favicon + message + button)
        LinearLayout contentRow = new LinearLayout(snackbarLayout.getContext());
        contentRow.setOrientation(LinearLayout.HORIZONTAL);
        contentRow.setGravity(android.view.Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams contentRowParams =
                new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,
                        LinearLayout.LayoutParams.WRAP_CONTENT);
        // Negative top margin to reduce gap between title and content row
        int messageMargin =
                snackbarLayout
                        .getResources()
                        .getDimensionPixelSize(R.dimen.snackbar_message_margin);
        contentRowParams.topMargin = -messageMargin * 3 / 4;
        contentRow.setLayoutParams(contentRowParams);

        // Add original children to the content row (preserving their layout params)
        for (View child : originalChildren) {
            contentRow.addView(child);
        }

        // Add content row as second child
        snackbarLayout.addView(contentRow);

        mViewRestructured = true;
    }

    /**
     * Builds formatted text with page title and URL, truncating as needed, and sets it on the
     * TextView.
     *
     * @param textView The TextView to set text on and measure text width
     * @param pageTitle The page title (displayed in bold)
     * @param url The URL to display
     * @param width The TextView width (will be adjusted for padding)
     */
    private void buildFormattedText(TextView textView, String pageTitle, String url, int width) {
        // Calculate available width accounting for padding
        int padding = textView.getPaddingLeft() + textView.getPaddingRight();
        int availableWidth = Math.max(width - padding, MINIMUM_AVAILABLE_WIDTH);

        SpannableStringBuilder builder = new SpannableStringBuilder();

        // Add page title in bold (truncated if needed)
        if (!pageTitle.isEmpty()) {
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
            if (builder.length() > 0) {
                builder.append("\n");
            }
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
                // Skip our programmatically added title TextView
                if (textView == mTitleTextView) {
                    continue;
                }
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

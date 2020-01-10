/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import static org.chromium.chrome.browser.util.ViewUtils.dpToPx;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.StrictMode;
import android.os.SystemClock;
import android.support.annotation.Nullable;
import android.text.format.DateFormat;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.RemoteViews;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Log;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.channels.BraveChannelDefinitions;
import org.chromium.ui.base.LocalizationUtils;

import java.util.Date;

/**
 * Builds a notification according to BraveAds spec.
 */
public class BraveAdsNotificationBuilder extends NotificationBuilderBase {
    /**
     * The maximum width of action icons in dp units.
     */
    private static final int MAX_ACTION_ICON_WIDTH_DP = 32;

    /**
     * The maximum number of lines of body text for the expanded state. Fewer lines are used when
     * the text is scaled up, with a minimum of one line.
     */
    private static final int MAX_BODY_LINES = 7;

    /**
     * The fontScale considered large for the purposes of layout.
     */
    private static final float FONT_SCALE_LARGE = 1.3f;

    /**
     * The maximum amount of padding (in dip units) that is applied around views that must have a
     * flexible amount of padding. If the font size is scaled up the applied padding will be scaled
     * down towards 0.
     */
    private static final int MAX_SCALABLE_PADDING_DP = 3;

    /**
     * The amount of padding at the start of the button, either before an icon or before the text.
     */
    private static final int BUTTON_PADDING_START_DP = 8;

    /**
     * The amount of padding between the icon and text of a button. Used only if there is an icon.
     */
    private static final int BUTTON_ICON_PADDING_DP = 8;

    /**
     * The size of the work profile badge (width and height).
     */
    private static final int WORK_PROFILE_BADGE_SIZE_DP = 16;

    /**
     * Material Grey 600 - to be applied to action button icons in the Material theme.
     */
    private static final int BUTTON_ICON_COLOR_MATERIAL = 0xff757575;

    private final Context mContext;

    public BraveAdsNotificationBuilder(Context context) {
        super(context.getResources());
        mContext = context;
    }

    /**
     * Override this function to always set the channel ID as Ads channel ID.
     *
     * This avoids the channel ID being overwritten by Chromium later using
     * notificationBuilder.setChannelId.
     */
    @Override
    public NotificationBuilderBase setChannelId(String channelId) {
        return super.setChannelId(BraveChannelDefinitions.ChannelId.BRAVE_ADS);
    }

    @Override
    public ChromeNotification build(NotificationMetadata metadata) {
        // A note about RemoteViews and updating notifications. When a notification is passed to the
        // {@code NotificationManager} with the same tag and id as a previous notification, an
        // in-place update will be performed. In that case, the actions of all new
        // {@link RemoteViews} will be applied to the views of the old notification. This is safe
        // for actions that overwrite old values such as setting the text of a {@code TextView}, but
        // care must be taken for additive actions. Especially in the case of
        // {@link RemoteViews#addView} the result could be to append new views below stale ones. In
        // that case {@link RemoteViews#removeAllViews} must be called before adding new ones.
        RemoteViews compactView =
                new RemoteViews(mContext.getPackageName(), R.layout.web_notification_brave_ads);
        RemoteViews bigView =
                new RemoteViews(mContext.getPackageName(), R.layout.web_notification_big_brave_ads);

        float fontScale = mContext.getResources().getConfiguration().fontScale;
        bigView.setInt(R.id.body, "setMaxLines", calculateMaxBodyLines(fontScale));
        int scaledPadding =
                calculateScaledPadding(fontScale, mContext.getResources().getDisplayMetrics());
        setChannelId(BraveChannelDefinitions.ChannelId.BRAVE_ADS);

        for (RemoteViews view : new RemoteViews[] {compactView, bigView}) {
            view.setTextViewText(R.id.title, mTitle);
            view.setTextViewText(R.id.body, mBody);
            view.setImageViewBitmap(R.id.icon, getBraveIcon());
            view.setViewPadding(R.id.title, 0, scaledPadding, 0, 0);
            view.setViewPadding(R.id.body_container, 0, scaledPadding, 0, scaledPadding);
            // addWorkProfileBadge(view);

            int smallIconId = useMaterial() ? R.id.small_icon_overlay : R.id.small_icon_footer;
            view.setViewVisibility(smallIconId, View.VISIBLE);
            if (mSmallIconBitmapForContent != null) {
                view.setImageViewBitmap(smallIconId, mSmallIconBitmapForContent);
            } else {
                view.setImageViewResource(smallIconId, mSmallIconId);
            }
        }

        // Note: under the hood this is not a NotificationCompat builder so be mindful of the
        // API level of methods you call on the builder.
        // TODO(crbug.com/697104) We should probably use a Compat builder.
        ChromeNotificationBuilder builder =
                NotificationBuilderFactory.createChromeNotificationBuilder(false /* preferCompat */,
                        mChannelId, mRemotePackageForBuilderContext, metadata);
        builder.setTicker(mTickerText);
        builder.setContentIntent(mContentIntent);
        builder.setDeleteIntent(mDeleteIntent);
        builder.setPriorityBeforeO(Notification.PRIORITY_HIGH);
        builder.setDefaults(mDefaults);
        if (mVibratePattern != null) builder.setVibrate(mVibratePattern);
        builder.setWhen(mTimestamp);
        builder.setShowWhen(true);
        builder.setOnlyAlertOnce(!mRenotify);
        builder.setContent(compactView);
        builder.setAutoCancel(true);

        // Some things are duplicated in the builder to ensure the notification shows correctly on
        // Wear devices and custom lock screens.
        builder.setContentTitle(mTitle);
        builder.setContentText(mBody);
        builder.setLargeIcon(getBraveIcon());
        setStatusBarIcon(builder, mSmallIconId, mSmallIconBitmapForStatusBar);
        // TODO: Check to see if this is what we want
        setGroupOnBuilder(builder, mOrigin);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            // Public versions only supported since L, and createPublicNotification requires L+.
            builder.setPublicVersion(createPublicNotification(mContext));
        }

        return builder.buildWithBigContentView(bigView);
    }

    private Bitmap getBraveIcon() {
        int largeIconId = R.drawable.btn_brave;
        return BitmapFactory.decodeResource(mContext.getResources(), largeIconId);
    }

    @Override
    public NotificationBuilderBase addButtonAction(@Nullable Bitmap iconBitmap,
            @Nullable CharSequence title, @Nullable PendingIntent intent) {
        return this;
    }

    /**
     * Adds an action to the notification, displayed as a button adjacent to the notification
     * content, which when tapped will trigger a remote input. This enables Android Wear input and,
     * from Android N, displays a text box within the notification for inline replies.
     */
    @Override
    public NotificationBuilderBase addTextAction(@Nullable Bitmap iconBitmap,
            @Nullable CharSequence title, @Nullable PendingIntent intent, String placeholder) {
        return this;
    }

    @Override
    public NotificationBuilderBase addSettingsAction(
            int iconId, @Nullable CharSequence title, @Nullable PendingIntent intent) {
        return this;
    }

    /**
     * Shows the work profile badge if it is needed.
     */
    private void addWorkProfileBadge(RemoteViews view) {
        Resources resources = mContext.getResources();
        DisplayMetrics metrics = resources.getDisplayMetrics();
        int size = dpToPx(metrics, WORK_PROFILE_BADGE_SIZE_DP);
        int[] colors = new int[size * size];

        // Create an immutable bitmap, so that it can not be reused for painting a badge into it.
        Bitmap bitmap = Bitmap.createBitmap(colors, size, size, Bitmap.Config.ARGB_8888);

        Drawable inputDrawable = new BitmapDrawable(resources, bitmap);
        Drawable outputDrawable = ApiCompatibilityUtils.getUserBadgedDrawableForDensity(
                inputDrawable, null /* badgeLocation */, metrics.densityDpi);

        // The input bitmap is immutable, so the output drawable will be a different instance from
        // the input drawable if the work profile badge was applied.
        if (inputDrawable != outputDrawable && outputDrawable instanceof BitmapDrawable) {
            view.setImageViewBitmap(
                    R.id.work_profile_badge, ((BitmapDrawable) outputDrawable).getBitmap());
            view.setViewVisibility(R.id.work_profile_badge, View.VISIBLE);
        }
    }

    /**
     * Scales down the maximum number of displayed lines in the body text if font scaling is greater
     * than 1.0. Never scales up the number of lines, as on some devices the notification text is
     * rendered in dp units (which do not scale) and additional lines could lead to cropping at the
     * bottom of the notification.
     *
     * @param fontScale The current system font scaling factor.
     * @return The number of lines to be displayed.
     */
    @VisibleForTesting
    static int calculateMaxBodyLines(float fontScale) {
        if (fontScale > 1.0f) {
            return (int) Math.round(Math.ceil((1 / fontScale) * MAX_BODY_LINES));
        }
        return MAX_BODY_LINES;
    }

    /**
     * Scales down the maximum amount of flexible padding to use if font scaling is over 1.0. Never
     * scales up the amount of padding, as on some devices the notification text is rendered in dp
     * units (which do not scale) and additional padding could lead to cropping at the bottom of the
     * notification. Never scales the padding below zero.
     *
     * @param fontScale The current system font scaling factor.
     * @param metrics The display metrics for the current context.
     * @return The amount of padding to be used, in pixels.
     */
    @VisibleForTesting
    static int calculateScaledPadding(float fontScale, DisplayMetrics metrics) {
        float paddingScale = 1.0f;
        if (fontScale > 1.0f) {
            fontScale = Math.min(fontScale, FONT_SCALE_LARGE);
            paddingScale = (FONT_SCALE_LARGE - fontScale) / (FONT_SCALE_LARGE - 1.0f);
        }
        return dpToPx(metrics, paddingScale * MAX_SCALABLE_PADDING_DP);
    }

    /**
     * Converts a px value to a dp value.
     */
    private static int pxToDp(float value, DisplayMetrics metrics) {
        return Math.round(value / ((float) metrics.densityDpi / DisplayMetrics.DENSITY_DEFAULT));
    }

    /**
     * Whether to use the Material look and feel or fall back to Holo.
     */
    @VisibleForTesting
    static boolean useMaterial() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP;
    }
}

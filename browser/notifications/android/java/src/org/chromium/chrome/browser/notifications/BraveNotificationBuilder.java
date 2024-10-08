/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.app.Notification;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.View;
import android.widget.RemoteViews;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.base.ApplicationStatus;
import org.chromium.chrome.browser.notifications.channels.BraveChannelDefinitions;
import org.chromium.components.browser_ui.notifications.NotificationMetadata;
import org.chromium.components.browser_ui.notifications.NotificationWrapper;
import org.chromium.components.browser_ui.notifications.NotificationWrapperBuilder;
import org.chromium.components.browser_ui.notifications.PendingIntentProvider;

/**
 * Builds a notification according to BraveAds spec.
 */
public class BraveNotificationBuilder extends StandardNotificationBuilder {
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
     * The notification body height in dp for compact view mode for Android 12 and higher.
     */
    private static final float COMPACT_BODY_CONTAINER_HEIGHT_DP = 40f;

    /**
     * The notification title margin top in dp for compact view mode for Android 12 and higher.
     */
    private static final float COMPACT_TITLE_MARGIN_TOP_DP = -4f;

    private static Bitmap sBraveIcon;

    private final Context mContext;

    private boolean mIsBraveAdsNotification;

    public BraveNotificationBuilder(Context context) {
        super(context);

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
        return super.setChannelId(
                mIsBraveAdsNotification ? BraveChannelDefinitions.ChannelId.BRAVE_ADS : channelId);
    }

    @Override
    public NotificationWrapper build(NotificationMetadata metadata) {
        if (!mIsBraveAdsNotification) {
            return super.build(metadata);
        }

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
        }

        // Support following custom notification changes on Android 12:
        // - custom notification already contains icon
        // - the dimensions become less than before.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            compactView.setViewPadding(R.id.title, 0, 0, 0, 0);
            compactView.setViewLayoutMargin(R.id.title, RemoteViews.MARGIN_TOP,
                    COMPACT_TITLE_MARGIN_TOP_DP, TypedValue.COMPLEX_UNIT_DIP);
            compactView.setViewPadding(R.id.body_container, 0, scaledPadding, 0, 0);
            compactView.setViewLayoutHeight(R.id.body_container, COMPACT_BODY_CONTAINER_HEIGHT_DP,
                    TypedValue.COMPLEX_UNIT_DIP);
            compactView.setViewVisibility(R.id.icon_frame, View.GONE);
            bigView.setViewVisibility(R.id.icon_frame, View.GONE);
        }

        // Note: under the hood this is not a NotificationCompat builder so be mindful of the
        // API level of methods you call on the builder.
        // TODO(crbug.com/697104) We should probably use a Compat builder.
        String channelId =
                ApplicationStatus.hasVisibleActivities()
                        ? BraveChannelDefinitions.ChannelId.BRAVE_ADS
                        : BraveChannelDefinitions.ChannelId.BRAVE_ADS_BACKGROUND;
        NotificationWrapperBuilder builder =
                NotificationWrapperBuilderFactory.createNotificationWrapperBuilder(
                        channelId, metadata);
        builder.setTicker(mTickerText);
        builder.setContentIntent(mContentIntent);
        builder.setDeleteIntent(mDeleteIntent);

        int priority =
                ApplicationStatus.hasVisibleActivities()
                        ? Notification.PRIORITY_HIGH
                        : Notification.PRIORITY_LOW;
        builder.setPriorityBeforeO(priority);
        builder.setDefaults(mDefaults);
        if (mVibratePattern != null) builder.setVibrate(mVibratePattern);
        builder.setWhen(mTimestamp);
        builder.setShowWhen(true);
        builder.setOnlyAlertOnce(!mRenotify);
        builder.setContent(compactView);
        builder.setAutoCancel(true);
        builder.setLocalOnly(true); // Disables showing on other devices, e.g. Android Wear

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
        if (sBraveIcon == null) {
            int largeIconId = R.mipmap.app_icon;
            Resources resources = mContext.getResources();
            DisplayMetrics metrics = resources.getDisplayMetrics();
            int iconSize = dpToPx(metrics, MAX_ACTION_ICON_WIDTH_DP);
            sBraveIcon = Bitmap.createScaledBitmap(
                    BitmapFactory.decodeResource(resources, largeIconId), iconSize, iconSize, true);
        }
        return sBraveIcon;
    }

    @Override
    public NotificationBuilderBase addButtonAction(@Nullable Bitmap iconBitmap,
            @Nullable CharSequence title, PendingIntentProvider intent) {
        return this;
    }

    /**
     * Adds an action to the notification, displayed as a button adjacent to the notification
     * content, which when tapped will trigger a remote input. This enables Android Wear input and,
     * from Android N, displays a text box within the notification for inline replies.
     */
    @Override
    public NotificationBuilderBase addTextAction(@Nullable Bitmap iconBitmap,
            @Nullable CharSequence title, PendingIntentProvider intent, String placeholder) {
        return this;
    }

    @Override
    public NotificationBuilderBase addSettingsAction(
            int iconId,
            @Nullable CharSequence title,
            PendingIntentProvider intent,
            @NotificationUmaTracker.ActionType int umaActionType) {
        return this;
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

    public void setIsBraveNotification(boolean isBraveAdsNotification) {
        mIsBraveAdsNotification = isBraveAdsNotification;
    }
}

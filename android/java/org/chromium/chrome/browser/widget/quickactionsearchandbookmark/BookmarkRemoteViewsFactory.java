/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.quickactionsearchandbookmark;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.widget.RemoteViews;
import android.widget.RemoteViewsService;

import org.chromium.base.ThreadUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.ui.favicon.FaviconUtils;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetProvider.WidgetTile;
import org.chromium.components.browser_ui.widget.RoundedIconGenerator;
import org.chromium.components.favicon.IconType;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/** RemoteViewsFactory that provides views for the bookmark GridView. */
@NullMarked
public class BookmarkRemoteViewsFactory implements RemoteViewsService.RemoteViewsFactory {
    private static final String TAG = "BookmarkFactory";
    private static final int DESIRED_ICON_SIZE = 44;
    // Timeout for icon loading - generous to account for UI thread contention and database I/O
    // No network calls are made, but database reads and UI thread scheduling can be slow
    private static final int ICON_LOAD_TIMEOUT_MS = 2000; // 2 seconds

    private final Context mContext;
    private final Map<String, Bitmap> mIconCache; // Cache loaded icons
    private List<WidgetTile> mWidgetTiles;
    private @Nullable LargeIconBridge mLargeIconBridge;

    public BookmarkRemoteViewsFactory(Context context, Intent intent) {
        mContext = context;
        mWidgetTiles = new ArrayList<>();
        mIconCache = new HashMap<>();
    }

    @Override
    public void onCreate() {
        onDataSetChanged();
    }

    @Override
    public void onDataSetChanged() {
        // Load the new bookmark tiles from shared preferences
        List<WidgetTile> newTiles =
                QuickActionSearchAndBookmarkWidgetProvider.DataManager.readWidgetTiles();

        // Clean up cache: remove icons for tiles that no longer exist
        Set<String> newUrls = new HashSet<>();
        for (WidgetTile tile : newTiles) {
            if (tile.getGURL() != null && tile.getGURL().isValid()) {
                newUrls.add(tile.getGURL().getSpec());
            }
        }

        // Remove cached icons that are no longer needed
        mIconCache.keySet().retainAll(newUrls);

        mWidgetTiles = newTiles;
    }

    @Override
    public void onDestroy() {
        mWidgetTiles.clear();
        mIconCache.clear();
        ThreadUtils.runOnUiThread(
                () -> {
                    if (mLargeIconBridge != null) {
                        mLargeIconBridge.destroy();
                        mLargeIconBridge = null;
                    }
                });
    }

    @Override
    public int getCount() {
        return mWidgetTiles.size();
    }

    @Override
    public @Nullable RemoteViews getViewAt(int position) {
        if (position < 0 || position >= mWidgetTiles.size()) {
            return null;
        }

        WidgetTile tile = mWidgetTiles.get(position);
        RemoteViews rv =
                new RemoteViews(mContext.getPackageName(), R.layout.widget_bookmark_grid_item);

        // Set the bookmark name
        rv.setTextViewText(R.id.bookmark_name, tile.getTitle());

        // Load real favicon icon
        loadFaviconForTile(rv, tile);

        // Set up the click intent - this will be merged with the template intent
        Intent fillInIntent = new Intent();
        fillInIntent.setData(Uri.parse(tile.getUrl()));
        // Set the click on the entire container so the whole tile is clickable
        rv.setOnClickFillInIntent(R.id.bookmark_icon, fillInIntent);
        rv.setOnClickFillInIntent(R.id.bookmark_name, fillInIntent);

        return rv;
    }

    private void loadFaviconForTile(RemoteViews rv, WidgetTile tile) {
        GURL gurl = tile.getGURL();
        if (gurl == null || !gurl.isValid()) {
            return;
        }

        String url = gurl.getSpec();

        // Check cache first
        final Bitmap cachedIcon = mIconCache.get(url);
        if (cachedIcon != null) {
            rv.setImageViewBitmap(R.id.bookmark_icon, cachedIcon);
            return;
        }

        // Load icon on UI thread with timeout
        // Use AtomicReference for thread-safe access across UI and background threads
        final AtomicReference<@Nullable Bitmap> iconResult = new AtomicReference<>();
        final AtomicReference<Integer> fallbackColorResult = new AtomicReference<>();
        // CountDownLatch provides both signaling (countDown) and blocking (await) mechanisms
        final CountDownLatch latch = new CountDownLatch(1);

        // Post icon loading to UI thread (non-blocking) and wait via latch
        // Background Thread          UI Thread
        //     |                         |
        //     |----runOnUiThread------->|
        //     |                    Load Icon (async)
        //     |    (wait at latch)      |
        //     |                    Callback fires
        //     |                    countDown()
        //     |<--latch released--------|
        // Continue...
        ThreadUtils.runOnUiThread(
                () -> {
                    try {
                        // Lazily initialize LargeIconBridge on first use
                        if (mLargeIconBridge == null) {
                            // Widgets are top-level entry points similar to Activities, so
                            // accessing the last used profile here is acceptable since
                            // there's no existing Profile context to pass through the
                            // RemoteViewsFactory chain. The same approach is used in
                            // upstream Chromium's bookmark widget.
                            mLargeIconBridge =
                                    new LargeIconBridge(
                                            ProfileManager.getLastUsedRegularProfile()); // nocheck
                        }

                        LargeIconCallback callback =
                                new LargeIconCallback() {
                                    @Override
                                    public void onLargeIconAvailable(
                                            @Nullable Bitmap icon,
                                            int fallbackColor,
                                            boolean isFallbackColorDefault,
                                            @IconType int iconType) {
                                        iconResult.set(icon);
                                        fallbackColorResult.set(fallbackColor);
                                        latch.countDown();
                                    }
                                };

                        mLargeIconBridge.getLargeIconForUrl(gurl, DESIRED_ICON_SIZE, callback);

                    } catch (Exception e) {
                        latch.countDown();
                    }
                });

        try {
            // Wait for icon with timeout
            boolean loaded = latch.await(ICON_LOAD_TIMEOUT_MS, TimeUnit.MILLISECONDS);

            Bitmap icon = iconResult.get();
            Integer fallbackColor = fallbackColorResult.get();

            Bitmap finalIcon;
            if (loaded && icon != null) {
                // Use the favicon as-is - already properly formatted from LargeIconBridge
                finalIcon = icon;
            } else if (loaded && fallbackColor != null) {
                finalIcon = getTileIconFromColor(gurl, fallbackColor);
            } else {
                // Timeout or missing fallback color - use Brave brand color as fallback
                int defaultFallbackColor = mContext.getColor(R.color.primitive_blurple_40);
                finalIcon = getTileIconFromColor(gurl, defaultFallbackColor);
            }

            // Cache the icon
            mIconCache.put(url, finalIcon);
            rv.setImageViewBitmap(R.id.bookmark_icon, finalIcon);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    private @Nullable Bitmap getTileIconFromColor(GURL gurl, int fallbackColor) {
        RoundedIconGenerator iconGenerator =
                FaviconUtils.createRoundedRectangleIconGenerator(mContext);
        iconGenerator.setBackgroundColor(fallbackColor);
        return iconGenerator.generateIconForUrl(gurl);
    }

    @Override
    public @Nullable RemoteViews getLoadingView() {
        return null;
    }

    @Override
    public int getViewTypeCount() {
        // All items use the same layout (widget_bookmark_grid_item.xml)
        return 1;
    }

    @Override
    public long getItemId(int position) {
        // Use URL hash as stable identifier for better update handling
        if (position >= 0 && position < mWidgetTiles.size()) {
            return mWidgetTiles.get(position).getUrl().hashCode();
        }
        return position;
    }

    @Override
    public boolean hasStableIds() {
        // IDs are based on URL, which is stable across data changes
        return true;
    }
}

/*
 Copyright (c) 2022 The Brave Authors. All rights reserved.
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this file,
 You can obtain one at https://mozilla.org/MPL/2.0/.
*/

package org.chromium.chrome.browser.widget.quickactionsearchandbookmark;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.RemoteViews;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveIntentHandler;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.browserservices.intents.WebappConstants;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.init.EmptyBrowserParts;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.searchwidget.SearchActivity;
import org.chromium.chrome.browser.searchwidget.SearchActivityClientImpl;
import org.chromium.chrome.browser.searchwidget.SearchWidgetProvider;
import org.chromium.chrome.browser.suggestions.tile.Tile;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityClient;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityExtras.IntentOrigin;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityExtras.SearchType;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.utils.BraveSearchWidgetUtils;
import org.chromium.components.webapps.ShortcutSource;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class QuickActionSearchAndBookmarkWidgetProvider extends AppWidgetProvider {
    private static final String TAG = "WidgetProvider";

    public static String FROM_SETTINGS = "FROM_SETTINGS";

    // Tiles row is ~72dp + search bar height(48dp)(depends on the text size
    // settings on the device)
    private static final int MIN_VISIBLE_HEIGHT_ROW_1 = 130;

    private static final Object LOCK = new Object();
    private static Set<Runnable> sUpdateAppWidgetsRunnables;

    private boolean mNativeLoaded;

    public QuickActionSearchAndBookmarkWidgetProvider() {
        mNativeLoaded = false;
        QuickActionSearchAndBookmarkWidgetProvider.sUpdateAppWidgetsRunnables =
                new HashSet<Runnable>();
        final BrowserParts parts =
                new EmptyBrowserParts() {
                    @Override
                    public void finishNativeInitialization() {
                        synchronized (QuickActionSearchAndBookmarkWidgetProvider.LOCK) {
                            mNativeLoaded = true;
                            for (Runnable runnable :
                                    QuickActionSearchAndBookmarkWidgetProvider
                                            .sUpdateAppWidgetsRunnables) {
                                PostTask.postTask(TaskTraits.UI_DEFAULT, runnable);
                            }
                            QuickActionSearchAndBookmarkWidgetProvider.sUpdateAppWidgetsRunnables
                                    .clear();
                        }
                    }
                };

        try {
            ChromeBrowserInitializer.getInstance().handlePreNativeStartupAndLoadLibraries(parts);

            ChromeBrowserInitializer.getInstance().handlePostNativeStartup(
                    true /* isAsync */, parts);
        } catch (ProcessInitException e) {
            Log.e(TAG, "Background Launch Error", e);
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        super.onReceive(context, intent);
        String widgetAddedToHomeScreen = context.getString(R.string.widget_added_to_home_screen);
        boolean isComingFromSettings = intent.getBooleanExtra(FROM_SETTINGS, false);
        if (isComingFromSettings) {
            Toast.makeText(context, widgetAddedToHomeScreen, Toast.LENGTH_SHORT).show();
        }
        BraveSearchWidgetUtils.setShouldShowWidgetPromo(false);
    }

    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {
        super.onUpdate(context, appWidgetManager, appWidgetIds);
        runUpdateAppWidgetsWithNative(appWidgetIds);
    }

    @Override
    public void onAppWidgetOptionsChanged(Context context, AppWidgetManager appWidgetManager,
            int appWidgetId, Bundle newOptions) {
        super.onAppWidgetOptionsChanged(context, appWidgetManager, appWidgetId, newOptions);
        runUpdateAppWidgetsWithNative(new int[] {appWidgetId});
    }

    private void runUpdateAppWidgetsWithNative(int[] appWidgetIds) {
        synchronized (QuickActionSearchAndBookmarkWidgetProvider.LOCK) {
            if (!mNativeLoaded) {
                QuickActionSearchAndBookmarkWidgetProvider.sUpdateAppWidgetsRunnables.add(
                        buildStartWithNativeRunnable(appWidgetIds));

                return;
            }
        }

        QuickActionSearchAndBookmarkWidgetProvider.updateAppWidgets(appWidgetIds);
    }

    /** Builds a runnable starting task with native portion. */
    private Runnable buildStartWithNativeRunnable(final int[] appWidgetIds) {
        return new Runnable() {
            @Override
            public void run() {
                ThreadUtils.assertOnUiThread();
                QuickActionSearchAndBookmarkWidgetProvider.updateAppWidgets(appWidgetIds);
            }
        };
    }

    public static void notifyWidgetDataChanged() {
        Context context = ContextUtils.getApplicationContext();
        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
        int[] appWidgetIds = getAppWidgetIds(context, appWidgetManager);
        for (int appWidgetId : appWidgetIds) {
            appWidgetManager.notifyAppWidgetViewDataChanged(appWidgetId, R.id.bookmarksGridView);
        }
    }

    private static void updateAppWidgets() {
        Context context = ContextUtils.getApplicationContext();
        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
        updateAppWidgets(getAppWidgetIds(context, appWidgetManager));
    }

    private static RemoteViews getBaseRemoteViews() {
        return new RemoteViews(ContextUtils.getApplicationContext().getPackageName(),
                R.layout.quick_action_search_and_bookmark_widget_layout);
    }

    private static int[] getAppWidgetIds(Context context, AppWidgetManager appWidgetManager) {
        return appWidgetManager.getAppWidgetIds(
                new ComponentName(context, QuickActionSearchAndBookmarkWidgetProvider.class));
    }

    private static void updateAppWidgets(int[] appWidgetIds) {
        Context context = ContextUtils.getApplicationContext();
        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
        List<WidgetTile> widgetTileList = DataManager.readWidgetTiles();

        for (int appWidgetId : appWidgetIds) {
            RemoteViews views = getBaseRemoteViews();
            Bundle options = appWidgetManager.getAppWidgetOptions(appWidgetId);
            int minHeight = options.getInt(AppWidgetManager.OPTION_APPWIDGET_MIN_HEIGHT);
            int maxHeight = options.getInt(AppWidgetManager.OPTION_APPWIDGET_MAX_HEIGHT);
            setSearchBarPendingIntent(context, views);

            // Determine if we should show tiles
            // Use maxHeight in portrait, minHeight in landscape
            boolean isPortrait = !ConfigurationUtils.isLandscape(context);
            int currentHeight = isPortrait ? maxHeight : minHeight;
            boolean shouldShowTiles =
                    widgetTileList.size() > 0 && currentHeight >= MIN_VISIBLE_HEIGHT_ROW_1;
            if (shouldShowTiles) {
                // Setup GridView and show it
                setupGridView(context, views, appWidgetId);
                views.setViewVisibility(R.id.bookmarksGridView, View.VISIBLE);
            } else {
                // Hide GridView when not needed
                views.setViewVisibility(R.id.bookmarksGridView, View.GONE);
            }

            setDynamicLayout(views, widgetTileList.size(), currentHeight);
            // Update widget first to register RemoteViews with AppWidgetManager
            appWidgetManager.updateAppWidget(appWidgetId, views);

            // Notify GridView about data changes AFTER the widget has been updated
            if (shouldShowTiles) {
                appWidgetManager.notifyAppWidgetViewDataChanged(
                        appWidgetId, R.id.bookmarksGridView);
            }
        }
    }

    private static void setupGridView(Context context, RemoteViews views, int appWidgetId) {
        // Set up the intent for the GridView
        Intent intent = new Intent(context, QuickActionSearchAndBookmarkWidgetService.class);
        intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetId);
        intent.setData(Uri.parse(intent.toUri(Intent.URI_INTENT_SCHEME)));

        views.setRemoteAdapter(R.id.bookmarksGridView, intent);
        views.setEmptyView(R.id.bookmarksGridView, android.R.id.empty);

        // Set up the click intent template for grid items
        // Use explicit intent to ChromeLauncherActivity instead of implicit ACTION_VIEW
        Intent clickIntent = new Intent(context, ChromeLauncherActivity.class);
        clickIntent.setAction(Intent.ACTION_VIEW);
        clickIntent.addCategory(Intent.CATEGORY_BROWSABLE);
        clickIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        clickIntent.putExtra(
                WebappConstants.EXTRA_SOURCE, ShortcutSource.BOOKMARK_NAVIGATOR_WIDGET);
        clickIntent.putExtra(WebappConstants.REUSE_URL_MATCHING_TAB_ELSE_NEW_TAB, true);

        // For GridView with fillInIntent, we need FLAG_MUTABLE to allow the URL data to be filled
        // in
        PendingIntent clickPendingIntent =
                PendingIntent.getActivity(
                        context,
                        0,
                        clickIntent,
                        PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);

        views.setPendingIntentTemplate(R.id.bookmarksGridView, clickPendingIntent);
    }

    private static void setSearchBarPendingIntent(Context context, RemoteViews views) {
        // Request code should be unique. By using distinct request codes,
        // we allow the system to differentiate between the intents,
        // ensuring that each button triggers its respective action.
        int requestCode = 0;
        views.setOnClickPendingIntent(
                R.id.ivIncognito, createIncognitoIntent(context, ++requestCode));
        views.setOnClickPendingIntent(
                R.id.layoutSearchWithBrave, createIntent(context, false, ++requestCode));
        views.setOnClickPendingIntent(
                R.id.ivVoiceSearch, createIntent(context, true, ++requestCode));
        views.setOnClickPendingIntent(
                R.id.ibLeo, createPendingIntent(context, createLeoIntent(context), ++requestCode));
    }

    private static void setDynamicLayout(RemoteViews views, int tilesSize, int currentHeight) {
        // Check if tiles should be visible
        boolean shouldShowTiles = tilesSize > 0 && currentHeight >= MIN_VISIBLE_HEIGHT_ROW_1;
        if (shouldShowTiles) {
            // When top tiles are visible:
            // 1. Set gravity to "top" to align content to top
            views.setInt(R.id.background, "setGravity", android.view.Gravity.TOP);
            // 2. Set layout height to match_parent (API 31+)
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S) {
                try {
                    views.setViewLayoutHeight(
                            R.id.background,
                            android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                            android.util.TypedValue.COMPLEX_UNIT_PX);
                } catch (Exception e) {
                    Log.d(TAG, "Could not set layout height to MATCH_PARENT", e);
                }
            }
        } else {
            // When no top tiles are shown:
            // 1. Set gravity to "center" to center the search bar
            views.setInt(R.id.background, "setGravity", android.view.Gravity.CENTER);
            // 2. Reset layout height to wrap_content (API 31+)
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S) {
                try {
                    views.setViewLayoutHeight(
                            R.id.background,
                            android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
                            android.util.TypedValue.COMPLEX_UNIT_PX);
                } catch (Exception e) {
                    Log.d(TAG, "Could not set layout height to WRAP_CONTENT", e);
                }
            }
        }
    }

    private static PendingIntent createIntent(
            @NonNull Context context, boolean startVoiceSearch, int requestCode) {
        SearchActivityClient client =
                new SearchActivityClientImpl(context, IntentOrigin.SEARCH_WIDGET);

        Intent searchIntent =
                client.newIntentBuilder()
                        .setSearchType(startVoiceSearch ? SearchType.VOICE : SearchType.TEXT)
                        .build();

        searchIntent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);
        searchIntent.setComponent(new ComponentName(context, SearchActivity.class));
        searchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return createPendingIntent(context, searchIntent, requestCode);
    }

    private static PendingIntent createIncognitoIntent(Context context, int requestCode) {
        Intent trustedIncognitoIntent =
                IntentHandler.createTrustedOpenNewTabIntent(context, /* incognito= */ true);
        trustedIncognitoIntent.putExtra(IntentHandler.EXTRA_INVOKED_FROM_APP_WIDGET, true);
        trustedIncognitoIntent.addFlags(
                Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        trustedIncognitoIntent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);
        return createPendingIntent(context, trustedIncognitoIntent, requestCode);
    }

    public static Intent createLeoIntent(Context context) {
        Intent trustedIncognitoIntent =
                IntentHandler.createTrustedOpenNewTabIntent(context, /* incognito= */ false);
        trustedIncognitoIntent.putExtra(IntentHandler.EXTRA_INVOKED_FROM_APP_WIDGET, true);
        trustedIncognitoIntent.putExtra(BraveIntentHandler.EXTRA_INVOKED_FROM_APP_WIDGET_LEO, true);
        trustedIncognitoIntent.addFlags(
                Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        trustedIncognitoIntent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);
        return trustedIncognitoIntent;
    }

    private static PendingIntent createPendingIntent(
            Context context, Intent intent, int requestCode) {
        return PendingIntent.getActivity(
                context,
                requestCode,
                intent,
                PendingIntent.FLAG_UPDATE_CURRENT
                        | IntentUtils.getPendingIntentMutabilityFlag(false));
    }

    /**
     * This class acts as a single source of truth for this widet. This widget would use this class
     * to fetch the data. Also, any modification to the data should be done through this class.
     */
    public static class DataManager {
        public static void parseTilesAndWriteWidgetTiles(List<Tile> tiles) {
            List<WidgetTile> widgetTileList = new ArrayList<>();
            for (Tile tile : tiles) {
                WidgetTile widgetTile = new WidgetTile(tile.getTitle(), tile.getUrl());
                widgetTileList.add(widgetTile);
            }
            writeWidgetTiles(widgetTileList);
        }

        public static void writeWidgetTiles(List<WidgetTile> widgetTileList) {
            JSONArray widgetTilesJsonArray = new JSONArray();
            for (WidgetTile widgetTile : widgetTileList) {
                if (widgetTile.toJSONObject() != null) {
                    widgetTilesJsonArray.put(widgetTile.toJSONObject());
                }
            }
            ChromeSharedPreferences.getInstance()
                    .writeString(
                            BravePreferenceKeys.BRAVE_QUICK_ACTION_SEARCH_AND_BOOKMARK_WIDGET_TILES,
                            widgetTilesJsonArray.toString());
            updateAppWidgets();
        }

        public static List<WidgetTile> readWidgetTiles() {
            String widgetTilesJson =
                    ChromeSharedPreferences.getInstance()
                            .readString(
                                    BravePreferenceKeys
                                            .BRAVE_QUICK_ACTION_SEARCH_AND_BOOKMARK_WIDGET_TILES,
                                    null);
            List<WidgetTile> widgetTileList = new ArrayList();

            // Check if there's any saved data
            if (widgetTilesJson == null || widgetTilesJson.isEmpty()) {
                return widgetTileList; // Return empty list
            }
            widgetTilesJson = "{\"widgetTiles\":" + widgetTilesJson + "}";
            try {
                JSONObject result = new JSONObject(widgetTilesJson);
                JSONArray widgetTilesJsonArray = result.getJSONArray("widgetTiles");
                for (int i = 0; i < widgetTilesJsonArray.length(); i++) {
                    JSONObject widgetTileJsonObject = widgetTilesJsonArray.getJSONObject(i);
                    WidgetTile widgetTile = new WidgetTile(widgetTileJsonObject.getString("title"),
                            new GURL(widgetTileJsonObject.getString("gurl")));
                    widgetTileList.add(widgetTile);
                }
            } catch (Exception e) {
                Log.e(TAG, "Error parsing widget tiles", e);
            }
            return widgetTileList;
        }
    }

    /** A short class for tile. It keeps only information needed to this widget. */
    public static class WidgetTile {
        private String mTitle;
        private GURL mGurl;

        public WidgetTile(String title, GURL gurl) {
            mTitle = title;
            mGurl = gurl;
        }

        public String getUrl() {
            return getGURL() != null ? getGURL().getSpec() : null;
        }

        public GURL getGURL() {
            return mGurl;
        }

        public String getTitle() {
            return mTitle;
        }

        public void parseTile(Tile tile) {
            mGurl = tile.getUrl();
            mTitle = tile.getTitle();
        }

        public JSONObject toJSONObject() {
            JSONObject jsonObject = new JSONObject();
            try {
                jsonObject.put("title", getTitle());
                jsonObject.put("gurl", getUrl());
                return jsonObject;
            } catch (JSONException e) {
                e.printStackTrace();
                return null;
            }
        }

        @Override
        public boolean equals(@Nullable Object obj) {
            if (obj instanceof WidgetTile) {
                return Objects.equals(getUrl(), ((WidgetTile) obj).getUrl());
            } else if (obj instanceof Tile) {
                Tile tile = (Tile) obj;
                if (tile.getUrl() != null) return Objects.equals(getUrl(), tile.getUrl().getSpec());
            } else if (obj instanceof String) {
                return Objects.equals(getUrl(), (String) obj);
            }
            return false;
        }

        @Override
        public int hashCode() {
            return Objects.hashCode(this.getUrl());
        }
    }
}

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
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.RemoteViews;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.drawable.RoundedBitmapDrawable;

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
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.browserservices.intents.WebappConstants;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.init.EmptyBrowserParts;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.searchwidget.SearchActivity;
import org.chromium.chrome.browser.searchwidget.SearchActivityClientImpl;
import org.chromium.chrome.browser.searchwidget.SearchWidgetProvider;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.suggestions.tile.Tile;
import org.chromium.chrome.browser.ui.favicon.FaviconUtils;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityClient;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityExtras;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityPreferencesManager;
import org.chromium.chrome.browser.ui.searchactivityutils.SearchActivityPreferencesManager.SearchActivityPreferences;
import org.chromium.chrome.browser.widget.quickactionsearchandbookmark.utils.BraveSearchWidgetUtils;
import org.chromium.components.browser_ui.widget.RoundedIconGenerator;
import org.chromium.components.favicon.IconType;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.components.favicon.LargeIconBridge.LargeIconCallback;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.webapps.ShortcutSource;
import org.chromium.ui.base.ViewUtils;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.function.Consumer;

public class QuickActionSearchAndBookmarkWidgetProvider extends AppWidgetProvider {
    private static final String TAG = "WidgetProvider";

    static class QuickActionSearchAndBookmarkWidgetProviderDelegate
            implements Consumer<SearchActivityPreferences> {
        public QuickActionSearchAndBookmarkWidgetProviderDelegate() {}

        @Override
        public void accept(SearchActivityPreferences prefs) {
            if (prefs == null) prefs = SearchActivityPreferencesManager.getCurrent();
            updateSearchEngine(prefs.searchEngineName);
        }
    }

    public static String FROM_SETTINGS = "FROM_SETTINGS";

    private static final int TOTAL_TILES = 16;
    private static final int TILES_PER_ROW = 4;
    private static final int MIN_VISIBLE_HEIGHT_ROW_1 = 125;
    private static final int MIN_VISIBLE_HEIGHT_ROW_2 = 220;
    private static final int MIN_VISIBLE_HEIGHT_ROW_3 = 252;
    private static final int MIN_VISIBLE_HEIGHT_ROW_4 = 320;
    private static final int DESIRED_ICON_SIZE = 44;
    private static final int DESIRED_ICON_RADIUS = 16;

    private final static int[][][] tileViewsIdArray = new int[][][] {
            {
                    {R.id.ivRow1Bookmark1Icon, R.id.tvRow1Bookmark1Name, R.id.layoutRow1Bookmark1},
                    {R.id.ivRow1Bookmark2Icon, R.id.tvRow1Bookmark2Name, R.id.layoutRow1Bookmark2},
                    {R.id.ivRow1Bookmark3Icon, R.id.tvRow1Bookmark3Name, R.id.layoutRow1Bookmark3},
                    {R.id.ivRow1Bookmark4Icon, R.id.tvRow1Bookmark4Name, R.id.layoutRow1Bookmark4},
            },
            {
                    {R.id.ivRow2Bookmark1Icon, R.id.tvRow2Bookmark1Name, R.id.layoutRow2Bookmark1},
                    {R.id.ivRow2Bookmark2Icon, R.id.tvRow2Bookmark2Name, R.id.layoutRow2Bookmark2},
                    {R.id.ivRow2Bookmark3Icon, R.id.tvRow2Bookmark3Name, R.id.layoutRow2Bookmark3},
                    {R.id.ivRow2Bookmark4Icon, R.id.tvRow2Bookmark4Name, R.id.layoutRow2Bookmark4},
            },
            {
                    {R.id.ivRow3Bookmark1Icon, R.id.tvRow3Bookmark1Name, R.id.layoutRow3Bookmark1},
                    {R.id.ivRow3Bookmark2Icon, R.id.tvRow3Bookmark2Name, R.id.layoutRow3Bookmark2},
                    {R.id.ivRow3Bookmark3Icon, R.id.tvRow3Bookmark3Name, R.id.layoutRow3Bookmark3},
                    {R.id.ivRow3Bookmark4Icon, R.id.tvRow3Bookmark4Name, R.id.layoutRow3Bookmark4},
            },
            {
                    {R.id.ivRow4Bookmark1Icon, R.id.tvRow4Bookmark1Name, R.id.layoutRow4Bookmark1},
                    {R.id.ivRow4Bookmark2Icon, R.id.tvRow4Bookmark2Name, R.id.layoutRow4Bookmark2},
                    {R.id.ivRow4Bookmark3Icon, R.id.tvRow4Bookmark3Name, R.id.layoutRow4Bookmark3},
                    {R.id.ivRow4Bookmark4Icon, R.id.tvRow4Bookmark4Name, R.id.layoutRow4Bookmark4},
            },
    };

    private static QuickActionSearchAndBookmarkWidgetProviderDelegate mDelegate;
    private static final Object mLock = new Object();
    private static Set<Runnable> mUpdateAppWidgetsRunnables;

    private boolean mNativeLoaded;

    public QuickActionSearchAndBookmarkWidgetProvider() {
        mNativeLoaded = false;
        QuickActionSearchAndBookmarkWidgetProvider.mUpdateAppWidgetsRunnables =
                new HashSet<Runnable>();
        final BrowserParts parts = new EmptyBrowserParts() {
            @Override
            public void finishNativeInitialization() {
                synchronized (QuickActionSearchAndBookmarkWidgetProvider.mLock) {
                    mNativeLoaded = true;
                    for (Runnable runnable :
                            QuickActionSearchAndBookmarkWidgetProvider.mUpdateAppWidgetsRunnables) {
                        PostTask.postTask(TaskTraits.UI_DEFAULT, runnable);
                    }
                    QuickActionSearchAndBookmarkWidgetProvider.mUpdateAppWidgetsRunnables.clear();
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

    public static void initializeDelegate() {
        SearchActivityPreferencesManager.addObserver(getDelegate());
    }

    private static QuickActionSearchAndBookmarkWidgetProviderDelegate getDelegate() {
        if (mDelegate == null) {
            mDelegate = new QuickActionSearchAndBookmarkWidgetProviderDelegate();
        }
        return mDelegate;
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
        synchronized (QuickActionSearchAndBookmarkWidgetProvider.mLock) {
            if (!mNativeLoaded) {
                QuickActionSearchAndBookmarkWidgetProvider.mUpdateAppWidgetsRunnables.add(
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

    public static void updateTileIcon(Tile tile) {
        int index = indexOf(tile);
        if (index != -1)
            updateTileIcon(tileViewsIdArray[index / TILES_PER_ROW][index % TILES_PER_ROW][0],
                    getBitmap(tile.getIcon()));
    }

    public static void updateSearchEngine(String searchEngine) {
        if (searchEngine == null) return;
        Context context = ContextUtils.getApplicationContext();
        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
        int[] appWidgetIds = getAppWidgetIds(context, appWidgetManager);
        for (int appWidgetId : appWidgetIds) {
            RemoteViews views = getBaseRemoteViews();
            String searchWithDefaultSearchEngine =
                    context.getString(R.string.search_with_search_engine, searchEngine);
            views.setTextViewText(R.id.tvSearchWithBrave, searchWithDefaultSearchEngine);
            appWidgetManager.partiallyUpdateAppWidget(appWidgetId, views);
        }
    }

    private static void updateAppWidgets() {
        Context context = ContextUtils.getApplicationContext();
        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
        updateAppWidgets(getAppWidgetIds(context, appWidgetManager));
    }

    private static int indexOf(Tile tile) {
        List<WidgetTile> widgetTileList = DataManager.readWidgetTiles();
        for (int i = 0; i < widgetTileList.size(); i++) {
            if (widgetTileList.get(i).getUrl().equals(tile.getUrl().getSpec())) return i;
        }
        return -1;
    }

    private static void updateTileIcon(int imageView, Bitmap icon) {
        Context context = ContextUtils.getApplicationContext();
        AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
        int[] appWidgetIds = getAppWidgetIds(context, appWidgetManager);
        for (int appWidgetId : appWidgetIds) {
            RemoteViews views = getBaseRemoteViews();
            views.setImageViewBitmap(imageView, icon);
            appWidgetManager.partiallyUpdateAppWidget(appWidgetId, views);
        }
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
            setDefaultSearchEngineString(views);
            setSearchBarPendingIntent(context, views);
            setTopTiles(context, views, widgetTileList);
            setRowsVisibility(views, widgetTileList.size(), minHeight);
            appWidgetManager.updateAppWidget(appWidgetId, views);
        }
    }

    private static void setDefaultSearchEngineString(RemoteViews views) {
        final Profile profile = ProfileManager.getLastUsedRegularProfile();
        TemplateUrl templateUrl =
                BraveSearchEngineUtils.getTemplateUrlByShortName(
                        profile, BraveSearchEngineUtils.getDSEShortName(profile, false));
        if (templateUrl != null) {
            String searchWithDefaultSearchEngine =
                    ContextUtils.getApplicationContext()
                            .getString(
                                    R.string.search_with_search_engine, templateUrl.getShortName());
            views.setTextViewText(R.id.tvSearchWithBrave, searchWithDefaultSearchEngine);
        }
    }

    private static void setTopTiles(
            Context context, RemoteViews views, List<WidgetTile> widgetTileList) {
        int tilesSize = widgetTileList.size();
        int i = 0;
        int j = 0;

        while (i < tilesSize && i < TOTAL_TILES) {
            j = j % TILES_PER_ROW;
            int row = i / TILES_PER_ROW;

            WidgetTile tile = widgetTileList.get(i);
            int tileLayoutId = tileViewsIdArray[row][j][2];
            int tileImageViewId = tileViewsIdArray[row][j][0];
            int tileTextViewId = tileViewsIdArray[row][j][1];

            views.setViewVisibility(tileLayoutId, View.VISIBLE);
            views.setOnClickPendingIntent(tileLayoutId, createIntent(context, tile.getUrl()));
            views.setTextViewText(tileTextViewId, tile.getTitle());
            views.setInt(tileImageViewId, "setColorFilter", 0);
            fetchGurlIcon(tileImageViewId, tile.getGURL());

            i++;
            j++;
        }

        // hide and uninitialize the remaining placeholder tiles
        while (i < TOTAL_TILES) {
            j = j % TILES_PER_ROW;
            int row = i / TILES_PER_ROW;
            int tileLayoutId = tileViewsIdArray[row][j][2];
            int tileImageViewId = tileViewsIdArray[row][j][0];
            int tileTextViewId = tileViewsIdArray[row][j][1];
            views.setViewVisibility(tileLayoutId, View.INVISIBLE);
            views.setOnClickPendingIntent(tileLayoutId, null);
            views.setTextViewText(tileTextViewId, "");
            views.setImageViewResource(tileImageViewId, 0);
            i++;
            j++;
        }
    }

    private static void fetchGurlIcon(final int imageViewId, GURL gurl) {
        LargeIconBridge largeIconBridge =
                new LargeIconBridge(ProfileManager.getLastUsedRegularProfile());
        LargeIconCallback callback =
                new LargeIconCallback() {
                    @Override
                    public void onLargeIconAvailable(
                            Bitmap icon,
                            int fallbackColor,
                            boolean isFallbackColorDefault,
                            @IconType int iconType) {
                        if (icon == null)
                            updateTileIcon(imageViewId, getTileIconFromColor(gurl, fallbackColor));
                        else updateTileIcon(imageViewId, getRoundedTileIconFromBitmap(icon));
                    }
                };
        largeIconBridge.getLargeIconForUrl(gurl, DESIRED_ICON_SIZE, callback);
    }

    private static Bitmap getRoundedTileIconFromBitmap(Bitmap icon) {
        RoundedBitmapDrawable roundedIcon = ViewUtils.createRoundedBitmapDrawable(
                ContextUtils.getApplicationContext().getResources(), icon, DESIRED_ICON_RADIUS);
        roundedIcon.setAntiAlias(true);
        roundedIcon.setFilterBitmap(true);
        return getBitmap(roundedIcon);
    }

    private static Bitmap getTileIconFromColor(GURL gurl, int fallbackColor) {
        RoundedIconGenerator mIconGenerator =
                FaviconUtils.createRoundedRectangleIconGenerator(
                        ContextUtils.getApplicationContext());
        mIconGenerator.setBackgroundColor(fallbackColor);
        return mIconGenerator.generateIconForUrl(gurl);
    }

    private static void setSearchBarPendingIntent(Context context, RemoteViews views) {
        views.setOnClickPendingIntent(R.id.ivIncognito, createIncognitoIntent(context));
        views.setOnClickPendingIntent(R.id.layoutSearchWithBrave, createIntent(context, false));
        views.setOnClickPendingIntent(R.id.ivVoiceSearch, createIntent(context, true));
    }

    private static Bitmap getBitmap(@Nullable Drawable drawable) {
        if (drawable != null) {
            Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                    drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
            drawable.draw(canvas);
            return bitmap;
        } else
            return null;
    }

    private static void setRowsVisibility(RemoteViews views, int tilesSize, int minHeight) {
        views.setViewVisibility(R.id.BookmarkLayoutRow1,
                tilesSize > 0 * TILES_PER_ROW && minHeight >= MIN_VISIBLE_HEIGHT_ROW_1
                        ? View.VISIBLE
                        : View.GONE);
        views.setViewVisibility(R.id.BookmarkLayoutRow2,
                tilesSize > 1 * TILES_PER_ROW && minHeight >= MIN_VISIBLE_HEIGHT_ROW_2
                        ? View.VISIBLE
                        : View.GONE);
        views.setViewVisibility(R.id.BookmarkLayoutRow3,
                tilesSize > 2 * TILES_PER_ROW && minHeight >= MIN_VISIBLE_HEIGHT_ROW_3
                        ? View.VISIBLE
                        : View.GONE);
        views.setViewVisibility(R.id.BookmarkLayoutRow4,
                tilesSize > 3 * TILES_PER_ROW && minHeight >= MIN_VISIBLE_HEIGHT_ROW_4
                        ? View.VISIBLE
                        : View.GONE);
    }

    private static PendingIntent createIntent(@NonNull Context context, @NonNull String url) {
        Intent intent = new Intent(
                Intent.ACTION_VIEW, Uri.parse(url), context, ChromeLauncherActivity.class);
        intent.addCategory(Intent.CATEGORY_BROWSABLE);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.putExtra(WebappConstants.EXTRA_SOURCE, ShortcutSource.BOOKMARK_NAVIGATOR_WIDGET);
        intent.putExtra(WebappConstants.REUSE_URL_MATCHING_TAB_ELSE_NEW_TAB, true);
        return createPendingIntent(context, intent);
    }

    private static PendingIntent createIntent(@NonNull Context context, boolean startVoiceSearch) {
        SearchActivityClient client = new SearchActivityClientImpl();
        Intent searchIntent =
                client.createIntent(
                        context,
                        SearchActivityExtras.IntentOrigin.SEARCH_WIDGET,
                        null,
                        startVoiceSearch
                                ? SearchActivityExtras.SearchType.VOICE
                                : SearchActivityExtras.SearchType.TEXT);

        searchIntent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);
        searchIntent.setComponent(new ComponentName(context, SearchActivity.class));
        searchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return createPendingIntent(context, searchIntent);
    }

    private static PendingIntent createIncognitoIntent(Context context) {
        Intent trustedIncognitoIntent =
                IntentHandler.createTrustedOpenNewTabIntent(context, /*incognito=*/true);
        trustedIncognitoIntent.putExtra(IntentHandler.EXTRA_INVOKED_FROM_APP_WIDGET, true);
        trustedIncognitoIntent.addFlags(
                Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        trustedIncognitoIntent.putExtra(SearchWidgetProvider.EXTRA_FROM_SEARCH_WIDGET, true);
        return createPendingIntent(context, trustedIncognitoIntent);
    }

    private static PendingIntent createPendingIntent(Context context, Intent intent) {
        return PendingIntent.getActivity(context, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT
                        | IntentUtils.getPendingIntentMutabilityFlag(false));
    }

    /**
     * This class acts as a single source of truth for this widet. This widget would use this class
     *to fetch the data. Also, any modification to the data should be done through this class.
     **/

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
                e.printStackTrace();
            }
            return widgetTileList;
        }
    }

    /**
     * A short class for tile. It keeps only information needed to this widget.
     **/

    public static class WidgetTile {
        private String title;
        private GURL gurl;

        public WidgetTile(String title, GURL gurl) {
            this.title = title;
            this.gurl = gurl;
        }

        public String getUrl() {
            return getGURL() != null ? getGURL().getSpec() : null;
        }

        public GURL getGURL() {
            return this.gurl;
        }

        public String getTitle() {
            return this.title;
        }

        public void parseTile(Tile tile) {
            this.gurl = tile.getUrl();
            this.title = tile.getTitle();
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

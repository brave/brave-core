/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.mostvisited;

import android.content.SharedPreferences;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.SiteSuggestion;
import org.chromium.chrome.browser.suggestions.tile.Tile;
import org.chromium.chrome.browser.suggestions.tile.TileSource;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;

/**
 * Brave wrapper around {@link MostVisitedSites} that filters the tile list from the bridge based on
 * the current display mode:
 *
 * <ul>
 *   <li><b>Shortcuts mode</b> — passes only {@link TileSource#CUSTOM_LINKS} tiles. Because the
 *       bridge runs in mixed mode (both custom links and top-sites enabled), custom link tiles are
 *       empty until the user adds a shortcut, so the NTP correctly shows only "+" buttons.
 *   <li><b>Frequently visited mode</b> — passes every tile except {@link TileSource#CUSTOM_LINKS}
 *       (see {@link #filterForMode}), so fallback content such as {@link TileSource#POPULAR} tiles
 *       still shows up for profiles with little or no browsing history.
 * </ul>
 *
 * <p>Filtering in Java rather than by changing {@code EnableTileTypes()} in C++ avoids the {@code
 * ShouldQueryTopSites()} fallback issue: when custom links are not yet initialised the C++ layer
 * always queries top-sites regardless of the tile-type options, so both modes would receive
 * identical top-sites data.
 *
 * <p>Mode is stored locally in Android SharedPreferences via {@link NtpUtil#getTopSitesDisplayMode}
 * / {@link NtpUtil#setTopSitesDisplayMode}, which every tile-related call site (including this
 * class) uses directly. The underlying Chrome profile pref {@code ntp.custom_links_visible} is not
 * mirrored: it is registered upstream without {@code SYNCABLE_PREF}, so it never receives an
 * incoming Desktop-sync change to react to, and Brave's own tile filtering never reads it (this
 * bridge always runs the native side in mixed mode, filtering by display mode purely in Java — see
 * the class doc above).
 */
@NullMarked
public class BraveMostVisitedSites implements MostVisitedSites {

    private final MostVisitedSites mBridge;
    private MostVisitedSites.@Nullable Observer mOuterObserver;
    private List<SiteSuggestion> mCachedSuggestions;
    private boolean mHasReceivedData;

    private final SharedPreferences.OnSharedPreferenceChangeListener mPrefListener;

    public BraveMostVisitedSites(Profile profile) {
        this(new MostVisitedSitesBridge(profile, /* enableCustomLinks= */ true));
    }

    // The chromium wrapper is not usable for us, it has no listener registration API.
    @SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
    @VisibleForTesting
    BraveMostVisitedSites(MostVisitedSites bridge) {
        mBridge = bridge;
        mCachedSuggestions = new ArrayList<>();

        mPrefListener =
                (prefs, key) -> {
                    if (BravePreferenceKeys.BRAVE_NTP_TOP_SITES_DISPLAY_MODE.equals(key)) {
                        onModeChanged();
                    }
                };
        ContextUtils.getAppSharedPreferences()
                .registerOnSharedPreferenceChangeListener(mPrefListener);
    }

    @Override
    public void setObserver(MostVisitedSites.Observer observer, int numSites) {
        mOuterObserver = observer;
        mBridge.setObserver(new FilteringObserver(), numSites);
    }

    // See explanation for the same suppression above.
    @SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
    @Override
    public void destroy() {
        ContextUtils.getAppSharedPreferences()
                .unregisterOnSharedPreferenceChangeListener(mPrefListener);
        mBridge.destroy();
    }

    @Override
    public void addBlocklistedUrl(GURL url) {
        mBridge.addBlocklistedUrl(url);
    }

    @Override
    public void removeBlocklistedUrl(GURL url) {
        mBridge.removeBlocklistedUrl(url);
    }

    @Override
    public void recordPageImpression(int tilesCount) {
        mBridge.recordPageImpression(tilesCount);
    }

    @Override
    public void recordTileImpression(Tile tile) {
        mBridge.recordTileImpression(tile);
    }

    @Override
    public void recordOpenedMostVisitedItem(Tile tile) {
        mBridge.recordOpenedMostVisitedItem(tile);
    }

    @Override
    public double getSuggestionScore(GURL url) {
        return mBridge.getSuggestionScore(url);
    }

    @Override
    public boolean addCustomLink(String name, @Nullable GURL url, @Nullable Integer pos) {
        return mBridge.addCustomLink(name, url, pos);
    }

    @Override
    public boolean assignCustomLink(GURL keyUrl, String name, @Nullable GURL url) {
        return mBridge.assignCustomLink(keyUrl, name, url);
    }

    @Override
    public boolean deleteCustomLink(GURL keyUrl) {
        return mBridge.deleteCustomLink(keyUrl);
    }

    @Override
    public boolean hasCustomLink(GURL keyUrl) {
        return mBridge.hasCustomLink(keyUrl);
    }

    @Override
    public boolean reorderCustomLink(GURL keyUrl, int newPos) {
        return mBridge.reorderCustomLink(keyUrl, newPos);
    }

    /**
     * Shortcuts mode keeps only manually-pinned {@link TileSource#CUSTOM_LINKS} tiles. Frequent
     * mode keeps everything else (not just {@link TileSource#TOP_SITES}): the bridge can also
     * surface {@link TileSource#POPULAR}/{@code POPULAR_BAKED_IN} tiles as fallback content for
     * profiles with little or no browsing history, plus other upstream sources (allowlisted,
     * enterprise-shortcut, homepage tiles). Excluding everything but TOP_SITES would leave
     * "frequently visited" empty for exactly the profiles that fallback content exists for.
     */
    @VisibleForTesting
    static List<SiteSuggestion> filterForMode(List<SiteSuggestion> all, int displayMode) {
        List<SiteSuggestion> result = new ArrayList<>(all.size());
        for (SiteSuggestion s : all) {
            boolean isCustomLink = s.source == TileSource.CUSTOM_LINKS;
            boolean keep =
                    (displayMode == NtpUtil.TOP_SITES_MODE_SHORTCUTS)
                            ? isCustomLink
                            : !isCustomLink;
            if (keep) {
                result.add(s);
            }
        }
        return result;
    }

    private void onModeChanged() {
        // Skip if the bridge hasn't delivered its first batch of suggestions yet:
        // mCachedSuggestions
        // is only an empty placeholder at that point, and notifying with it would flash an empty
        // list right before the real data (already correctly filtered) arrives.
        if (mOuterObserver == null || !mHasReceivedData) return;
        int mode = NtpUtil.getTopSitesDisplayMode();
        mOuterObserver.onSiteSuggestionsAvailable(
                /* isUserTriggered= */ true, filterForMode(mCachedSuggestions, mode));
    }

    private class FilteringObserver implements MostVisitedSites.Observer {
        @Override
        public void onSiteSuggestionsAvailable(
                boolean isUserTriggered, List<SiteSuggestion> suggestions) {
            mHasReceivedData = true;
            mCachedSuggestions = new ArrayList<>(suggestions);
            if (mOuterObserver != null) {
                mOuterObserver.onSiteSuggestionsAvailable(
                        isUserTriggered,
                        filterForMode(suggestions, NtpUtil.getTopSitesDisplayMode()));
            }
        }

        @Override
        public void onIconMadeAvailable(GURL siteUrl) {
            if (mOuterObserver != null) {
                mOuterObserver.onIconMadeAvailable(siteUrl);
            }
        }
    }
}

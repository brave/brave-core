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
 *   <li><b>Frequently visited mode</b> — passes only {@link TileSource#TOP_SITES} tiles.
 * </ul>
 *
 * <p>Filtering in Java rather than by changing {@code EnableTileTypes()} in C++ avoids the {@code
 * ShouldQueryTopSites()} fallback issue: when custom links are not yet initialised the C++ layer
 * always queries top-sites regardless of the tile-type options, so both modes would receive
 * identical top-sites data.
 *
 * <p>Mode is persisted in the Chrome profile pref {@code ntp.custom_links_visible} (written by
 * {@link NtpUtil#setTopSitesDisplayMode}) for Desktop sync compatibility, with a parallel write to
 * Android SharedPreferences that fires the local listener here.
 */
@NullMarked
public class BraveMostVisitedSites implements MostVisitedSites {

    private final MostVisitedSites mBridge;
    private MostVisitedSites.@Nullable Observer mOuterObserver;
    private List<SiteSuggestion> mCachedSuggestions;

    private final SharedPreferences.OnSharedPreferenceChangeListener mPrefListener;

    public BraveMostVisitedSites(Profile profile) {
        this(new MostVisitedSitesBridge(profile, /* enableCustomLinks= */ true));
    }

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

    @VisibleForTesting
    static List<SiteSuggestion> filterForMode(List<SiteSuggestion> all, int displayMode) {
        int targetSource =
                (displayMode == NtpUtil.TOP_SITES_MODE_SHORTCUTS)
                        ? TileSource.CUSTOM_LINKS
                        : TileSource.TOP_SITES;
        List<SiteSuggestion> result = new ArrayList<>(all.size());
        for (SiteSuggestion s : all) {
            if (s.source == targetSource) {
                result.add(s);
            }
        }
        return result;
    }

    private void onModeChanged() {
        if (mOuterObserver == null) return;
        int mode = NtpUtil.getTopSitesDisplayMode();
        mOuterObserver.onSiteSuggestionsAvailable(
                /* isUserTriggered= */ true, filterForMode(mCachedSuggestions, mode));
    }

    private class FilteringObserver implements MostVisitedSites.Observer {
        @Override
        public void onSiteSuggestionsAvailable(
                boolean isUserTriggered, List<SiteSuggestion> suggestions) {
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

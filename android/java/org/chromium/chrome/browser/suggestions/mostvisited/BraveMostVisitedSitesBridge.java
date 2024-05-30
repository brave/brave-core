// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions.mostvisited;

import org.jni_zero.CalledByNative;
import org.jni_zero.NativeMethods;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.SiteSuggestion;
import org.chromium.chrome.browser.suggestions.tile.Tile;
import org.chromium.url.GURL;

import java.util.ArrayList;
import org.chromium.base.Log;
import java.util.List;
import org.chromium.chrome.browser.suggestions.mostvisited.MostVisitedSitesBridge;

/** Methods to bridge into native history to provide most recent urls, titles and thumbnails. */
public class BraveMostVisitedSitesBridge extends MostVisitedSitesBridge {
    public BraveMostVisitedSitesBridge(Profile profile) {
        super(profile);
    }

    public void enableCustomLinks(boolean isEnabled) {
        MostVisitedSitesBridgeJni.get()
                .enableCustomLinks(mNativeMostVisitedSitesBridge, isEnabled);
    }

    public void initializeCustomLinks() {
        MostVisitedSitesBridgeJni.get()
                .initializeCustomLinks(mNativeMostVisitedSitesBridge);
    }

    public void uninitializeCustomLinks() {
        MostVisitedSitesBridgeJni.get()
                .uninitializeCustomLinks(mNativeMostVisitedSitesBridge);
    }

    public void addCustomLink(GURL url) {
        MostVisitedSitesBridgeJni.get()
                .addCustomLink(mNativeMostVisitedSitesBridge, url);
    }
}

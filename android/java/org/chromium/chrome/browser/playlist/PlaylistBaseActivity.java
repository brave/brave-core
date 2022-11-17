/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import org.chromium.base.BraveFeatureList;
import org.chromium.chrome.browser.PlaylistPageHandlerFactory;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.PageHandler;

public abstract class PlaylistBaseActivity
        extends AsyncInitializationActivity implements ConnectionErrorHandler {
    protected PageHandler mPlaylistPageHandler;

    @Override
    public void onConnectionError(MojoException e) {
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)) {
            mPlaylistPageHandler = null;
            initPlaylistPageHandler();
        }
    }

    private void initPlaylistPageHandler() {
        if (mPlaylistPageHandler != null) {
            return;
        }

        mPlaylistPageHandler =
                PlaylistPageHandlerFactory.getInstance().getPlaylistPageHandler(this);
    }

    public PageHandler getPlaylistPageHandler() {
        return mPlaylistPageHandler;
    }

    // @Override
    // public boolean onOptionsItemSelected(MenuItem item) {
    //     switch (item.getItemId()) {
    //         case android.R.id.home:
    //             finish();
    //             return true;
    //     }
    //     return super.onOptionsItemSelected(item);
    // }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)) {
            initPlaylistPageHandler();
        }
    }

    @Override
    public void onDestroy() {
        if (mPlaylistPageHandler != null) mPlaylistPageHandler.close();
        super.onDestroy();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }
}

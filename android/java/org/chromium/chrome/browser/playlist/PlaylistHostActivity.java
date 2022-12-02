/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import android.content.Intent;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;
import androidx.lifecycle.ViewModelProvider;

import com.brave.playlist.PlaylistViewModel;
import com.brave.playlist.fragment.PlaylistFragment;
import com.brave.playlist.listener.PlaylistOptionsListener;
import com.brave.playlist.model.PlaylistOptionsModel;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.playlist.PlaylistBaseActivity;
import org.chromium.chrome.browser.playlist.PlaylistUtils;
import org.chromium.playlist.mojom.PlaylistItem;

public class PlaylistHostActivity extends PlaylistBaseActivity implements PlaylistOptionsListener {
    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_playlist_host);
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        String playlistName = PlaylistUtils.DEFAULT_PLAYLIST;
        if (getIntent() != null) {
            playlistName = getIntent().getStringExtra(PlaylistUtils.PLAYLIST_NAME);
        }
        PlaylistViewModel viewModel =
                new ViewModelProvider(PlaylistHostActivity.this).get(PlaylistViewModel.class);
        if (mPlaylistPageHandler != null) {
            mPlaylistPageHandler.getPlaylist(playlistName, playlist -> {
                JSONObject playlistJsonObject = new JSONObject();
                try {
                    playlistJsonObject.put("id", playlist.id);
                    playlistJsonObject.put("name", playlist.name);
                    JSONArray playlistItemsJsonArray = new JSONArray();
                    for (PlaylistItem playlistItem : playlist.items) {
                        JSONObject playlistItemObject = new JSONObject();
                        playlistItemObject.put("id", playlistItem.id);
                        playlistItemObject.put("name", playlistItem.name);
                        playlistItemObject.put("page_source", playlistItem.pageSource.url);
                        playlistItemObject.put("media_path", playlistItem.mediaPath.url);
                        playlistItemObject.put("thumbnail_path", playlistItem.thumbnailPath.url);
                        playlistItemObject.put("cached", playlistItem.cached);
                        playlistItemObject.put("author", playlistItem.author);
                        playlistItemObject.put("duration", playlistItem.duration);
                        playlistItemsJsonArray.put(playlistItemObject);
                    }
                    playlistJsonObject.put("items", playlistItemsJsonArray);
                    Log.e(PlaylistUtils.TAG, playlistJsonObject.toString());
                } catch (JSONException e) {
                    Log.e(PlaylistUtils.TAG, "PlaylistHostActivity -> JSONException error " + e);
                }

                viewModel.setPlaylistData(playlistJsonObject.toString());

                PlaylistFragment playlistFragment = new PlaylistFragment();
                playlistFragment.setPlaylistOptionsListener(this);
                getSupportFragmentManager()
                        .beginTransaction()
                        .replace(R.id.fragment_container_view_tag, playlistFragment)
                        .commit();
            });
        }
    }

    @Override
    public void onOptionClicked(PlaylistOptionsModel playlistOptionsModel) {
        Log.e(PlaylistUtils.TAG,
                "PlaylistOptionsModel type : " + playlistOptionsModel.getOptionType());
    }
}

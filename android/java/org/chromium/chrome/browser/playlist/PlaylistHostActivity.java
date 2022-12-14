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
import com.brave.playlist.enums.PlaylistOptions;
import com.brave.playlist.fragment.AllPlaylistFragment;
import com.brave.playlist.fragment.PlaylistFragment;
import com.brave.playlist.listener.PlaylistOptionsListener;
import com.brave.playlist.model.MediaModel;
import com.brave.playlist.model.PlaylistModel;
import com.brave.playlist.model.PlaylistOptionsModel;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.playlist.PlaylistBaseActivity;
import org.chromium.chrome.browser.playlist.PlaylistUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.playlist.mojom.Playlist;
import org.chromium.playlist.mojom.PlaylistItem;

public class PlaylistHostActivity extends PlaylistBaseActivity implements PlaylistOptionsListener {
    private PlaylistViewModel playlistViewModel;
    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_playlist_host);
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        playlistViewModel =
                new ViewModelProvider(PlaylistHostActivity.this).get(PlaylistViewModel.class);

        playlistViewModel.getCreatePlaylistOption().observe(PlaylistHostActivity.this, newName -> {
            if (mPlaylistPageHandler != null) {
                Playlist playlist = new Playlist();
                playlist.name = newName;
                playlist.items = new PlaylistItem[0];
                Log.e(PlaylistUtils.TAG, "Name : " + playlist.name);
                mPlaylistPageHandler.createPlaylist(playlist);
                Log.e(PlaylistUtils.TAG, "after Name : " + playlist.name);
                openAllPlaylists();
            }
        });

        playlistViewModel.getPlaylistToOpen().observe(PlaylistHostActivity.this, playlistId -> {
            if (mPlaylistPageHandler != null) {
                openPlaylist(playlistId, true);
            }
        });

        playlistViewModel.getDeletePlaylistItems().observe(
                PlaylistHostActivity.this, playlistItems -> {
                    if (mPlaylistPageHandler != null) {
                        for (MediaModel playlistItem : playlistItems.getItems()) {
                            mPlaylistPageHandler.removeItemFromPlaylist(
                                    playlistItems.getId(), playlistItem.getId());
                        }
                        openPlaylist(playlistItems.getId(), false);
                    }
                });

        if (getIntent() != null && getIntent().getStringExtra(PlaylistUtils.PLAYLIST_ID) != null) {
            String playlistId = getIntent().getStringExtra(PlaylistUtils.PLAYLIST_ID);
            if (playlistId.equals("all")) {
                openAllPlaylists();
            } else {
                openPlaylist(playlistId, true);
            }
        }
    }

    private void openPlaylist(String playlistId, boolean recreateFragment) {
        if (mPlaylistPageHandler != null) {
            mPlaylistPageHandler.getPlaylist(playlistId, playlist -> {
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
                        playlistItemObject.put("media_src", playlistItem.mediaSrc.url);
                        playlistItemObject.put("thumbnail_path", playlistItem.thumbnailPath.url);
                        playlistItemObject.put("cached", playlistItem.cached);
                        playlistItemObject.put("author", playlistItem.author);
                        playlistItemObject.put("duration", playlistItem.duration);
                        playlistItemsJsonArray.put(playlistItemObject);
                    }
                    playlistJsonObject.put("items", playlistItemsJsonArray);
                    if (playlistViewModel != null) {
                        playlistViewModel.setPlaylistData(playlistJsonObject.toString(2));
                    }

                    if (recreateFragment) {
                        PlaylistFragment playlistFragment = new PlaylistFragment();
                        playlistFragment.setPlaylistOptionsListener(this);
                        getSupportFragmentManager()
                                .beginTransaction()
                                .replace(R.id.fragment_container_view_tag, playlistFragment)
                                .commit();
                    }
                } catch (JSONException e) {
                    Log.e(PlaylistUtils.TAG, "PlaylistHostActivity -> JSONException error " + e);
                }
            });
        }
    }

    private void openAllPlaylists() {
        if (mPlaylistPageHandler != null) {
            mPlaylistPageHandler.getAllPlaylists(playlists -> {
                try {
                    JSONArray playlistsJsonArray = new JSONArray();
                    for (Playlist playlist : playlists) {
                        JSONObject playlistJsonObject = new JSONObject();

                        playlistJsonObject.put("id", playlist.id);
                        playlistJsonObject.put("name", playlist.name);
                        JSONArray playlistItemsJsonArray = new JSONArray();
                        for (PlaylistItem playlistItem : playlist.items) {
                            JSONObject playlistItemObject = new JSONObject();
                            playlistItemObject.put("id", playlistItem.id);
                            playlistItemObject.put("name", playlistItem.name);
                            playlistItemObject.put("page_source", playlistItem.pageSource.url);
                            playlistItemObject.put("media_path", playlistItem.mediaPath.url);
                            playlistItemObject.put("media_src", playlistItem.mediaSrc.url);
                            playlistItemObject.put(
                                    "thumbnail_path", playlistItem.thumbnailPath.url);
                            playlistItemObject.put("cached", playlistItem.cached);
                            playlistItemObject.put("author", playlistItem.author);
                            playlistItemObject.put("duration", playlistItem.duration);
                            playlistItemsJsonArray.put(playlistItemObject);
                        }
                        playlistJsonObject.put("items", playlistItemsJsonArray);
                        Log.e(PlaylistUtils.TAG, "playlistJsonObject : " + playlistJsonObject);
                        playlistsJsonArray.put(playlistJsonObject);
                    }
                    Log.e(PlaylistUtils.TAG, "playlistsJsonArray : " + playlistsJsonArray);
                    if (playlistViewModel != null) {
                        playlistViewModel.setAllPlaylistData(playlistsJsonArray.toString(2));
                    }

                    AllPlaylistFragment allPlaylistFragment = new AllPlaylistFragment();
                    // allPlaylistFragment.setPlaylistOptionsListener(this);
                    getSupportFragmentManager()
                            .beginTransaction()
                            .replace(R.id.fragment_container_view_tag, allPlaylistFragment)
                            .commit();
                } catch (JSONException e) {
                    Log.e(PlaylistUtils.TAG, "PlaylistHostActivity -> JSONException error " + e);
                }
            });
        }
    }

    @Override
    public void onOptionClicked(PlaylistOptionsModel playlistOptionsModel) {
        Log.e(PlaylistUtils.TAG,
                "PlaylistOptionsModel type : " + playlistOptionsModel.getOptionType());
        if (PlaylistOptions.DELETE_PLAYLIST == playlistOptionsModel.getOptionType()) {
            if (mPlaylistPageHandler != null && playlistOptionsModel.getPlaylistModel() != null) {
                mPlaylistPageHandler.removePlaylist(
                        playlistOptionsModel.getPlaylistModel().getId());
                finish();
            }
        }
    }
}

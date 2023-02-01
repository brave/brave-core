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
import com.brave.playlist.model.DownloadProgressModel;
import com.brave.playlist.model.MoveOrCopyModel;
import com.brave.playlist.model.PlaylistItemModel;
import com.brave.playlist.model.PlaylistItemOptionModel;
import com.brave.playlist.model.PlaylistModel;
import com.brave.playlist.model.PlaylistOptionsModel;
import com.brave.playlist.view.bottomsheet.MoveOrCopyToPlaylistBottomSheet;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.playlist.PlaylistBaseActivity;
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid;
import org.chromium.chrome.browser.playlist.PlaylistUtils;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistPreferences;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.Playlist;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistService;
import org.chromium.playlist.mojom.PlaylistServiceObserver;

public class PlaylistHostActivity extends AsyncInitializationActivity
        implements ConnectionErrorHandler, PlaylistOptionsListener, PlaylistServiceObserver {
    private PlaylistService mPlaylistService;
    private PlaylistViewModel mPlaylistViewModel;

    @Override
    public void onConnectionError(MojoException e) {
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && SharedPreferencesManager.getInstance().readBoolean(
                        BravePlaylistPreferences.PREF_ENABLE_PLAYLIST, true)) {
            mPlaylistService = null;
            initPlaylistService();
        }
    }

    private void initPlaylistService() {
        if (mPlaylistService != null) {
            return;
        }

        mPlaylistService = PlaylistServiceFactoryAndroid.getInstance().getPlaylistService(this);
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_playlist_host);
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)) {
            initPlaylistService();
            mPlaylistService.addObserver(this);
        }
        mPlaylistViewModel =
                new ViewModelProvider(PlaylistHostActivity.this).get(PlaylistViewModel.class);

        mPlaylistViewModel.getCreatePlaylistOption().observe(PlaylistHostActivity.this, newName -> {
            if (mPlaylistService != null) {
                Playlist playlist = new Playlist();
                playlist.name = newName;
                playlist.items = new PlaylistItem[0];
                Log.e(PlaylistUtils.TAG, "Name : " + playlist.name);
                mPlaylistService.createPlaylist(playlist, createdPlaylist -> {
                    mPlaylistService.setDefaultPlaylistId(createdPlaylist.id);
                });
                Log.e(PlaylistUtils.TAG, "after Name : " + playlist.name);
                loadAllPlaylists();
            }
        });

        mPlaylistViewModel.getRenamePlaylistOption().observe(
                PlaylistHostActivity.this, renamePlaylistModel -> {
                    if (mPlaylistService != null) {
                        mPlaylistService.renamePlaylist(renamePlaylistModel.getPlaylistId(),
                                renamePlaylistModel.getNewName(), updatedPlaylist -> {
                                    Log.e(PlaylistUtils.TAG,
                                            "after rename Name : " + updatedPlaylist.name);
                                    loadPlaylist(updatedPlaylist.id);
                                });
                    }
                });

        mPlaylistViewModel.getPlaylistToOpen().observe(PlaylistHostActivity.this, playlistId -> {
            if (mPlaylistService != null) {
                showPlaylist(playlistId, true);
            }
        });

        mPlaylistViewModel.getFetchPlaylistData().observe(PlaylistHostActivity.this, playlistId -> {
            if (mPlaylistService != null) {
                loadPlaylist(playlistId);
            }
        });

        mPlaylistViewModel.getReorderPlaylistItems().observe(
                PlaylistHostActivity.this, playlistItems -> {
                    if (mPlaylistService != null) {
                        for (int i = 0; i < playlistItems.size(); i++) {
                            PlaylistItemModel playlistItemModel = playlistItems.get(i);
                            mPlaylistService.reorderItemFromPlaylist(
                                    playlistItemModel.getPlaylistId(), playlistItemModel.getId(),
                                    (short) i);
                        }
                        if (playlistItems.size() > 0) {
                            loadPlaylist(playlistItems.get(0).getPlaylistId());
                        }
                    }
                });

        mPlaylistViewModel.getDeletePlaylistItems().observe(
                PlaylistHostActivity.this, playlistItems -> {
                    if (mPlaylistService != null) {
                        for (PlaylistItemModel playlistItem : playlistItems.getItems()) {
                            mPlaylistService.removeItemFromPlaylist(
                                    playlistItems.getId(), playlistItem.getId());
                        }
                        loadPlaylist(playlistItems.getId());
                    }
                });

        mPlaylistViewModel.getPlaylistOption().observe(
                PlaylistHostActivity.this, playlistOptionsModel -> {
                    if (mPlaylistService != null) {
                        PlaylistOptions option = playlistOptionsModel.getOptionType();
                        if (option == PlaylistOptions.EDIT_PLAYLIST) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.EDIT_PLAYLIST");
                        } else if (option == PlaylistOptions.RENAME_PLAYLIST) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.RENAME_PLAYLIST");
                        } else if (option == PlaylistOptions.REMOVE_PLAYLIST_OFFLINE_DATA) {
                            Log.e(PlaylistUtils.TAG,
                                    "PlaylistOptions.REMOVE_PLAYLIST_OFFLINE_DATA");
                            if (mPlaylistService != null
                                    && playlistOptionsModel.getPlaylistModel() != null) {
                                mPlaylistService.removeLocalDataForItemsInPlaylist(
                                        playlistOptionsModel.getPlaylistModel().getId());
                            }
                        } else if (option == PlaylistOptions.DOWNLOAD_PLAYLIST_FOR_OFFLINE_USE) {
                            Log.e(PlaylistUtils.TAG,
                                    "PlaylistOptions.DOWNLOAD_PLAYLIST_FOR_OFFLINE_USE");
                            if (mPlaylistService != null
                                    && playlistOptionsModel.getPlaylistModel() != null) {
                                for (PlaylistItemModel playlistItemModel :
                                        playlistOptionsModel.getPlaylistModel().getItems()) {
                                    mPlaylistService.recoverLocalDataForItem(
                                            playlistItemModel.getId());
                                }
                            }
                        } else if (option == PlaylistOptions.DELETE_PLAYLIST) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.DELETE_PLAYLIST");
                            if (mPlaylistService != null
                                    && playlistOptionsModel.getPlaylistModel() != null) {
                                mPlaylistService.removePlaylist(
                                        playlistOptionsModel.getPlaylistModel().getId());
                                loadAllPlaylists();
                            }
                        }
                    }
                });

        mPlaylistViewModel.getAllPlaylistOption().observe(
                PlaylistHostActivity.this, playlistOptionsModel -> {
                    if (mPlaylistService != null) {
                        PlaylistOptions option = playlistOptionsModel.getOptionType();
                        if (option == PlaylistOptions.REMOVE_ALL_OFFLINE_DATA) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.REMOVE_ALL_OFFLINE_DATA");
                            if (mPlaylistService != null
                                    && playlistOptionsModel.getAllPlaylistModels() != null) {
                                for (PlaylistModel playlistModel :
                                        playlistOptionsModel.getAllPlaylistModels()) {
                                    mPlaylistService.removeLocalDataForItemsInPlaylist(
                                            playlistModel.getId());
                                }
                            }
                        } else if (option
                                == PlaylistOptions.DOWNLOAD_ALL_PLAYLISTS_FOR_OFFLINE_USE) {
                            Log.e(PlaylistUtils.TAG,
                                    "PlaylistOptions.DOWNLOAD_ALL_PLAYLISTS_FOR_OFFLINE_USE");
                        }
                    }
                });

        mPlaylistViewModel.getPlaylistItemOption().observe(
                PlaylistHostActivity.this, playlistItemOption -> {
                    if (mPlaylistService != null) {
                        PlaylistOptions option = playlistItemOption.getOptionType();
                        if (option == PlaylistOptions.MOVE_PLAYLIST_ITEM) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.MOVE_PLAYLIST_ITEM");
                            showMoveOrCopyPlaylistBottomSheet(playlistItemOption);
                        } else if (option == PlaylistOptions.COPY_PLAYLIST_ITEM) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.COPY_PLAYLIST_ITEM");
                            showMoveOrCopyPlaylistBottomSheet(playlistItemOption);
                        } else if (option == PlaylistOptions.DELETE_ITEMS_OFFLINE_DATA) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.DELETE_ITEMS_OFFLINE_DATA");
                            mPlaylistService.removeLocalDataForItem(
                                    playlistItemOption.getPlaylistItemModel().getId());
                        } else if (option == PlaylistOptions.SHARE_PLAYLIST_ITEM) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.SHARE_PLAYLIST_ITEM");
                        } else if (option == PlaylistOptions.OPEN_IN_NEW_TAB) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.OPEN_IN_NEW_TAB");
                            openPlaylistInTab(false,
                                    playlistItemOption.getPlaylistItemModel().getPageSource());
                        } else if (option == PlaylistOptions.OPEN_IN_PRIVATE_TAB) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.OPEN_IN_PRIVATE_TAB");
                            openPlaylistInTab(true,
                                    playlistItemOption.getPlaylistItemModel().getPageSource());
                        } else if (option == PlaylistOptions.DELETE_PLAYLIST_ITEM) {
                            Log.e(PlaylistUtils.TAG, "PlaylistOptions.DELETE_PLAYLIST_ITEM");
                            mPlaylistService.removeItemFromPlaylist(
                                    playlistItemOption.getPlaylistId(),
                                    playlistItemOption.getPlaylistItemModel().getId());
                            loadPlaylist(playlistItemOption.getPlaylistId());
                        }
                    }
                });

        if (getIntent() != null && getIntent().getStringExtra(PlaylistUtils.PLAYLIST_ID) != null) {
            String playlistId = getIntent().getStringExtra(PlaylistUtils.PLAYLIST_ID);
            if (playlistId.equals("all")) {
                showAllPlaylistsFragment();
            } else {
                showPlaylist(playlistId, false);
            }
        }
    }

    private void loadPlaylist(String playlistId) {
        if (mPlaylistService != null) {
            mPlaylistService.getPlaylist(playlistId, playlist -> {
                JSONObject playlistJson = new JSONObject();
                try {
                    playlistJson.put("id", playlist.id);
                    playlistJson.put("name", playlist.name);
                    JSONArray playlistItemsJsonArray = new JSONArray();
                    for (PlaylistItem playlistItem : playlist.items) {
                        JSONObject playlistItemObject = new JSONObject();
                        playlistItemObject.put("id", playlistItem.id);
                        playlistItemObject.put("name", playlistItem.name);
                        playlistItemObject.put("page_source", playlistItem.pageSource.url);
                        playlistItemObject.put("media_path", playlistItem.mediaPath.url);
                        playlistItemObject.put("media_src", playlistItem.mediaSource.url);
                        playlistItemObject.put("thumbnail_path", playlistItem.thumbnailPath.url);
                        playlistItemObject.put("cached", playlistItem.cached);
                        playlistItemObject.put("author", playlistItem.author);
                        playlistItemObject.put("duration", playlistItem.duration);
                        playlistItemsJsonArray.put(playlistItemObject);
                    }
                    playlistJson.put("items", playlistItemsJsonArray);
                    // GsonBuilder gsonBuilder = new GsonBuilder();
                    // Gson gson = gsonBuilder.create();
                    // String playlistJson = gson.toJson(playlist);
                    Log.e(PlaylistUtils.TAG, "playlistJson :  " + playlistJson.toString(2));
                    if (mPlaylistViewModel != null) {
                        mPlaylistViewModel.setPlaylistData(playlistJson.toString(2));
                    }
                } catch (Exception e) {
                    Log.e(PlaylistUtils.TAG, "PlaylistHostActivity -> JSONException error " + e);
                }
            });
        }
    }

    private void showPlaylist(String playlistId, boolean shouldFallback) {
        loadPlaylist(playlistId);
        PlaylistFragment playlistFragment = new PlaylistFragment();
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction().replace(
                R.id.fragment_container_view_tag, playlistFragment);
        if (shouldFallback) {
            transaction.addToBackStack(AllPlaylistFragment.class.getSimpleName());
        }
        transaction.commit();
    }

    private void loadAllPlaylists() {
        if (mPlaylistService != null) {
            mPlaylistService.getAllPlaylists(playlists -> {
                try {
                    // GsonBuilder gsonBuilder = new GsonBuilder();
                    // Gson gson = gsonBuilder.create();
                    // String playlistsJson = gson.toJson(playlists);
                    // Log.e(PlaylistUtils.TAG, "loadAllPlaylists " + playlistsJson);
                    JSONArray playlistsJson = new JSONArray();
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
                            playlistItemObject.put("media_src", playlistItem.mediaSource.url);
                            playlistItemObject.put(
                                    "thumbnail_path", playlistItem.thumbnailPath.url);
                            playlistItemObject.put("cached", playlistItem.cached);
                            playlistItemObject.put("author", playlistItem.author);
                            playlistItemObject.put("duration", playlistItem.duration);
                            playlistItemsJsonArray.put(playlistItemObject);
                        }
                        playlistJsonObject.put("items", playlistItemsJsonArray);
                        Log.e(PlaylistUtils.TAG, "playlistJsonObject : " + playlistJsonObject);
                        playlistsJson.put(playlistJsonObject);
                    }
                    Log.e(PlaylistUtils.TAG, "playlistsJson : " + playlistsJson.toString(2));
                    if (mPlaylistViewModel != null) {
                        mPlaylistViewModel.setAllPlaylistData(playlistsJson.toString(2));
                    }
                } catch (Exception e) {
                    Log.e(PlaylistUtils.TAG, "PlaylistHostActivity -> JSONException error " + e);
                }
            });
        }
    }

    private void showAllPlaylistsFragment() {
        loadAllPlaylists();
        AllPlaylistFragment allPlaylistFragment = new AllPlaylistFragment();
        getSupportFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_container_view_tag, allPlaylistFragment)
                .commit();
    }

    private void showMoveOrCopyPlaylistBottomSheet(
            PlaylistItemOptionModel playlistItemOptionModel) {
        loadAllPlaylists();
        MoveOrCopyModel moveOrCopyModel =
                new MoveOrCopyModel(playlistItemOptionModel.getOptionType(),
                        playlistItemOptionModel.getPlaylistId(), "");
        new MoveOrCopyToPlaylistBottomSheet(moveOrCopyModel)
                .show(getSupportFragmentManager(), null);
    }

    @Override
    public void onOptionClicked(PlaylistOptionsModel playlistOptionsModel) {
        Log.e(PlaylistUtils.TAG,
                "PlaylistOptionsModel type : " + playlistOptionsModel.getOptionType());
        if (PlaylistOptions.DELETE_PLAYLIST == playlistOptionsModel.getOptionType()) {
            if (mPlaylistService != null && playlistOptionsModel.getPlaylistModel() != null) {
                mPlaylistService.removePlaylist(playlistOptionsModel.getPlaylistModel().getId());
                finish();
            }
        }
    }

    private void openPlaylistInTab(boolean isIncognito, String url) {
        finish();
        TabUtils.openUrlInNewTab(isIncognito, url);
    }

    @Override
    public void onEvent(int event) {
        Log.e(PlaylistUtils.TAG, "Event : " + event);
    }

    @Override
    public void onMediaFileDownloadProgressed(String id, long totalBytes, long receivedBytes,
            byte percentComplete, String timeRemaining) {
        Log.e(PlaylistUtils.TAG,
                "Id : " + id + "totalBytes : " + (totalBytes / 1024) + "KB "
                        + "receivedBytes : " + (receivedBytes / 1024) + "KB "
                        + "timeRemaining : " + timeRemaining);
        if (mPlaylistViewModel != null) {
            mPlaylistViewModel.updateDownloadProgress(new DownloadProgressModel(
                    id, totalBytes, receivedBytes, percentComplete, timeRemaining));
        }
    }

    @Override
    public void onDestroy() {
        if (mPlaylistService != null) {
            mPlaylistService.close();
        }
        super.onDestroy();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public void close() {}
}

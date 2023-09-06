/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import android.os.Bundle;
import android.text.TextUtils;

import androidx.fragment.app.FragmentTransaction;
import androidx.lifecycle.ViewModelProvider;

import com.brave.playlist.PlaylistViewModel;
import com.brave.playlist.enums.PlaylistEventEnum;
import com.brave.playlist.enums.PlaylistOptions;
import com.brave.playlist.fragment.AllPlaylistFragment;
import com.brave.playlist.fragment.PlaylistFragment;
import com.brave.playlist.listener.PlaylistOptionsListener;
import com.brave.playlist.model.DownloadProgressModel;
import com.brave.playlist.model.MoveOrCopyModel;
import com.brave.playlist.model.PlaylistEventModel;
import com.brave.playlist.model.PlaylistItemModel;
import com.brave.playlist.model.PlaylistModel;
import com.brave.playlist.model.PlaylistOptionsModel;
import com.brave.playlist.util.ConstantUtils;
import com.brave.playlist.util.PlaylistUtils;
import com.brave.playlist.view.bottomsheet.MoveOrCopyToPlaylistBottomSheet;

import org.chromium.base.BraveFeatureList;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistPreferences;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.Playlist;
import org.chromium.playlist.mojom.PlaylistEvent;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistService;

import java.util.ArrayList;
import java.util.List;

public class PlaylistHostActivity extends AsyncInitializationActivity
        implements ConnectionErrorHandler, PlaylistOptionsListener,
                   PlaylistServiceObserverImplDelegate {
    private static final String TAG = "BravePlaylist";
    private PlaylistService mPlaylistService;
    private PlaylistViewModel mPlaylistViewModel;
    private PlaylistServiceObserverImpl mPlaylistServiceObserver;

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
            mPlaylistServiceObserver = new PlaylistServiceObserverImpl(this);
            mPlaylistService.addObserver(mPlaylistServiceObserver);
        }
        mPlaylistViewModel =
                new ViewModelProvider(PlaylistHostActivity.this).get(PlaylistViewModel.class);

        mPlaylistViewModel.getCreatePlaylistOption().observe(
                PlaylistHostActivity.this, createPlaylistModel -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    Playlist playlist = new Playlist();
                    playlist.name = createPlaylistModel.getNewPlaylistId();
                    playlist.items = new PlaylistItem[0];
                    mPlaylistService.createPlaylist(playlist, createdPlaylist -> {
                        if (createPlaylistModel.isMoveOrCopy()) {
                            MoveOrCopyModel tempMoveOrCopyModel = PlaylistUtils.moveOrCopyModel;
                            PlaylistUtils.moveOrCopyModel =
                                    new MoveOrCopyModel(tempMoveOrCopyModel.getPlaylistOptions(),
                                            createdPlaylist.id, tempMoveOrCopyModel.getItems());
                            mPlaylistViewModel.performMoveOrCopy(PlaylistUtils.moveOrCopyModel);
                        }
                    });
                    loadAllPlaylists();
                });

        mPlaylistViewModel.getRenamePlaylistOption().observe(
                PlaylistHostActivity.this, renamePlaylistModel -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    mPlaylistService.renamePlaylist(renamePlaylistModel.getPlaylistId(),
                            renamePlaylistModel.getNewName(),
                            updatedPlaylist -> { loadPlaylist(updatedPlaylist.id); });
                });

        mPlaylistViewModel.getPlaylistToOpen().observe(
                PlaylistHostActivity.this, playlistId -> { showPlaylist(playlistId, true); });

        mPlaylistViewModel.getFetchPlaylistData().observe(PlaylistHostActivity.this, playlistId -> {
            if (mPlaylistService == null) {
                return;
            }
            if (playlistId.equals(ConstantUtils.ALL_PLAYLIST)) {
                loadAllPlaylists();
            } else {
                loadPlaylist(playlistId);
            }
        });

        mPlaylistViewModel.getReorderPlaylistItems().observe(
                PlaylistHostActivity.this, playlistItems -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    for (int i = 0; i < playlistItems.size(); i++) {
                        PlaylistItemModel playlistItemModel = playlistItems.get(i);
                        mPlaylistService.reorderItemFromPlaylist(playlistItemModel.getPlaylistId(),
                                playlistItemModel.getId(), (short) i, (result) -> {});
                    }
                    if (playlistItems.size() > 0) {
                        loadPlaylist(playlistItems.get(0).getPlaylistId());
                    }
                });

        mPlaylistViewModel.getDeletePlaylistItems().observe(
                PlaylistHostActivity.this, playlistItems -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    for (PlaylistItemModel playlistItem : playlistItems.getItems()) {
                        mPlaylistService.removeItemFromPlaylist(
                                playlistItems.getId(), playlistItem.getId());
                    }
                    loadPlaylist(playlistItems.getId());
                });

        mPlaylistViewModel.getMoveOrCopyItems().observe(
                PlaylistHostActivity.this, moveOrCopyModel -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    if (moveOrCopyModel.getPlaylistOptions() == PlaylistOptions.MOVE_PLAYLIST_ITEM
                            || moveOrCopyModel.getPlaylistOptions()
                                    == PlaylistOptions.MOVE_PLAYLIST_ITEMS) {
                        for (PlaylistItemModel playlistItem : moveOrCopyModel.getItems()) {
                            mPlaylistService.moveItem(playlistItem.getPlaylistId(),
                                    moveOrCopyModel.getToPlaylistId(), playlistItem.getId());
                        }
                        if (moveOrCopyModel.getItems().size() > 0) {
                            loadPlaylist(moveOrCopyModel.getItems().get(0).getPlaylistId());
                        }
                    } else {
                        String[] playlistIds = new String[moveOrCopyModel.getItems().size()];
                        for (int i = 0; i < moveOrCopyModel.getItems().size(); i++) {
                            playlistIds[i] = moveOrCopyModel.getItems().get(i).getId();
                        }
                        mPlaylistService.copyItemToPlaylist(
                                playlistIds, moveOrCopyModel.getToPlaylistId());
                    }
                });

        mPlaylistViewModel.getPlaylistOption().observe(
                PlaylistHostActivity.this, playlistOptionsModel -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    PlaylistOptions option = playlistOptionsModel.getOptionType();
                    if (option == PlaylistOptions.REMOVE_PLAYLIST_OFFLINE_DATA) {
                        if (playlistOptionsModel.getPlaylistModel() != null) {
                            mPlaylistService.removeLocalDataForItemsInPlaylist(
                                    playlistOptionsModel.getPlaylistModel().getId());
                        }
                    } else if (option == PlaylistOptions.DOWNLOAD_PLAYLIST_FOR_OFFLINE_USE) {
                        if (playlistOptionsModel.getPlaylistModel() != null) {
                            for (PlaylistItemModel playlistItemModel :
                                    playlistOptionsModel.getPlaylistModel().getItems()) {
                                mPlaylistService.recoverLocalDataForItem(playlistItemModel.getId(),
                                        true,
                                        playlistItem
                                        -> {

                                        });
                            }
                        }
                    } else if (option == PlaylistOptions.DELETE_PLAYLIST) {
                        if (playlistOptionsModel.getPlaylistModel() != null) {
                            mPlaylistService.removePlaylist(
                                    playlistOptionsModel.getPlaylistModel().getId());
                            loadAllPlaylists();
                        }
                    } else if (option == PlaylistOptions.MOVE_PLAYLIST_ITEMS) {
                        showMoveOrCopyPlaylistBottomSheet();
                    } else if (option == PlaylistOptions.COPY_PLAYLIST_ITEMS) {
                        showMoveOrCopyPlaylistBottomSheet();
                    }
                });

        mPlaylistViewModel.getAllPlaylistOption().observe(
                PlaylistHostActivity.this, playlistOptionsModel -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    PlaylistOptions option = playlistOptionsModel.getOptionType();
                    if (option == PlaylistOptions.REMOVE_ALL_OFFLINE_DATA) {
                        if (playlistOptionsModel.getAllPlaylistModels() != null) {
                            for (PlaylistModel playlistModel :
                                    playlistOptionsModel.getAllPlaylistModels()) {
                                mPlaylistService.removeLocalDataForItemsInPlaylist(
                                        playlistModel.getId());
                            }
                        }
                    } else if (option == PlaylistOptions.DOWNLOAD_ALL_PLAYLISTS_FOR_OFFLINE_USE) {
                        mPlaylistService.getAllPlaylists(playlists -> {
                            for (Playlist playlist : playlists) {
                                for (PlaylistItem playlistItem : playlist.items) {
                                    mPlaylistService.recoverLocalDataForItem(playlistItem.id, true,
                                            tempPlaylistItem
                                            -> {

                                            });
                                }
                            }
                        });
                    }
                });

        mPlaylistViewModel.getPlaylistItemOption().observe(
                PlaylistHostActivity.this, playlistItemOption -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    PlaylistOptions option = playlistItemOption.getOptionType();
                    if (option == PlaylistOptions.MOVE_PLAYLIST_ITEM) {
                        showMoveOrCopyPlaylistBottomSheet();
                    } else if (option == PlaylistOptions.COPY_PLAYLIST_ITEM) {
                        showMoveOrCopyPlaylistBottomSheet();
                    } else if (option == PlaylistOptions.DELETE_ITEMS_OFFLINE_DATA) {
                        mPlaylistService.removeLocalDataForItem(
                                playlistItemOption.getPlaylistItemModel().getId());
                        loadPlaylist(playlistItemOption.getPlaylistId());
                    } else if (option == PlaylistOptions.OPEN_IN_NEW_TAB) {
                        openPlaylistInTab(
                                false, playlistItemOption.getPlaylistItemModel().getPageSource());
                    } else if (option == PlaylistOptions.OPEN_IN_PRIVATE_TAB) {
                        openPlaylistInTab(
                                true, playlistItemOption.getPlaylistItemModel().getPageSource());
                    } else if (option == PlaylistOptions.DELETE_PLAYLIST_ITEM) {
                        mPlaylistService.removeItemFromPlaylist(playlistItemOption.getPlaylistId(),
                                playlistItemOption.getPlaylistItemModel().getId());
                        loadPlaylist(playlistItemOption.getPlaylistId());
                    } else if (option == PlaylistOptions.RECOVER_PLAYLIST_ITEM) {
                        mPlaylistService.recoverLocalDataForItem(
                                playlistItemOption.getPlaylistItemModel().getId(), true,
                                playlistItem -> {
                                    loadPlaylist(playlistItemOption.getPlaylistId());
                                });
                    }
                });

        if (getIntent() != null) {
            if (!TextUtils.isEmpty(getIntent().getAction())
                    && getIntent().getAction().equals(ConstantUtils.PLAYLIST_ACTION)
                    && !TextUtils.isEmpty(
                            getIntent().getStringExtra(ConstantUtils.CURRENT_PLAYLIST_ID))
                    && !TextUtils.isEmpty(
                            getIntent().getStringExtra(ConstantUtils.CURRENT_PLAYING_ITEM_ID))) {
                showPlaylistForPlayer(getIntent().getStringExtra(ConstantUtils.CURRENT_PLAYLIST_ID),
                        getIntent().getStringExtra(ConstantUtils.CURRENT_PLAYING_ITEM_ID));
            } else {
                String playlistId = getIntent().getStringExtra(ConstantUtils.PLAYLIST_ID);
                if (!TextUtils.isEmpty(playlistId)
                        && playlistId.equals(ConstantUtils.ALL_PLAYLIST)) {
                    showAllPlaylistsFragment();
                } else {
                    showPlaylist(playlistId, false);
                }
            }
        }
    }

    private void showPlaylist(String playlistId, boolean shouldFallback) {
        if (mPlaylistService == null) {
            return;
        }
        loadPlaylist(playlistId);
        PlaylistFragment playlistFragment = new PlaylistFragment();
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction().replace(
                R.id.fragment_container_view_tag, playlistFragment);
        if (shouldFallback) {
            transaction.addToBackStack(AllPlaylistFragment.class.getSimpleName());
        }
        transaction.commit();
    }

    private void showPlaylistForPlayer(String playlistId, String playlistItemId) {
        if (mPlaylistService == null) {
            return;
        }
        loadPlaylist(playlistId);
        PlaylistFragment playlistFragment = new PlaylistFragment();
        Bundle fragmentBundle = new Bundle();
        fragmentBundle.putString(ConstantUtils.CURRENT_PLAYING_ITEM_ID, playlistItemId);
        playlistFragment.setArguments(fragmentBundle);
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction().replace(
                R.id.fragment_container_view_tag, playlistFragment);
        transaction.commit();
    }

    private void loadPlaylist(String playlistId) {
        if (mPlaylistService == null) {
            return;
        }
        mPlaylistService.getPlaylist(playlistId, playlist -> {
            List<PlaylistItemModel> playlistItems = new ArrayList();
            for (PlaylistItem playlistItem : playlist.items) {
                PlaylistItemModel playlistItemModel = new PlaylistItemModel(playlistItem.id,
                        playlist.id, playlistItem.name, playlistItem.pageSource.url,
                        playlistItem.mediaPath.url, playlistItem.mediaSource.url,
                        playlistItem.thumbnailPath.url, playlistItem.author, playlistItem.duration,
                        playlistItem.lastPlayedPosition, playlistItem.cached, false, 0);
                playlistItems.add(playlistItemModel);
            }
            PlaylistModel playlistModel =
                    new PlaylistModel(playlist.id, playlist.name, playlistItems);
            if (mPlaylistViewModel != null) {
                mPlaylistViewModel.setPlaylistData(playlistModel);
            }
        });
    }

    private void loadAllPlaylists() {
        if (mPlaylistService == null) {
            return;
        }
        mPlaylistService.getAllPlaylists(playlists -> {
            List<PlaylistModel> allPlaylists = new ArrayList();
            for (Playlist playlist : playlists) {
                List<PlaylistItemModel> playlistItems = new ArrayList();
                for (PlaylistItem playlistItem : playlist.items) {
                    PlaylistItemModel playlistItemModel =
                            new PlaylistItemModel(playlistItem.id, playlist.id, playlistItem.name,
                                    playlistItem.pageSource.url, playlistItem.mediaPath.url,
                                    playlistItem.mediaSource.url, playlistItem.thumbnailPath.url,
                                    playlistItem.author, playlistItem.duration,
                                    playlistItem.lastPlayedPosition, playlistItem.cached, false, 0);
                    playlistItems.add(playlistItemModel);
                }
                PlaylistModel playlistModel =
                        new PlaylistModel(playlist.id, playlist.name, playlistItems);
                allPlaylists.add(playlistModel);
            }
            if (mPlaylistViewModel != null) {
                mPlaylistViewModel.setAllPlaylistData(allPlaylists);
            }
        });
    }

    private void showAllPlaylistsFragment() {
        AllPlaylistFragment allPlaylistFragment = new AllPlaylistFragment();
        getSupportFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_container_view_tag, allPlaylistFragment)
                .commit();
    }

    private void showMoveOrCopyPlaylistBottomSheet() {
        loadAllPlaylists();
        new MoveOrCopyToPlaylistBottomSheet().show(getSupportFragmentManager(), null);
    }

    @Override
    public void onOptionClicked(PlaylistOptionsModel playlistOptionsModel) {
        if (PlaylistOptions.DELETE_PLAYLIST == playlistOptionsModel.getOptionType()
                && mPlaylistService != null && playlistOptionsModel.getPlaylistModel() != null) {
            mPlaylistService.removePlaylist(playlistOptionsModel.getPlaylistModel().getId());
            finish();
        }
    }

    private void openPlaylistInTab(boolean isIncognito, String url) {
        finish();
        TabUtils.openUrlInNewTab(isIncognito, url);
    }

    @Override
    public void onEvent(int eventType, String playlistId) {
        if (mPlaylistViewModel == null) {
            return;
        }
        PlaylistEventEnum playlistEvent;
        switch (eventType) {
            case PlaylistEvent.ITEM_ADDED:
                playlistEvent = PlaylistEventEnum.kItemAdded;
                break;
            case PlaylistEvent.ITEM_THUMBNAIL_READY:
                playlistEvent = PlaylistEventEnum.kItemThumbnailReady;
                break;
            case PlaylistEvent.ITEM_THUMBNAIL_FAILED:
                playlistEvent = PlaylistEventEnum.kItemThumbnailFailed;
                break;
            case PlaylistEvent.ITEM_CACHED:
                playlistEvent = PlaylistEventEnum.kItemCached;
                break;
            case PlaylistEvent.ITEM_DELETED:
                playlistEvent = PlaylistEventEnum.kItemDeleted;
                break;
            case PlaylistEvent.ITEM_UPDATED:
                playlistEvent = PlaylistEventEnum.kItemUpdated;
                break;
            case PlaylistEvent.ITEM_MOVED:
                playlistEvent = PlaylistEventEnum.kItemMoved;
                break;
            case PlaylistEvent.ITEM_ABORTED:
                playlistEvent = PlaylistEventEnum.kItemAborted;
                break;
            case PlaylistEvent.ITEM_LOCAL_DATA_REMOVED:
                playlistEvent = PlaylistEventEnum.kItemLocalDataRemoved;
                break;
            case PlaylistEvent.LIST_CREATED:
                playlistEvent = PlaylistEventEnum.kListCreated;
                break;
            case PlaylistEvent.ALL_DELETED:
                playlistEvent = PlaylistEventEnum.kAllDeleted;
                break;
            default:
                playlistEvent = PlaylistEventEnum.kNone;
                break;
        }
        mPlaylistViewModel.updatePlaylistEvent(new PlaylistEventModel(playlistEvent, playlistId));
    }

    @Override
    public void onMediaFileDownloadProgressed(String id, long totalBytes, long receivedBytes,
            byte percentComplete, String timeRemaining) {
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
        if (mPlaylistServiceObserver != null) {
            mPlaylistServiceObserver.close();
            mPlaylistServiceObserver.destroy();
            mPlaylistServiceObserver = null;
        }
        super.onDestroy();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }
}

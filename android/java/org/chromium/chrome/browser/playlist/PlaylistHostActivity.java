/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;

import androidx.fragment.app.FragmentTransaction;
import androidx.lifecycle.ViewModelProvider;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate;
import org.chromium.chrome.browser.playlist.hls_content.HlsService;
import org.chromium.chrome.browser.playlist.hls_content.HlsServiceImpl;
import org.chromium.chrome.browser.playlist.kotlin.PlaylistViewModel;
import org.chromium.chrome.browser.playlist.kotlin.fragment.AllPlaylistFragment;
import org.chromium.chrome.browser.playlist.kotlin.fragment.PlaylistFragment;
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistOptionsListener;
import org.chromium.chrome.browser.playlist.kotlin.local_database.PlaylistRepository;
import org.chromium.chrome.browser.playlist.kotlin.model.HlsContentProgressModel;
import org.chromium.chrome.browser.playlist.kotlin.model.MoveOrCopyModel;
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemModel;
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistModel;
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOptionsModel;
import org.chromium.chrome.browser.playlist.kotlin.playback_service.VideoPlaybackService;
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils;
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils;
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.MoveOrCopyToPlaylistBottomSheet;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileProvider;
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
    private static final String TAG = "PlaylistHostActivity";
    private PlaylistService mPlaylistService;
    private PlaylistViewModel mPlaylistViewModel;
    private PlaylistServiceObserverImpl mPlaylistServiceObserver;

    @Override
    public void onConnectionError(MojoException e) {
        if (mPlaylistService != null) {
            mPlaylistService.close();
            mPlaylistService = null;
        }
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)) {
            initPlaylistService();
        }
    }

    private void initPlaylistService() {
        if (mPlaylistService != null) {
            mPlaylistService = null;
        }
        mPlaylistService =
                PlaylistServiceFactoryAndroid.getInstance()
                        .getPlaylistService(
                                getProfileProviderSupplier().get().getOriginalProfile(), this);
        addPlaylistObserver();
    }

    private void addPlaylistObserver() {
        mPlaylistServiceObserver = new PlaylistServiceObserverImpl(this);
        mPlaylistService.addObserver(mPlaylistServiceObserver);
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
        }
        mPlaylistViewModel =
                new ViewModelProvider(PlaylistHostActivity.this).get(PlaylistViewModel.class);

        mPlaylistViewModel
                .getCreatePlaylistOption()
                .observe(
                        PlaylistHostActivity.this,
                        createPlaylistModel -> {
                            if (mPlaylistService == null) {
                                return;
                            }
                            Playlist playlist = new Playlist();
                            playlist.name = createPlaylistModel.getNewPlaylistId();
                            playlist.items = new PlaylistItem[0];
                            mPlaylistService.createPlaylist(
                                    playlist,
                                    createdPlaylist -> {
                                        if (createPlaylistModel.isMoveOrCopy()
                                                && PlaylistUtils.moveOrCopyModel != null) {
                                            MoveOrCopyModel tempMoveOrCopyModel =
                                                    PlaylistUtils.moveOrCopyModel;
                                            PlaylistUtils.moveOrCopyModel =
                                                    new MoveOrCopyModel(
                                                            tempMoveOrCopyModel
                                                                    .getPlaylistOptionsEnum(),
                                                            createdPlaylist.id,
                                                            tempMoveOrCopyModel.getPlaylistItems());
                                            mPlaylistViewModel.performMoveOrCopy(
                                                    PlaylistUtils.moveOrCopyModel);
                                        }
                                    });
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
                });

        mPlaylistViewModel
                .getDeletePlaylistItems()
                .observe(
                        PlaylistHostActivity.this,
                        playlistItems -> {
                            if (mPlaylistService == null) {
                                return;
                            }
                            for (PlaylistItemModel playlistItem : playlistItems.getItems()) {
                                deleteHLSContent(playlistItem.getId());
                                mPlaylistService.removeItemFromPlaylist(
                                        playlistItems.getId(), playlistItem.getId());
                            }
                            if (playlistItems.getItems().size() > 0) {
                                loadPlaylist(playlistItems.getItems().get(0).getPlaylistId());
                            }
                        });

        mPlaylistViewModel
                .getMoveOrCopyItems()
                .observe(
                        PlaylistHostActivity.this,
                        moveOrCopyModel -> {
                            if (mPlaylistService == null) {
                                return;
                            }
                            if (moveOrCopyModel.getPlaylistOptionsEnum()
                                            == PlaylistModel.PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM
                                    || moveOrCopyModel.getPlaylistOptionsEnum()
                                            == PlaylistModel.PlaylistOptionsEnum
                                                    .MOVE_PLAYLIST_ITEMS) {
                                for (PlaylistItemModel playlistItem :
                                        moveOrCopyModel.getPlaylistItems()) {
                                    mPlaylistService.moveItem(
                                            playlistItem.getPlaylistId(),
                                            moveOrCopyModel.getToPlaylistId(),
                                            playlistItem.getId());
                                }
                            } else {
                                String[] playlistItemIds =
                                        new String[moveOrCopyModel.getPlaylistItems().size()];
                                for (int i = 0;
                                        i < moveOrCopyModel.getPlaylistItems().size();
                                        i++) {
                                    playlistItemIds[i] =
                                            moveOrCopyModel.getPlaylistItems().get(i).getId();
                                }
                                mPlaylistService.copyItemToPlaylist(
                                        playlistItemIds, moveOrCopyModel.getToPlaylistId());
                            }
                            if (moveOrCopyModel.getPlaylistItems().size() > 0) {
                                loadPlaylist(
                                        moveOrCopyModel.getPlaylistItems().get(0).getPlaylistId());
                            }
                        });

        mPlaylistViewModel
                .getPlaylistOption()
                .observe(
                        PlaylistHostActivity.this,
                        playlistOptionsModel -> {
                            if (mPlaylistService == null) {
                                return;
                            }
                            PlaylistModel.PlaylistOptionsEnum option =
                                    playlistOptionsModel.getOptionType();
                            if (option == PlaylistModel.PlaylistOptionsEnum.DELETE_PLAYLIST) {
                                if (playlistOptionsModel.getPlaylistModel() != null) {
                                    mPlaylistService.removePlaylist(
                                            playlistOptionsModel.getPlaylistModel().getId());
                                }
                            } else if (option
                                    == PlaylistModel.PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS) {
                                showMoveOrCopyPlaylistBottomSheet();
                            } else if (option
                                    == PlaylistModel.PlaylistOptionsEnum.COPY_PLAYLIST_ITEMS) {
                                showMoveOrCopyPlaylistBottomSheet();
                            }
                        });

        mPlaylistViewModel
                .getPlaylistItemOption()
                .observe(
                        PlaylistHostActivity.this,
                        playlistItemOption -> {
                            if (mPlaylistService == null) {
                                return;
                            }
                            PlaylistModel.PlaylistOptionsEnum option =
                                    playlistItemOption.getOptionType();
                            if (option == PlaylistModel.PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM) {
                                showMoveOrCopyPlaylistBottomSheet();
                            } else if (option
                                    == PlaylistModel.PlaylistOptionsEnum.COPY_PLAYLIST_ITEM) {
                                showMoveOrCopyPlaylistBottomSheet();
                            } else if (option
                                    == PlaylistModel.PlaylistOptionsEnum
                                            .DELETE_ITEMS_OFFLINE_DATA) {
                                mPlaylistService.removeLocalDataForItem(
                                        playlistItemOption.getPlaylistItemModel().getId());
                                // Playlist item will be updated based on event
                            } else if (option
                                    == PlaylistModel.PlaylistOptionsEnum.OPEN_IN_NEW_TAB) {
                                openPlaylistInTab(
                                        false,
                                        playlistItemOption.getPlaylistItemModel().getPageSource());
                            } else if (option
                                    == PlaylistModel.PlaylistOptionsEnum.OPEN_IN_PRIVATE_TAB) {
                                openPlaylistInTab(
                                        true,
                                        playlistItemOption.getPlaylistItemModel().getPageSource());
                            } else if (option
                                    == PlaylistModel.PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM) {
                                deleteHLSContent(playlistItemOption.getPlaylistItemModel().getId());
                                mPlaylistService.removeItemFromPlaylist(
                                        playlistItemOption.getPlaylistId(),
                                        playlistItemOption.getPlaylistItemModel().getId());
                                loadPlaylist(playlistItemOption.getPlaylistId());
                            }
                        });

        if (getIntent() != null) {
            if (!TextUtils.isEmpty(getIntent().getAction())
                    && getIntent().getAction().equals(ConstantUtils.PLAYLIST_ACTION)) {
                showPlaylistForPlayer();
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

    private void showPlaylistForPlayer() {
        if (mPlaylistService == null
                && TextUtils.isEmpty(VideoPlaybackService.Companion.getCurrentPlaylistId())) {
            return;
        }
        loadPlaylist(VideoPlaybackService.Companion.getCurrentPlaylistId());
        PlaylistFragment playlistFragment = new PlaylistFragment();
        Bundle fragmentBundle = new Bundle();
        fragmentBundle.putBoolean(ConstantUtils.SHOULD_OPEN_PLAYER, true);
        playlistFragment.setArguments(fragmentBundle);
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction().replace(
                R.id.fragment_container_view_tag, playlistFragment);
        transaction.commit();
    }

    private void loadPlaylist(String playlistId) {
        if (mPlaylistService == null) {
            return;
        }
        mPlaylistService.getPlaylist(
                playlistId,
                playlist -> {
                    if (playlist == null) {
                        Log.d(TAG, "loadPlaylist is null from service");
                        return;
                    }
                    List<PlaylistItemModel> playlistItems = new ArrayList();
                    for (PlaylistItem playlistItem : playlist.items) {
                        PlaylistItemModel playlistItemModel =
                                new PlaylistItemModel(
                                        playlistItem.id,
                                        playlist.id,
                                        playlistItem.name,
                                        playlistItem.pageSource.url,
                                        playlistItem.mediaPath.url,
                                        playlistItem.hlsMediaPath.url,
                                        playlistItem.mediaSource.url,
                                        playlistItem.thumbnailPath.url,
                                        playlistItem.author,
                                        playlistItem.duration,
                                        playlistItem.lastPlayedPosition,
                                        playlistItem.mediaFileBytes,
                                        playlistItem.cached,
                                        false);
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
        mPlaylistService.getAllPlaylists(
                playlists -> {
                    List<PlaylistModel> allPlaylists = new ArrayList();
                    for (Playlist playlist : playlists) {
                        List<PlaylistItemModel> playlistItems = new ArrayList();
                        for (PlaylistItem playlistItem : playlist.items) {
                            PlaylistItemModel playlistItemModel =
                                    new PlaylistItemModel(
                                            playlistItem.id,
                                            playlist.id,
                                            playlistItem.name,
                                            playlistItem.pageSource.url,
                                            playlistItem.mediaPath.url,
                                            playlistItem.hlsMediaPath.url,
                                            playlistItem.mediaSource.url,
                                            playlistItem.thumbnailPath.url,
                                            playlistItem.author,
                                            playlistItem.duration,
                                            playlistItem.lastPlayedPosition,
                                            playlistItem.mediaFileBytes,
                                            playlistItem.cached,
                                            false);
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
    public void onPlaylistOptionClicked(PlaylistOptionsModel playlistOptionsModel) {
        if (PlaylistModel.PlaylistOptionsEnum.DELETE_PLAYLIST
                        == playlistOptionsModel.getOptionType()
                && mPlaylistService != null
                && playlistOptionsModel.getPlaylistModel() != null) {
            mPlaylistService.removePlaylist(playlistOptionsModel.getPlaylistModel().getId());
            finish();
        }
    }

    private void openPlaylistInTab(boolean isIncognito, String url) {
        finish();
        TabUtils.openUrlInNewTab(isIncognito, url);
    }

    private void updatePlaylistItem(String playlistId, PlaylistItem playlistItem) {
        if (mPlaylistViewModel == null) {
            return;
        }
        PlaylistItemModel playlistItemModel =
                new PlaylistItemModel(
                        playlistItem.id,
                        playlistId,
                        playlistItem.name,
                        playlistItem.pageSource.url,
                        playlistItem.mediaPath.url,
                        playlistItem.hlsMediaPath.url,
                        playlistItem.mediaSource.url,
                        playlistItem.thumbnailPath.url,
                        playlistItem.author,
                        playlistItem.duration,
                        playlistItem.lastPlayedPosition,
                        (long) playlistItem.mediaFileBytes,
                        playlistItem.cached,
                        false);
        mPlaylistViewModel.updatePlaylistItem(playlistItemModel);
    }

    private void deleteHLSContent(String playlistItemId) {
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> {
                    PlaylistRepository playlistRepository =
                            new PlaylistRepository(PlaylistHostActivity.this);
                    if (playlistRepository != null
                            && playlistRepository.isHlsContentQueueModelExists(playlistItemId)) {
                        playlistRepository.deleteHlsContentQueueModel(playlistItemId);
                    }
                    if (HlsServiceImpl.currentDownloadingPlaylistItemId.equals(playlistItemId)) {
                        HlsServiceImpl.currentDownloadingPlaylistItemId = "";
                        mPlaylistService.cancelQuery(playlistItemId);
                        stopService(new Intent(PlaylistHostActivity.this, HlsService.class));
                        PlaylistUtils.checkAndStartHlsDownload(PlaylistHostActivity.this);
                    }
                });
    }

    @Override
    public void onItemCached(PlaylistItem playlistItem) {
        loadPlaylist(ConstantUtils.DEFAULT_PLAYLIST);
    }

    @Override
    public void onItemUpdated(PlaylistItem playlistItem) {
        loadPlaylist(ConstantUtils.DEFAULT_PLAYLIST);
    }

    @Override
    public void onPlaylistUpdated(Playlist playlist) {
        // Used only for reorder items
        loadPlaylist(playlist.id);
    }

    @Override
    public void onEvent(int eventType, String id) {
        if (eventType == PlaylistEvent.LIST_CREATED || eventType == PlaylistEvent.LIST_REMOVED) {
            loadAllPlaylists();
        }
    }

    @Override
    public void onMediaFileDownloadProgressed(String id, long totalBytes, long receivedBytes,
            byte percentComplete, String timeRemaining) {
        if (mPlaylistViewModel != null) {
            mPlaylistViewModel.updateDownloadProgress(
                    new HlsContentProgressModel(
                            id, totalBytes, receivedBytes, "" + percentComplete));
        }
    }

    @Override
    public void onDestroy() {
        if (mPlaylistService != null) {
            mPlaylistService.close();
            mPlaylistService = null;
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

    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import static com.google.android.exoplayer2.util.Assertions.checkNotNull;
import static com.google.android.exoplayer2.util.Util.castNonNull;

import static java.lang.Math.min;

import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;

import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentTransaction;
import androidx.lifecycle.ViewModelProvider;

import com.brave.playlist.PlaylistViewModel;
import com.brave.playlist.enums.PlaylistItemEventEnum;
import com.brave.playlist.enums.PlaylistOptionsEnum;
import com.brave.playlist.fragment.AllPlaylistFragment;
import com.brave.playlist.fragment.PlaylistFragment;
import com.brave.playlist.listener.PlaylistOptionsListener;
import com.brave.playlist.model.DownloadProgressModel;
import com.brave.playlist.model.MoveOrCopyModel;
import com.brave.playlist.model.PlaylistItemEventModel;
import com.brave.playlist.model.PlaylistItemModel;
import com.brave.playlist.model.PlaylistModel;
import com.brave.playlist.model.PlaylistOptionsModel;
import com.brave.playlist.util.ConstantUtils;
import com.brave.playlist.util.HLSParsingUtil;
import com.brave.playlist.util.MediaUtils;
import com.brave.playlist.util.PlaylistUtils;
import com.brave.playlist.view.bottomsheet.MoveOrCopyToPlaylistBottomSheet;
import com.google.android.exoplayer2.C;
import com.google.android.exoplayer2.PlaybackException;
import com.google.android.exoplayer2.source.hls.playlist.HlsMediaPlaylist.Segment;
import com.google.android.exoplayer2.upstream.BaseDataSource;
import com.google.android.exoplayer2.upstream.DataSource;
import com.google.android.exoplayer2.upstream.DataSourceException;
import com.google.android.exoplayer2.upstream.DataSpec;
import com.google.android.exoplayer2.upstream.HttpDataSource;
import com.google.android.exoplayer2.upstream.HttpUtil;
import com.google.android.exoplayer2.upstream.TransferListener;
import com.google.android.exoplayer2.util.UriUtil;
import com.google.android.exoplayer2.util.Util;
import com.google.common.base.Predicate;
import com.google.common.collect.ImmutableMap;
import com.google.common.net.HttpHeaders;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate;
import org.chromium.chrome.browser.playlist.PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistPreferences;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.playlist.mojom.Playlist;
import org.chromium.playlist.mojom.PlaylistEvent;
import org.chromium.playlist.mojom.PlaylistItem;
import org.chromium.playlist.mojom.PlaylistService;
import org.chromium.playlist.mojom.PlaylistStreamingObserver;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;

public class PlaylistHostActivity extends AsyncInitializationActivity
        implements ConnectionErrorHandler, PlaylistOptionsListener,
                   PlaylistServiceObserverImplDelegate {
    private static final String TAG = "BravePlaylist";
    public static PlaylistService mPlaylistService;
    private PlaylistViewModel mPlaylistViewModel;
    private PlaylistServiceObserverImpl mPlaylistServiceObserver;
    Queue<Segment> segmentsQueue = new LinkedList<Segment>();
    // private FileOutputStream fileOutputStream = null;
    // private PlaylistStreamingObserverImpl mPlaylistStreamingObserver;

    @Override
    public void onConnectionError(MojoException e) {
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePlaylistPreferences.PREF_ENABLE_PLAYLIST, true)) {
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
            // mPlaylistStreamingObserver = new PlaylistStreamingObserverImpl(this);
            mPlaylistService.addObserver(mPlaylistServiceObserver);
            // mPlaylistService.addObserverForStreaming(mPlaylistStreamingObserver);
        }
        // if (mPlaylistService != null) {
        //     mPlaylistService.queryPrompt("https://rr6---sn-8qu-t0ael.googlevideo.com/videoplayback?expire=1691067483&ei=-0_LZPHhNuu_sfIP5firuAg&ip=23.233.146.226&id=o-AExLNlXx85_Q4tcHnUF6MGOc_T66iJJf1OaHS0gjPRn3&itag=18&source=youtube&requiressl=yes&mh=-i&mm=31%2C29&mn=sn-8qu-t0ael%2Csn-t0a7ln7d&ms=au%2Crdu&mv=m&mvi=6&pl=19&initcwndbps=2480000&spc=UWF9f_yJ8R6zaVHVRh7UWka1Z0gG9C0wjTxsbIcl9Q&vprv=1&svpuc=1&mime=video%2Fmp4&gir=yes&clen=14278085&ratebypass=yes&dur=314.049&lmt=1690412588093133&mt=1691045367&fvip=4&fexp=24007246%2C24363392&c=MWEB&txp=5538434&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cspc%2Cvprv%2Csvpuc%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AOq0QJ8wRAIgP_0mpb4UkaWYq1C7-4oQkbrwZysBqsqw6vjd30-DK5ICICTSjSeKzCfQrnzZeEAmHVxvGJI3iWfEHbSwWlQ-G4xD&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=AG3C_xAwRgIhALQm0pOXEIjgjdFHih-hyWFGwUvjyfd9uZDbXwzcecpOAiEA1disYFd9JOfd2nmMArSVz7_6u-BUN9dUmTUqsYGSdPk%3D&cpn=Coffzlw1c5mzHuJT&cver=2.20230802.00.00&ptk=youtube_single&oid=PAIgdgLHpTFS4TA-jk_AHA&ptchn=8p1vwvWtl6T73JiExfWs1g&pltype=content",
        //     "GET");
        // }
        mPlaylistViewModel =
                new ViewModelProvider(PlaylistHostActivity.this).get(PlaylistViewModel.class);

        mPlaylistViewModel.getOpenPlaylistStream().observe(
                PlaylistHostActivity.this, playlistItemModel -> {
                    String extension = playlistItemModel.getMediaPath().substring(
                            playlistItemModel.getMediaPath().lastIndexOf("."));
                    Log.e("data_source", "extension : " + extension);
                    // if (playlistItemModel.isCached() && extension == ".m3u8") {
                    final String manifestUrl = HLSParsingUtil.getContentManifestUrl(
                            PlaylistHostActivity.this, playlistItemModel);
                    if (mPlaylistService != null) {
                        // Log.e("data_source", "queryPrompt : "+manifestUrl);
                        mPlaylistService.queryPrompt(manifestUrl, "GET");
                        PlaylistStreamingObserver playlistStreamingObserverImpl =
                                new PlaylistStreamingObserver() {
                                    @Override
                                    public void onResponseStarted(String url, long contentLength) {
                                        try {
                                            Log.e("data_source",
                                                    "onResponseStarted : " + manifestUrl);
                                            File mediaFile = new File(
                                                    MediaUtils
                                                            .getTempManifestFile(
                                                                    PlaylistHostActivity.this)
                                                            .getAbsolutePath());
                                            if (mediaFile.exists()) {
                                                mediaFile.delete();
                                            }
                                        } catch (Exception ex) {
                                            ex.printStackTrace();
                                        }
                                    }

                                    @Override
                                    public void onDataReceived(byte[] response) {
                                        // Log.e("data_source",
                                        //         "onDataReceived : " + response.length);
                                        try {
                                            MediaUtils.writeToFile(response,
                                                    MediaUtils
                                                            .getTempManifestFile(
                                                                    PlaylistHostActivity.this)
                                                            .getAbsolutePath());
                                        } catch (Exception ex) {
                                            ex.printStackTrace();
                                        }
                                    }

                                    @Override
                                    public void onDataCompleted() {
                                        // Log.e("data_source", "onDataCompleted : "+manifestUrl);
                                        try {
                                            List<Segment> segments =
                                                    HLSParsingUtil.getContentSegments(
                                                            MediaUtils
                                                                    .getTempManifestFile(
                                                                            PlaylistHostActivity
                                                                                    .this)
                                                                    .getAbsolutePath(),
                                                            manifestUrl);
                                            for (Segment segment : segments) {
                                                // Log.e("data_source",
                                                //         "segment.url : " +
                                                //         UriUtil.resolve(manifestUrl,
                                                //         segment.url));
                                                segmentsQueue.add(segment);
                                            }
                                            mPlaylistService.clearObserverForStreaming();
                                            Segment segment = segmentsQueue.poll();
                                            if (segment != null) {
                                                downalodHLSFile(manifestUrl, segment);
                                            }
                                        } catch (Exception ex) {
                                            ex.printStackTrace();
                                        }
                                    }

                                    @Override
                                    public void close() {}

                                    @Override
                                    public void onConnectionError(MojoException e) {}
                                };

                        mPlaylistService.addObserverForStreaming(playlistStreamingObserverImpl);
                    }

                    // } else {
                    //     if (mPlaylistService != null) {
                    //         Log.e("data_source", "queryPrompt");
                    //         mPlaylistService.queryPrompt(playlistItemModel.getMediaSrc(), "GET");
                    //         if (mPlaylistViewModel == null) {
                    //             return;
                    //         }
                    //         Log.e("data_source", "onResponseStarted");
                    //         mPlaylistViewModel.setPlaylistItemToOpen(playlistItemModel);

                    //         PlaylistStreamingObserver playlistStreamingObserverImpl =
                    //                 new PlaylistStreamingObserver() {
                    //                     @Override
                    //                     public void onResponseStarted(
                    //                             String url, long contentLength) {
                    //                         try {
                    //                             File mediaFile = new File(
                    //                                     MediaUtils
                    //                                             .getTempFile(
                    //                                                     PlaylistHostActivity.this)
                    //                                             .getAbsolutePath());
                    //                             if (mediaFile.exists()) {
                    //                                 mediaFile.delete();
                    //                             }
                    //                             // fileOutputStream = new
                    //                             // FileOutputStream(mediaFile);
                    //                         } catch (Exception ex) {
                    //                             ex.printStackTrace();
                    //                         }
                    //                         // Start videoPlayer page
                    //                         //  if (mPlaylistViewModel == null) {
                    //                         //      return;
                    //                         //  }
                    //                         //  Log.e("data_source", "onResponseStarted");
                    //                         //
                    //                         mPlaylistViewModel.setPlaylistItemToOpen(playlistItemModel);
                    //                     }

                    //                     @Override
                    //                     public void onDataReceived(byte[] response) {
                    //                         Log.e("data_source",
                    //                                 "onDataReceived : " + response.length);
                    //                         // PostTask.postTask(
                    //                         //         TaskTraits.USER_VISIBLE_MAY_BLOCK, () -> {
                    //                         try {
                    //                             // MediaUtils.writeToFile(response,
                    //                             // PlaylistHostActivity.this); String text =
                    //                             "Hello
                    //                             // \n";
                    //                             MediaUtils.writeToFile(response,
                    //                                     MediaUtils
                    //                                             .getTempFile(
                    //                                                     PlaylistHostActivity.this)
                    //                                             .getAbsolutePath());
                    //                             // fileOutputStream.write(response);
                    //                         } catch (Exception ex) {
                    //                             ex.printStackTrace();
                    //                         }
                    //                         // });
                    //                     }

                    //                     @Override
                    //                     public void onDataCompleted() {
                    //                         try {
                    //                             // fileOutputStream.close();
                    //                         } catch (Exception ex) {
                    //                             ex.printStackTrace();
                    //                         }
                    //                         // if (mPlaylistViewModel == null) {
                    //                         //     return;
                    //                         // }
                    //                         // Log.e("data_source", "onDataCompleted");
                    //                         //
                    //                         mPlaylistViewModel.setPlaylistItemToOpen(playlistItemModel);
                    //                     }

                    //                     @Override
                    //                     public void close() {}

                    //                     @Override
                    //                     public void onConnectionError(MojoException e) {}
                    //                 };

                    //         mPlaylistService.addObserverForStreaming(playlistStreamingObserverImpl);
                    //     }
                    // }
                });

        mPlaylistViewModel.getCreatePlaylistOption().observe(
                PlaylistHostActivity.this, createPlaylistModel -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    Playlist playlist = new Playlist();
                    playlist.name = createPlaylistModel.getNewPlaylistId();
                    playlist.items = new PlaylistItem[0];
                    mPlaylistService.createPlaylist(playlist, createdPlaylist -> {
                        if (createPlaylistModel.isMoveOrCopy()
                                && PlaylistUtils.moveOrCopyModel != null) {
                            MoveOrCopyModel tempMoveOrCopyModel = PlaylistUtils.moveOrCopyModel;
                            PlaylistUtils.moveOrCopyModel = new MoveOrCopyModel(
                                    tempMoveOrCopyModel.getPlaylistOptionsEnum(),
                                    createdPlaylist.id, tempMoveOrCopyModel.getPlaylistItems());
                            mPlaylistViewModel.performMoveOrCopy(PlaylistUtils.moveOrCopyModel);
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
                        // PlaylistDownloadUtils.removeDownloadRequest(
                        //         PlaylistHostActivity.this, playlistItem);
                    }
                    loadPlaylist(playlistItems.getId());
                });

        mPlaylistViewModel.getMoveOrCopyItems().observe(
                PlaylistHostActivity.this, moveOrCopyModel -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    if (moveOrCopyModel.getPlaylistOptionsEnum()
                                    == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM
                            || moveOrCopyModel.getPlaylistOptionsEnum()
                                    == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS) {
                        for (PlaylistItemModel playlistItem : moveOrCopyModel.getPlaylistItems()) {
                            mPlaylistService.moveItem(playlistItem.getPlaylistId(),
                                    moveOrCopyModel.getToPlaylistId(), playlistItem.getId());
                        }
                    } else {
                        String[] playlistIds =
                                new String[moveOrCopyModel.getPlaylistItems().size()];
                        for (int i = 0; i < moveOrCopyModel.getPlaylistItems().size(); i++) {
                            playlistIds[i] = moveOrCopyModel.getPlaylistItems().get(i).getId();
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
                    PlaylistOptionsEnum option = playlistOptionsModel.getOptionType();
                    if (option == PlaylistOptionsEnum.REMOVE_PLAYLIST_OFFLINE_DATA) {
                        if (playlistOptionsModel.getPlaylistModel() != null) {
                            mPlaylistService.removeLocalDataForItemsInPlaylist(
                                    playlistOptionsModel.getPlaylistModel().getId());
                        }
                    } else if (option == PlaylistOptionsEnum.DOWNLOAD_PLAYLIST_FOR_OFFLINE_USE) {
                        if (playlistOptionsModel.getPlaylistModel() != null) {
                            for (PlaylistItemModel playlistItemModel :
                                    playlistOptionsModel.getPlaylistModel().getItems()) {
                                mPlaylistService.recoverLocalDataForItem(playlistItemModel.getId(),
                                        true,
                                        playlistItem
                                        -> {
                                                // PlaylistDownloadUtils.startDownloadRequest(
                                                //         PlaylistHostActivity.this,
                                                //         playlistItemModel);
                                        });
                            }
                        }
                    } else if (option == PlaylistOptionsEnum.DELETE_PLAYLIST) {
                        if (playlistOptionsModel.getPlaylistModel() != null) {
                            mPlaylistService.removePlaylist(
                                    playlistOptionsModel.getPlaylistModel().getId());
                            for (PlaylistItemModel playlistItem :
                                    playlistOptionsModel.getPlaylistModel().getItems()) {
                                // PlaylistDownloadUtils.removeDownloadRequest(
                                //         PlaylistHostActivity.this, playlistItem);
                            }
                        }
                    } else if (option == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEMS) {
                        showMoveOrCopyPlaylistBottomSheet();
                    } else if (option == PlaylistOptionsEnum.COPY_PLAYLIST_ITEMS) {
                        showMoveOrCopyPlaylistBottomSheet();
                    }
                });

        mPlaylistViewModel.getAllPlaylistOption().observe(
                PlaylistHostActivity.this, playlistOptionsModel -> {
                    if (mPlaylistService == null) {
                        return;
                    }
                    PlaylistOptionsEnum option = playlistOptionsModel.getOptionType();
                    if (option == PlaylistOptionsEnum.REMOVE_ALL_OFFLINE_DATA) {
                        if (playlistOptionsModel.getAllPlaylistModels() != null) {
                            for (PlaylistModel playlistModel :
                                    playlistOptionsModel.getAllPlaylistModels()) {
                                mPlaylistService.removeLocalDataForItemsInPlaylist(
                                        playlistModel.getId());
                            }
                        }
                    } else if (option
                            == PlaylistOptionsEnum.DOWNLOAD_ALL_PLAYLISTS_FOR_OFFLINE_USE) {
                        mPlaylistService.getAllPlaylists(playlists -> {
                            for (Playlist playlist : playlists) {
                                for (PlaylistItem playlistItem : playlist.items) {
                                    mPlaylistService.recoverLocalDataForItem(
                                            playlistItem.id, true, tempPlaylistItem -> {
                                                PlaylistItemModel playlistItemModel =
                                                        new PlaylistItemModel(playlistItem.id,
                                                                ConstantUtils.DEFAULT_PLAYLIST,
                                                                playlistItem.name,
                                                                playlistItem.pageSource.url,
                                                                playlistItem.mediaPath.url,
                                                                playlistItem.mediaSource.url,
                                                                playlistItem.thumbnailPath.url,
                                                                playlistItem.author,
                                                                playlistItem.duration,
                                                                playlistItem.lastPlayedPosition,
                                                                playlistItem.cached, false, 0);
                                                // PlaylistDownloadUtils.startDownloadRequest(
                                                //         PlaylistHostActivity.this,
                                                //         playlistItemModel);
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
                    PlaylistOptionsEnum option = playlistItemOption.getOptionType();
                    if (option == PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM) {
                        showMoveOrCopyPlaylistBottomSheet();
                    } else if (option == PlaylistOptionsEnum.COPY_PLAYLIST_ITEM) {
                        showMoveOrCopyPlaylistBottomSheet();
                    } else if (option == PlaylistOptionsEnum.DELETE_ITEMS_OFFLINE_DATA) {
                        mPlaylistService.removeLocalDataForItem(
                                playlistItemOption.getPlaylistItemModel().getId());
                        // PlaylistDownloadUtils.removeDownloadRequest(PlaylistHostActivity.this,
                        //         playlistItemOption.getPlaylistItemModel());
                        // Playlist item will be updated based on event
                    } else if (option == PlaylistOptionsEnum.OPEN_IN_NEW_TAB) {
                        openPlaylistInTab(
                                false, playlistItemOption.getPlaylistItemModel().getPageSource());
                    } else if (option == PlaylistOptionsEnum.OPEN_IN_PRIVATE_TAB) {
                        openPlaylistInTab(
                                true, playlistItemOption.getPlaylistItemModel().getPageSource());
                    } else if (option == PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM) {
                        mPlaylistService.removeItemFromPlaylist(playlistItemOption.getPlaylistId(),
                                playlistItemOption.getPlaylistItemModel().getId());
                        // PlaylistDownloadUtils.removeDownloadRequest(PlaylistHostActivity.this,
                        //         playlistItemOption.getPlaylistItemModel());
                        loadPlaylist(playlistItemOption.getPlaylistId());
                    } else if (option == PlaylistOptionsEnum.RECOVER_PLAYLIST_ITEM) {
                        mPlaylistService.recoverLocalDataForItem(
                                playlistItemOption.getPlaylistItemModel().getId(), true,
                                playlistItem
                                -> {
                                        // Playlist item will be updated based on event
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
        if (PlaylistOptionsEnum.DELETE_PLAYLIST == playlistOptionsModel.getOptionType()
                && mPlaylistService != null && playlistOptionsModel.getPlaylistModel() != null) {
            mPlaylistService.removePlaylist(playlistOptionsModel.getPlaylistModel().getId());
            for (PlaylistItemModel playlistItem :
                    playlistOptionsModel.getPlaylistModel().getItems()) {
                // PlaylistDownloadUtils.removeDownloadRequest(
                //         PlaylistHostActivity.this, playlistItem);
            }
            finish();
        }
    }

    private void openPlaylistInTab(boolean isIncognito, String url) {
        finish();
        TabUtils.openUrlInNewTab(isIncognito, url);
    }

    @Override
    public void onEvent(int eventType, String id) {
        if (mPlaylistService == null || mPlaylistViewModel == null) {
            return;
        }
        if (eventType == PlaylistEvent.LIST_CREATED || eventType == PlaylistEvent.LIST_REMOVED) {
            loadAllPlaylists();
        } else if (eventType == PlaylistEvent.ITEM_MOVED) {
            loadPlaylist(id);
        } else {
            updatePlaylistItemEvent(eventType, id);
        }
    }

    private void updatePlaylistItemEvent(int eventType, String playlistItemId) {
        PlaylistItemEventEnum playlistItemEvent = PlaylistItemEventEnum.NONE;
        switch (eventType) {
            case PlaylistEvent.ITEM_ADDED:
                playlistItemEvent = PlaylistItemEventEnum.ITEM_ADDED;
                break;
            case PlaylistEvent.ITEM_THUMBNAIL_READY:
                playlistItemEvent = PlaylistItemEventEnum.ITEM_THUMBNAIL_READY;
                break;
            case PlaylistEvent.ITEM_THUMBNAIL_FAILED:
                playlistItemEvent = PlaylistItemEventEnum.ITEM_THUMBNAIL_FAILED;
                break;
            case PlaylistEvent.ITEM_CACHED:
                playlistItemEvent = PlaylistItemEventEnum.ITEM_CACHED;
                break;
            case PlaylistEvent.ITEM_DELETED:
                playlistItemEvent = PlaylistItemEventEnum.ITEM_DELETED;
                break;
            case PlaylistEvent.ITEM_UPDATED:
                playlistItemEvent = PlaylistItemEventEnum.ITEM_UPDATED;
                break;
            case PlaylistEvent.ITEM_ABORTED:
                playlistItemEvent = PlaylistItemEventEnum.ITEM_ABORTED;
                break;
            case PlaylistEvent.ITEM_LOCAL_DATA_REMOVED:
                playlistItemEvent = PlaylistItemEventEnum.ITEM_LOCAL_DATA_REMOVED;
                break;
        }
        final PlaylistItemEventEnum localPlaylistItemEvent = playlistItemEvent;
        mPlaylistService.getPlaylistItem(playlistItemId, playlistItem -> {
            PlaylistItemModel playlistItemModel = new PlaylistItemModel(playlistItem.id,
                    ConstantUtils.DEFAULT_PLAYLIST, playlistItem.name, playlistItem.pageSource.url,
                    playlistItem.mediaPath.url, playlistItem.mediaSource.url,
                    playlistItem.thumbnailPath.url, playlistItem.author, playlistItem.duration,
                    playlistItem.lastPlayedPosition, playlistItem.cached, false, 0);
            mPlaylistViewModel.updatePlaylistItemEvent(
                    new PlaylistItemEventModel(localPlaylistItemEvent, playlistItemModel));
        });
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
        // if (mPlaylistStreamingObserver != null) {
        //     mPlaylistStreamingObserver.close();
        //     mPlaylistStreamingObserver.destroy();
        //     mPlaylistStreamingObserver = null;
        // }
        super.onDestroy();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    private void downalodHLSFile(String manifestUrl, Segment segment) {
        if (mPlaylistService != null) {
            // Log.e("data_source", "queryPrompt : "+manifestUrl);
            mPlaylistService.queryPrompt(UriUtil.resolve(manifestUrl, segment.url), "GET");
            PlaylistStreamingObserver playlistStreamingObserverImpl =
                    new PlaylistStreamingObserver() {
                        @Override
                        public void onResponseStarted(String url, long contentLength) {
                            try {
                                Log.e("data_source", "onResponseStarted : " + segment.url);
                                // File mediaFile = new File(
                                //         MediaUtils
                                //                 .getTempFile(
                                //                         PlaylistHostActivity.this)
                                //                 .getAbsolutePath());
                                // if (mediaFile.exists()) {
                                //     mediaFile.delete();
                                // }
                            } catch (Exception ex) {
                                ex.printStackTrace();
                            }
                        }

                        @Override
                        public void onDataReceived(byte[] response) {
                            // Log.e("data_source",
                            //         "onDataReceived : " + response.length);
                            try {
                                MediaUtils.writeToFile(response,
                                        MediaUtils.getTempFile(PlaylistHostActivity.this)
                                                .getAbsolutePath());
                            } catch (Exception ex) {
                                ex.printStackTrace();
                            }
                        }

                        @Override
                        public void onDataCompleted() {
                            // Log.e("data_source", "onDataCompleted : "+segment.url);
                            try {
                                mPlaylistService.clearObserverForStreaming();
                                Segment newSegment = segmentsQueue.poll();
                                if (newSegment != null) {
                                    downalodHLSFile(manifestUrl, newSegment);
                                }
                            } catch (Exception ex) {
                                ex.printStackTrace();
                            }
                        }

                        @Override
                        public void close() {}

                        @Override
                        public void onConnectionError(MojoException e) {}
                    };

            mPlaylistService.addObserverForStreaming(playlistStreamingObserverImpl);
        }
    }
}

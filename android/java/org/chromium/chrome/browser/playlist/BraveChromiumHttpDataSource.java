/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.playlist;

import static com.google.android.exoplayer2.upstream.HttpUtil.buildRangeRequestHeader;
import static com.google.android.exoplayer2.util.Assertions.checkNotNull;
import static com.google.android.exoplayer2.util.Util.castNonNull;

import static java.lang.Math.min;

import android.net.Uri;

import androidx.annotation.Nullable;

import com.google.android.exoplayer2.C;
import com.google.android.exoplayer2.PlaybackException;
import com.google.android.exoplayer2.upstream.BaseDataSource;
import com.google.android.exoplayer2.upstream.DataSource;
import com.google.android.exoplayer2.upstream.DataSourceException;
import com.google.android.exoplayer2.upstream.DataSpec;
import com.google.android.exoplayer2.upstream.HttpDataSource;
import com.google.android.exoplayer2.upstream.HttpUtil;
import com.google.android.exoplayer2.upstream.TransferListener;
import com.google.android.exoplayer2.util.Log;
import com.google.android.exoplayer2.util.Util;
import com.google.common.base.Predicate;
import com.google.common.collect.ImmutableMap;
import com.google.common.net.HttpHeaders;

import org.chromium.chrome.browser.playlist.PlaylistHostActivity;
import org.chromium.chrome.browser.playlist.PlaylistStreamingObserverImpl;
import org.chromium.chrome.browser.playlist.PlaylistStreamingObserverImpl.PlaylistStreamingObserverImplDelegate;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.net.ChromiumNetworkAdapter;
import org.chromium.net.NetworkTrafficAnnotationTag;
import org.chromium.playlist.mojom.PlaylistService;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class BraveChromiumHttpDataSource extends BaseDataSource
        implements PlaylistStreamingObserverImplDelegate, ConnectionErrorHandler {
    // private PlaylistService mPlaylistService;
    private PlaylistStreamingObserverImpl mPlaylistStreamingObserver;

    @Nullable
    private Uri uri;
    private int bytesAlreadyRead;
    private boolean opened;

    @Override
    public void onConnectionError(MojoException e) {
        // mPlaylistService = null;
        // initPlaylistService();
    }

    // private void initPlaylistService() {
    //     if (mPlaylistService != null) {
    //         return;
    //     }

    //     mPlaylistService = PlaylistServiceFactoryAndroid.getInstance().getPlaylistService(this);
    // }

    public BraveChromiumHttpDataSource() {
        super(/* isNetwork= */ true);
        // initPlaylistService();
        mPlaylistStreamingObserver = new PlaylistStreamingObserverImpl(this);
        PlaylistHostActivity.mPlaylistService.addObserverForStreaming(mPlaylistStreamingObserver);
    }

    @Override
    public long open(DataSpec dataSpec) {
        Log.e("data_source", "open : " + dataSpec.toString());
        uri = dataSpec.uri;
        if (PlaylistHostActivity.mPlaylistService != null) {
            PlaylistHostActivity.mPlaylistService.queryPrompt(
                    "https://rr6---sn-8qu-t0ael.googlevideo.com/videoplayback?expire=1691067483&ei=-0_LZPHhNuu_sfIP5firuAg&ip=23.233.146.226&id=o-AExLNlXx85_Q4tcHnUF6MGOc_T66iJJf1OaHS0gjPRn3&itag=18&source=youtube&requiressl=yes&mh=-i&mm=31%2C29&mn=sn-8qu-t0ael%2Csn-t0a7ln7d&ms=au%2Crdu&mv=m&mvi=6&pl=19&initcwndbps=2480000&spc=UWF9f_yJ8R6zaVHVRh7UWka1Z0gG9C0wjTxsbIcl9Q&vprv=1&svpuc=1&mime=video%2Fmp4&gir=yes&clen=14278085&ratebypass=yes&dur=314.049&lmt=1690412588093133&mt=1691045367&fvip=4&fexp=24007246%2C24363392&c=MWEB&txp=5538434&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cspc%2Cvprv%2Csvpuc%2Cmime%2Cgir%2Cclen%2Cratebypass%2Cdur%2Clmt&sig=AOq0QJ8wRAIgP_0mpb4UkaWYq1C7-4oQkbrwZysBqsqw6vjd30-DK5ICICTSjSeKzCfQrnzZeEAmHVxvGJI3iWfEHbSwWlQ-G4xD&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Cinitcwndbps&lsig=AG3C_xAwRgIhALQm0pOXEIjgjdFHih-hyWFGwUvjyfd9uZDbXwzcecpOAiEA1disYFd9JOfd2nmMArSVz7_6u-BUN9dUmTUqsYGSdPk%3D&cpn=Coffzlw1c5mzHuJT&cver=2.20230802.00.00&ptk=youtube_single&oid=PAIgdgLHpTFS4TA-jk_AHA&ptchn=8p1vwvWtl6T73JiExfWs1g&pltype=content",
                    "GET");
        }
        // BraveVpnNativeWorker.getInstance().queryPrompt(
        //         dataSpec.uri.toString(),
        //         "GET");
        transferInitializing(dataSpec);
        bytesAlreadyRead = 0;
        opened = true;
        transferStarted(dataSpec);
        return C.LENGTH_UNSET;
    }

    @Override
    public int read(byte[] buffer, int offset, int readLength) {
        // if (!opened) {
        //     throw new HttpDataSourceException(
        //             "Data source is not opened.", dataSpec, HttpDataSourceException.TYPE_OPEN);
        // }
        BraveVpnNativeWorker.readLength = readLength + bytesAlreadyRead;
        int responseLength;
        try {
            responseLength = BraveVpnNativeWorker.responseLength.get();
        } catch (Exception ex) {
        }
        byte[] data = BraveVpnNativeWorker.getInstance().getLatestData();
        int byteAvailableToRead = (int) (data.length - bytesAlreadyRead);
        if (readLength == 0 || byteAvailableToRead == 0) {
            Log.e("data_source",
                    "byteAvailableToRead : " + byteAvailableToRead + " : readLength : " + readLength
                            + " : C.RESULT_NOTHING_READ");
            return C.RESULT_NOTHING_READ;
        }
        int bytesReadThisTime = Math.min(byteAvailableToRead, readLength);
        System.arraycopy(data, bytesAlreadyRead, buffer, offset, bytesReadThisTime);
        bytesAlreadyRead += bytesReadThisTime;
        Log.e("data_source",
                "offset : " + offset + " : byteAvailableToRead : " + byteAvailableToRead);
        bytesTransferred(bytesReadThisTime);
        return bytesReadThisTime;
    }

    @Override
    public Uri getUri() {
        return uri;
    }

    @Override
    public void close() {
        if (opened) {
            opened = false;
            transferEnded();
        }
        uri = null;
        // if (mPlaylistService != null) {
        //     mPlaylistService.close();
        // }
        if (mPlaylistStreamingObserver != null) {
            mPlaylistStreamingObserver.close();
            mPlaylistStreamingObserver.destroy();
            mPlaylistStreamingObserver = null;
        }
    }

    // private byte[] data;

    // @Nullable
    // private Uri uri;
    // private int readPosition;
    // private int bytesRemaining;
    // private boolean opened;

    // /**
    //  * @param data The data to be read.
    //  */
    // public BraveChromiumHttpDataSource(byte[] data) {
    //     super(/* isNetwork= */ false);
    //     this.data = data;
    // }

    // @Override
    // public long open(DataSpec dataSpec) throws IOException {
    //     BraveVpnNativeWorker.getInstance().addObserver(this);
    //     uri = dataSpec.uri;
    //     transferInitializing(dataSpec);
    //     readPosition = (int) dataSpec.position;
    //     bytesRemaining = (int) ((data.length - dataSpec.position));
    //     opened = true;
    //     transferStarted(dataSpec);
    //     return C.LENGTH_UNSET;
    // }

    // @Override
    // public int read(byte[] buffer, int offset, int readLength) {

    //     if (readLength == 0 || bytesRemaining == 0)
    //         return C.RESULT_NOTHING_READ;

    //     readLength = min(readLength, bytesRemaining);
    //     for (int i = 0; i < readLength; i++) {
    //         buffer[offset + i] = data[readPosition + i];
    //     }
    //     readPosition += readLength;
    //     bytesRemaining -= readLength;
    //     bytesTransferred(readLength);
    //     return readLength;
    // }

    // @Override
    // @Nullable
    // public Uri getUri() {
    //     return uri;
    // }

    // @Override
    // public void close() {
    //     if (opened) {
    //         opened = false;
    //         transferEnded();
    //     }
    //     uri = null;
    // }

    // public void increaseBytesRemaining(int x) {
    //     bytesRemaining += x;

    // }

    // @Override
    // public void onDataReceived(byte[] response) {
    //     bytesRemaining += response.length;
    //     this.data = BraveVpnNativeWorker.output.toByteArray();
    // }
}

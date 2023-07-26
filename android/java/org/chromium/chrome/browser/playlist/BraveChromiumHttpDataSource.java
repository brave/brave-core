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

import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.net.ChromiumNetworkAdapter;
import org.chromium.net.NetworkTrafficAnnotationTag;

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

public class BraveChromiumHttpDataSource extends BaseDataSource implements BraveVpnObserver {
    @Nullable
    private Uri uri;
    private int bytesAlreadyRead;
    private boolean opened;

    public BraveChromiumHttpDataSource() {
        super(/* isNetwork= */ true);
    }

    @Override
    public long open(DataSpec dataSpec) {
        Log.e("data_source", "open : " + dataSpec.toString());
        uri = dataSpec.uri;
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

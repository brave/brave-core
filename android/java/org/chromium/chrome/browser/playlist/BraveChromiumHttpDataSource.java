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
import org.chromium.chrome.browser.playlist.kotlin.TestVideoActivity;
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

public class BraveChromiumHttpDataSource extends BaseDataSource {
    @Nullable
    private Uri uri;
    private int bytesAlreadyRead;
    private boolean opened;

    private long bytesToRead;
    private long bytesRead;

    public BraveChromiumHttpDataSource() {
        super(/* isNetwork= */ false);
    }

    @Override
    public long open(DataSpec dataSpec) {
        // Log.e("data_source", "open : " + dataSpec.toString());
        uri = dataSpec.uri;
        transferInitializing(dataSpec);
        // bytesAlreadyRead = 0;
        opened = true;
        transferStarted(dataSpec);
        bytesRead = 0;
        bytesToRead = 0;
        try {
            bytesToRead = BraveVpnNativeWorker.contentLength.get();
        } catch (Exception ex) {
        }
        Log.e("data_source", "open : " + bytesToRead);
        return bytesToRead;
    }

    @Override
    public int read(byte[] buffer, int offset, int readLength) {
        // readLength = Math.min(readLength, 100);
        // BraveVpnNativeWorker.readLength = readLength + bytesAlreadyRead;
        // int responseLength;
        // try {
        //     responseLength = BraveVpnNativeWorker.responseLength.get();
        // } catch (Exception ex) {
        // }
        // byte[] data = BraveVpnNativeWorker.getInstance().getLatestData();
        // int byteAvailableToRead = (int) (data.length - bytesAlreadyRead);
        // if (readLength == 0 || byteAvailableToRead == 0) {
        //     Log.e("data_source",
        //             "byteAvailableToRead : " + byteAvailableToRead + " : readLength : " +
        //             readLength
        //                     + " : C.RESULT_NOTHING_READ");
        //     return C.RESULT_NOTHING_READ;
        // }
        // int bytesReadThisTime = Math.min(byteAvailableToRead, readLength);
        // System.arraycopy(data, bytesAlreadyRead, buffer, offset, bytesReadThisTime);
        // bytesAlreadyRead += bytesReadThisTime;
        // Log.e("data_source",
        //         "offset : " + offset + " : byteAvailableToRead : " + byteAvailableToRead);
        // bytesTransferred(bytesReadThisTime);
        // return bytesReadThisTime;
        Log.e("data_source", "readLength : " + readLength);

        if (readLength == 0) {
            Log.e("data_source", "readLength == 0");
            return 0;
        }
        if (bytesToRead != C.LENGTH_UNSET) {
            long bytesRemaining = bytesToRead - bytesRead;
            if (bytesRemaining == 0) {
                Log.e("data_source", "bytesRemaining C.RESULT_END_OF_INPUT");
                return C.RESULT_END_OF_INPUT;
            }
            readLength = (int) min(readLength, bytesRemaining);
        }

        int read = 0;
        try {
            read = TestVideoActivity.pipedInputStream.read(buffer, offset, readLength);
            // Log.e("data_source", "TestVideoActivity.pipedInputStream length :
            // "+TestVideoActivity.pipedInputStream.available());
        } catch (IOException e) {
            Log.e("data_source", "ERROR:::::");
            e.printStackTrace();
        }
        if (read == -1) {
            Log.e("data_source", "C.RESULT_END_OF_INPUT");
            return C.RESULT_END_OF_INPUT;
        }

        Log.e("data_source", "read : " + read);

        bytesRead += read;
        bytesTransferred(read);
        return read;
    }

    @Override
    public Uri getUri() {
        return uri;
    }

    @Override
    public void close() {
        Log.e("data_source", "close");
        if (opened) {
            opened = false;
            transferEnded();
        }
        uri = null;
    }
}

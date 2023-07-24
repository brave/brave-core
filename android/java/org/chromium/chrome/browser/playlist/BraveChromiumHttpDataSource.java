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

public class BraveChromiumHttpDataSource implements HttpDataSource {
    private final RequestProperties requestProperties = new RequestProperties();
    // private byte[] dataReceived;
    // private long bytesRead;
    // private long bytesToRead;
    // @Nullable
    // private DataSpec dataSpec;

    // @Override
    // @Nullable
    // public Uri getUri() {
    //     Log.e("custom", "BraveVpnNativeWorker.url : "+ BraveVpnNativeWorker.url);
    //     return BraveVpnNativeWorker.url == null ? null : Uri.parse(BraveVpnNativeWorker.url);
    // }

    // @Override
    // public int getResponseCode() {
    //     return 200;
    // }

    // @Override
    // public Map<String, List<String>> getResponseHeaders() {
    //     return ImmutableMap.of();
    // }

    // @Override
    // public void setRequestProperty(String name, String value) {
    //     checkNotNull(name);
    //     checkNotNull(value);
    //     requestProperties.set(name, value);
    // }

    // @Override
    // public void clearRequestProperty(String name) {
    //     checkNotNull(name);
    //     requestProperties.remove(name);
    // }

    // @Override
    // public void clearAllRequestProperties() {
    //     requestProperties.clear();
    // }

    // @Override
    // public void addTransferListener(TransferListener transferListener) {}

    // @Override
    // public long open(DataSpec dataSpec) throws HttpDataSourceException {
    //     this.dataSpec = dataSpec;
    //     // BraveVpnNativeWorker.getInstance().queryPrompt(
    //     //         dataSpec.uri.toString(),
    //     //         dataSpec.getHttpMethodString());
    //     bytesRead = 0;
    //     bytesToRead = dataSpec.length != C.LENGTH_UNSET ? dataSpec.length :
    //     BraveVpnNativeWorker.contentLength; Log.e("custom", "bytesToRead : "+ bytesToRead);
    //     return bytesToRead ;
    // }

    // @Override
    // public int read(byte[] buffer, int offset, int length) throws HttpDataSourceException {
    //     try {
    //         return readInternal(buffer, offset, length);
    //     } catch (IOException e) {
    //         throw HttpDataSourceException.createForIOException(
    //                 e, castNonNull(dataSpec), HttpDataSourceException.TYPE_READ);
    //     }
    // }

    // private int readInternal(byte[] buffer, int offset, int readLength) throws IOException {
    //     if (readLength == 0) {
    //         return 0;
    //     }

    //     Log.e("custom", "dataReceived : before readLength : "+ readLength);
    //     if (bytesToRead != C.LENGTH_UNSET) {
    //         Log.e("custom", "dataReceived : bytesToRead : "+ bytesToRead);
    //         Log.e("custom", "dataReceived : bytesRead : "+ bytesRead);
    //         long bytesRemaining = bytesToRead - bytesRead;
    //         if (bytesRemaining == 0) {
    //             return C.RESULT_END_OF_INPUT;
    //         }

    //         Log.e("custom", "dataReceived : bytesRemaining : "+ bytesRemaining);
    //         readLength = (int) min(200, bytesRemaining);
    //     }

    //     // byte[] dataReceived = BraveVpnNativeWorker.output.toByteArray();
    //     byte[] dataReceived = BraveVpnNativeWorker.tempStorage.get(0);
    //     Log.e("custom", "dataReceived : dataReceived.length : "+ dataReceived.length);

    //     Log.e("custom", "dataReceived : offset : "+ offset);
    //     Log.e("custom", "dataReceived : after readLength : "+ readLength);
    //     Log.e("custom", "dataReceived : min(readLength,200) : "+ min(readLength,200));

    //     readLength = min(readLength,200);
    //     readLength = min(readLength, dataReceived.length);
    //     InputStream targetStream = new ByteArrayInputStream(dataReceived);
    //     int read = targetStream.read(buffer, BraveVpnNativeWorker.currentOffset, readLength);
    //     Log.e("custom", "dataReceived : read.length : "+ read);
    //     if (read == -1) {
    //         return C.RESULT_END_OF_INPUT;
    //     }

    //     targetStream.close();
    //     if (dataReceived.length > readLength) {
    //         BraveVpnNativeWorker.currentOffset = BraveVpnNativeWorker.currentOffset + readLength;
    //     } else {
    //         BraveVpnNativeWorker.currentOffset = 0;
    //         BraveVpnNativeWorker.tempStorage.remove(0);
    //     }

    //     bytesRead += read;

    //     // return bytes read for offset
    //     return read;
    // }

    // @Override
    // public void close() throws HttpDataSourceException {
    //     // Close streams
    //     try {
    //             // BraveVpnNativeWorker.output.close();
    //         BraveVpnNativeWorker.tempStorage.clear();
    //         } catch (Exception e) {
    //             Log.e("data_source", e.getMessage());
    //         }
    // }

    // /**
    //  * {@link DataSource.Factory} for {@link BraveChromiumHttpDataSource} instances.
    //  */
    // public static final class Factory implements HttpDataSource.Factory {
    //     /**
    //      * Creates an instance.
    //      */
    //     public Factory() {}

    //     @Override
    //     public BraveChromiumHttpDataSource.Factory setDefaultRequestProperties(
    //             Map<String, String> defaultRequestProperties) {
    //         return this;
    //     }

    //     @Override
    //     public BraveChromiumHttpDataSource createDataSource() {
    //         BraveChromiumHttpDataSource dataSource =
    //                 new BraveChromiumHttpDataSource();
    //         return dataSource;
    //     }
    // }

    private byte[] data;
    private DataSpec dataSpec;
    private long bytesRead;
    private boolean opened;

    public BraveChromiumHttpDataSource(byte[] data) {
        this.data = data;
    }

    @Override
    public void setRequestProperty(String name, String value) {
        checkNotNull(name);
        checkNotNull(value);
        requestProperties.set(name, value);
    }

    @Override
    public void clearRequestProperty(String name) {
        checkNotNull(name);
        requestProperties.remove(name);
    }

    @Override
    public void clearAllRequestProperties() {
        requestProperties.clear();
    }

    @Override
    public void addTransferListener(TransferListener transferListener) {}

    @Override
    public long open(DataSpec dataSpec) throws HttpDataSourceException {
        this.dataSpec = dataSpec;
        bytesRead = 0;
        opened = true;
        // transferInitializing(dataSpec);
        Log.e("data_source", "data.length : " + data.length);
        return data.length;
    }

    @Override
    public int read(byte[] buffer, int offset, int readLength) throws HttpDataSourceException {
        if (!opened) {
            throw new HttpDataSourceException(
                    "Data source is not opened.", dataSpec, HttpDataSourceException.TYPE_OPEN);
        }

        int remaining = (int) (data.length - bytesRead);
        if (remaining == 0) {
            return C.RESULT_END_OF_INPUT; // End of stream reached
        }

        Log.e("data_source", "remaining : " + remaining);
        Log.e("data_source", "readLength : " + readLength);
        int bytesReadThisTime = Math.min(remaining, readLength);
        System.arraycopy(data, (int) bytesRead, buffer, offset, bytesReadThisTime);
        // removeBytes(data, offset);
        bytesRead += bytesReadThisTime;
        Log.e("data_source", "bytesReadThisTime : " + bytesReadThisTime);
        // bytesTransferred(bytesReadThisTime);
        return bytesReadThisTime;
    }

    void removeBytes(byte[] data, int offset) {
        for (int i = offset; i < data.length; i++) {
            data[i - offset] = data[i];
        }
        for (int i = data.length - 1; i > data.length - offset + 1; i--) {
            data[i] = 0;
        }
    }

    @Override
    public Uri getUri() {
        return dataSpec != null ? dataSpec.uri : null;
    }

    @Override
    public Map<String, List<String>> getResponseHeaders() {
        return ImmutableMap.of();
    }

    @Override
    public void close() throws HttpDataSourceException {
        dataSpec = null;
        opened = false;
        bytesRead = 0;
    }

    @Override
    public int getResponseCode() {
        return 200;
    }
}

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
    /**
     * The default connection timeout, in milliseconds.
     */
    public static final int DEFAULT_CONNECT_TIMEOUT_MILLIS = 8 * 1000;
    /**
     * The default read timeout, in milliseconds.
     */
    public static final int DEFAULT_READ_TIMEOUT_MILLIS = 8 * 1000;
    private static final String TAG = "DataSource";
    private static final long MAX_BYTES_TO_DRAIN = 2048;
    private final int connectTimeoutMillis;
    private final int readTimeoutMillis;
    @Nullable
    private final String userAgent;
    @Nullable
    private final RequestProperties defaultRequestProperties;
    private final RequestProperties requestProperties;
    @Nullable
    private Predicate<String> contentTypePredicate;
    @Nullable
    private DataSpec dataSpec;
    @Nullable
    private HttpURLConnection connection;
    @Nullable
    private InputStream inputStream;
    private boolean opened;
    private int responseCode;
    private long bytesToRead;
    private long bytesRead;

    private BraveChromiumHttpDataSource(@Nullable String userAgent, int connectTimeoutMillis,
            int readTimeoutMillis, @Nullable RequestProperties defaultRequestProperties,
            @Nullable Predicate<String> contentTypePredicate) {
        //        super(/* isNetwork= */ true);
        this.userAgent = userAgent;
        this.connectTimeoutMillis = connectTimeoutMillis;
        this.readTimeoutMillis = readTimeoutMillis;
        this.defaultRequestProperties = defaultRequestProperties;
        this.contentTypePredicate = contentTypePredicate;
        this.requestProperties = new RequestProperties();
    }

    /**
     * On platform API levels 19 and 20, okhttp's implementation of {@link InputStream#close} can
     * block for a long time if the stream has a lot of data remaining. Call this method before
     * closing the input stream to make a best effort to cause the input stream to encounter an
     * unexpected end of input, working around this issue. On other platform API levels, the method
     * does nothing.
     *
     * @param connection     The connection whose {@link InputStream} should be terminated.
     * @param bytesRemaining The number of bytes remaining to be read from the input stream if its
     *                       length is known. {@link C#LENGTH_UNSET} otherwise.
     */
    private static void maybeTerminateInputStream(
            @Nullable HttpURLConnection connection, long bytesRemaining) {
        if (connection == null || Util.SDK_INT < 19 || Util.SDK_INT > 20) {
            return;
        }

        try {
            InputStream inputStream = connection.getInputStream();
            if (bytesRemaining == C.LENGTH_UNSET) {
                // If the input stream has already ended, do nothing. The socket may be re-used.
                if (inputStream.read() == -1) {
                    return;
                }
            } else if (bytesRemaining <= MAX_BYTES_TO_DRAIN) {
                // There isn't much data left. Prefer to allow it to drain, which may allow the
                // socket to be re-used.
                return;
            }
            String className = inputStream.getClass().getName();
            if ("com.android.okhttp.internal.http.HttpTransport$ChunkedInputStream".equals(
                        className)
                    || "com.android.okhttp.internal.http.HttpTransport$FixedLengthInputStream"
                               .equals(className)) {
                Class<?> superclass = inputStream.getClass().getSuperclass();
                Method unexpectedEndOfInput =
                        checkNotNull(superclass).getDeclaredMethod("unexpectedEndOfInput");
                unexpectedEndOfInput.setAccessible(true);
                unexpectedEndOfInput.invoke(inputStream);
            }
        } catch (Exception e) {
            // If an IOException then the connection didn't ever have an input stream, or it was
            // closed already. If another type of exception then something went wrong, most likely
            // the device isn't using okhttp.
        }
    }

    /**
     * @deprecated Use {@link
     *         BraveChromiumHttpDataSource.Factory#setContentTypePredicate(Predicate)}
     * instead.
     */
    @Deprecated
    public void setContentTypePredicate(@Nullable Predicate<String> contentTypePredicate) {
        this.contentTypePredicate = contentTypePredicate;
    }

    @Override
    @Nullable
    public Uri getUri() {
        return connection == null ? null : Uri.parse(connection.getURL().toString());
    }

    @Override
    public int getResponseCode() {
        return connection == null || responseCode <= 0 ? -1 : responseCode;
    }

    @Override
    public Map<String, List<String>> getResponseHeaders() {
        if (connection == null) {
            return ImmutableMap.of();
        }
        return connection.getHeaderFields();
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

    /**
     * Opens the source to read the specified data.
     */
    @Override
    public long open(DataSpec dataSpec) throws HttpDataSourceException {
        this.dataSpec = dataSpec;
        Log.e("NTP", "open : dataspec : " + dataSpec.toString());
        bytesRead = 0;
        bytesToRead = 0;
        //        transferInitializing(dataSpec);

        String responseMessage;
        HttpURLConnection connection;
        try {
            this.connection = makeConnection(dataSpec);
            connection = this.connection;
            responseCode = connection.getResponseCode();
            responseMessage = connection.getResponseMessage();
        } catch (IOException e) {
            closeConnectionQuietly();
            throw HttpDataSourceException.createForIOException(
                    e, dataSpec, HttpDataSourceException.TYPE_OPEN);
        }

        // Check for a valid response code.
        if (responseCode < 200 || responseCode > 299) {
            Map<String, List<String>> headers = connection.getHeaderFields();
            if (responseCode == 416) {
                long documentSize = HttpUtil.getDocumentSize(
                        connection.getHeaderField(HttpHeaders.CONTENT_RANGE));
                if (dataSpec.position == documentSize) {
                    opened = true;
                    //                    transferStarted(dataSpec);
                    return dataSpec.length != C.LENGTH_UNSET ? dataSpec.length : 0;
                }
            }

            @Nullable
            InputStream errorStream = connection.getErrorStream();
            byte[] errorResponseBody;
            try {
                errorResponseBody =
                        errorStream != null ? Util.toByteArray(errorStream) : Util.EMPTY_BYTE_ARRAY;
            } catch (IOException e) {
                errorResponseBody = Util.EMPTY_BYTE_ARRAY;
            }
            closeConnectionQuietly();
            @Nullable
            IOException cause = responseCode == 416 ? new DataSourceException(
                                        PlaybackException.ERROR_CODE_IO_READ_POSITION_OUT_OF_RANGE)
                                                    : null;
            throw new InvalidResponseCodeException(
                    responseCode, responseMessage, cause, headers, dataSpec, errorResponseBody);
        }

        // Check for a valid content type.
        String contentType = connection.getContentType();
        if (contentTypePredicate != null && !contentTypePredicate.apply(contentType)) {
            closeConnectionQuietly();
            throw new InvalidContentTypeException(contentType, dataSpec);
        }

        if (dataSpec.length != C.LENGTH_UNSET) {
            bytesToRead = dataSpec.length;
        } else {
            long contentLength =
                    HttpUtil.getContentLength(connection.getHeaderField(HttpHeaders.CONTENT_LENGTH),
                            connection.getHeaderField(HttpHeaders.CONTENT_RANGE));
            bytesToRead = (contentLength);
        }

        try {
            inputStream = connection.getInputStream();
        } catch (IOException e) {
            closeConnectionQuietly();
            throw new HttpDataSourceException(e, dataSpec,
                    PlaybackException.ERROR_CODE_IO_UNSPECIFIED, HttpDataSourceException.TYPE_OPEN);
        }

        opened = true;
        //        transferStarted(dataSpec);

        return bytesToRead;
    }

    @Override
    public int read(byte[] buffer, int offset, int length) throws HttpDataSourceException {
        try {
            return readInternal(buffer, offset, length);
        } catch (IOException e) {
            throw HttpDataSourceException.createForIOException(
                    e, castNonNull(dataSpec), HttpDataSourceException.TYPE_READ);
        }
    }

    @Override
    public void close() throws HttpDataSourceException {
        try {
            @Nullable
            InputStream inputStream = this.inputStream;
            if (inputStream != null) {
                long bytesRemaining =
                        bytesToRead == C.LENGTH_UNSET ? C.LENGTH_UNSET : bytesToRead - bytesRead;
                maybeTerminateInputStream(connection, bytesRemaining);
                try {
                    inputStream.close();
                } catch (IOException e) {
                    throw new HttpDataSourceException(e, castNonNull(dataSpec),
                            PlaybackException.ERROR_CODE_IO_UNSPECIFIED,
                            HttpDataSourceException.TYPE_CLOSE);
                }
            }
        } finally {
            inputStream = null;
            closeConnectionQuietly();
            if (opened) {
                opened = false;
                //                transferEnded();
            }
        }
    }

    /**
     * Establishes a connection, following redirects to do so where permitted.
     */
    private HttpURLConnection makeConnection(DataSpec dataSpec) throws IOException {
        URL url = new URL(dataSpec.uri.toString());
        @DataSpec.HttpMethod
        int httpMethod = dataSpec.httpMethod;
        @Nullable
        byte[] httpBody = dataSpec.httpBody;
        long position = dataSpec.position;
        long length = dataSpec.length;
        return makeConnection(
                url, httpMethod, httpBody, position, length, dataSpec.httpRequestHeaders);
    }

    /**
     * Configures a connection and opens it.
     *
     * @param url               The url to connect to.
     * @param httpMethod        The http method.
     * @param httpBody          The body data, or {@code null} if not required.
     * @param position          The byte offset of the requested data.
     * @param length            The length of the requested data, or {@link C#LENGTH_UNSET}.
     * @param requestParameters parameters (HTTP headers) to include in request.
     */
    private HttpURLConnection makeConnection(URL url, @DataSpec.HttpMethod int httpMethod,
            @Nullable byte[] httpBody, long position, long length,
            Map<String, String> requestParameters) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setConnectTimeout(connectTimeoutMillis);
        connection.setReadTimeout(readTimeoutMillis);

        Map<String, String> requestHeaders = new HashMap<>();
        if (defaultRequestProperties != null) {
            requestHeaders.putAll(defaultRequestProperties.getSnapshot());
        }
        requestHeaders.putAll(requestProperties.getSnapshot());
        requestHeaders.putAll(requestParameters);

        for (Map.Entry<String, String> property : requestHeaders.entrySet()) {
            connection.setRequestProperty(property.getKey(), property.getValue());
        }

        @Nullable
        String rangeHeader = buildRangeRequestHeader(position, length);
        if (rangeHeader != null) {
            connection.setRequestProperty(HttpHeaders.RANGE, rangeHeader);
        }
        if (userAgent != null) {
            connection.setRequestProperty(HttpHeaders.USER_AGENT, userAgent);
        }
        connection.setDoOutput(httpBody != null);
        connection.setRequestMethod(DataSpec.getStringForHttpMethod(httpMethod));

        if (httpBody != null) {
            connection.setFixedLengthStreamingMode(httpBody.length);
            connection.connect();
            OutputStream os = connection.getOutputStream();
            os.write(httpBody);
            os.close();
        } else {
            connection.connect();
        }
        return connection;
    }

    /**
     * Reads up to {@code length} bytes of data and stores them into {@code buffer}, starting at
     * index
     * {@code offset}.
     *
     * <p>This method blocks until at least one byte of data can be read, the end of the opened
     * range is detected, or an exception is thrown.
     *
     * @param buffer     The buffer into which the read data should be stored.
     * @param offset     The start offset into {@code buffer} at which data should be written.
     * @param readLength The maximum number of bytes to read.
     * @return The number of bytes read, or {@link C#RESULT_END_OF_INPUT} if the end of the opened
     * range is reached.
     * @throws IOException If an error occurs reading from the source.
     */
    private int readInternal(byte[] buffer, int offset, int readLength) throws IOException {
        if (readLength == 0) {
            return 0;
        }

        Log.e("NTP", "readInternal");
        if (bytesToRead != C.LENGTH_UNSET) {
            long bytesRemaining = bytesToRead - bytesRead;
            if (bytesRemaining == 0) {
                return C.RESULT_END_OF_INPUT;
            }
            readLength = (int) min(readLength, bytesRemaining);
        }

        int read = castNonNull(inputStream).read(buffer, offset, readLength);
        if (read == -1) {
            return C.RESULT_END_OF_INPUT;
        }

        bytesRead += read;
        //        bytesTransferred(read);
        return read;
    }

    /**
     * Closes the current connection quietly, if there is one.
     */
    private void closeConnectionQuietly() {
        if (connection != null) {
            try {
                connection.disconnect();
            } catch (Exception e) {
                Log.e(TAG, "Unexpected error while disconnecting", e);
            }
            connection = null;
        }
    }

    /**
     * {@link DataSource.Factory} for {@link BraveChromiumHttpDataSource} instances.
     */
    public static final class Factory implements HttpDataSource.Factory {
        private final RequestProperties defaultRequestProperties;
        private final int connectTimeoutMs;
        private final int readTimeoutMs;
        @Nullable
        private TransferListener transferListener;
        @Nullable
        private Predicate<String> contentTypePredicate;
        @Nullable
        private String userAgent;
        private boolean allowCrossProtocolRedirects;
        private boolean keepPostFor302Redirects;

        /**
         * Creates an instance.
         */
        public Factory() {
            defaultRequestProperties = new RequestProperties();
            connectTimeoutMs = DEFAULT_CONNECT_TIMEOUT_MILLIS;
            readTimeoutMs = DEFAULT_READ_TIMEOUT_MILLIS;
        }

        @Override
        public BraveChromiumHttpDataSource.Factory setDefaultRequestProperties(
                Map<String, String> defaultRequestProperties) {
            this.defaultRequestProperties.clearAndSet(defaultRequestProperties);
            return this;
        }

        @Override
        public BraveChromiumHttpDataSource createDataSource() {
            BraveChromiumHttpDataSource dataSource =
                    new BraveChromiumHttpDataSource(userAgent, connectTimeoutMs, readTimeoutMs,
                            defaultRequestProperties, contentTypePredicate);
            if (transferListener != null) {
                dataSource.addTransferListener(transferListener);
            }
            return dataSource;
        }
    }
}
 
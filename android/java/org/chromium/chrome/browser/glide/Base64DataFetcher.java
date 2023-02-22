/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.glide;

import android.util.Base64;

import androidx.annotation.NonNull;

import com.bumptech.glide.Priority;
import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.data.DataFetcher;

import java.nio.ByteBuffer;

/**
 * Fetches the base64 section from a give string using a {@link ByteBuffer}.
 */
public class Base64DataFetcher implements DataFetcher<ByteBuffer> {
    private final String model;

    public Base64DataFetcher(String model) {
        this.model = model;
    }

    @Override
    public void loadData(
            @NonNull Priority priority, @NonNull DataCallback<? super ByteBuffer> callback) {
        try {
            String base64Section = getBase64SectionOfModel();
            byte[] data = Base64.decode(base64Section, Base64.DEFAULT);
            ByteBuffer byteBuffer = ByteBuffer.wrap(data);
            callback.onDataReady(byteBuffer);
        } catch (Exception ex) {
            callback.onLoadFailed(ex);
        }
    }

    @Override
    public void cleanup() {
        // Intentionally empty.
    }

    @Override
    public void cancel() {
        // Intentionally empty.
    }

    @NonNull
    @Override
    public Class<ByteBuffer> getDataClass() {
        return ByteBuffer.class;
    }

    @NonNull
    @Override
    public DataSource getDataSource() {
        return DataSource.REMOTE;
    }

    private String getBase64SectionOfModel() {
        // See https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/Data_URIs.
        int startOfBase64Section = model.indexOf(',');
        return model.substring(startOfBase64Section + 1);
    }
}

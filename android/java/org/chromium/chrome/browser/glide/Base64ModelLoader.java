/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.glide;

import androidx.annotation.NonNull;

import com.bumptech.glide.load.Options;
import com.bumptech.glide.load.model.ModelLoader;
import com.bumptech.glide.signature.ObjectKey;

import java.nio.ByteBuffer;

/**
 * Loads a {@link ByteBuffer} from a Base 64 encoded String.
 */
public final class Base64ModelLoader implements ModelLoader<String, ByteBuffer> {
    @NonNull
    @Override
    public LoadData<ByteBuffer> buildLoadData(
            @NonNull String model, int width, int height, @NonNull Options options) {
        return new LoadData<>(new ObjectKey(model), new Base64DataFetcher(model));
    }

    @Override
    public boolean handles(@NonNull String model) {
        return model.startsWith("data:");
    }
}

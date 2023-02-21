/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.glide;

import androidx.annotation.NonNull;

import com.bumptech.glide.load.model.ModelLoader;
import com.bumptech.glide.load.model.ModelLoaderFactory;
import com.bumptech.glide.load.model.MultiModelLoaderFactory;

import java.nio.ByteBuffer;

public class Base64ModelLoaderFactory implements ModelLoaderFactory<String, ByteBuffer> {
    @NonNull
    @Override
    public ModelLoader<String, ByteBuffer> build(@NonNull MultiModelLoaderFactory unused) {
        return new Base64ModelLoader();
    }

    @Override
    public void teardown() {
        // Do nothing.
    }
}
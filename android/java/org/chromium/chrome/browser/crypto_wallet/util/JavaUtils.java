/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import java.util.Collections;
import java.util.List;

public class JavaUtils {
    public static <T> List<T> safeVal(List<T> list) {
        if (list == null) return Collections.emptyList();
        return list;
    }

    @SuppressWarnings("unchecked")
    public static <T> T[] safeVal(T[] arr) {
        if (arr == null) return (T[]) new Object[0];
        return arr;
    }

    public static <T> T safeVal(T object, T defValue) {
        if (object == null) return defValue;
        return object;
    }
}

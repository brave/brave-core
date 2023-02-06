/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.function.Predicate;

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

    public static <T> List<T> filter(List<T> list, Predicate<T> filter) {
        List<T> filteredList = new ArrayList<>();
        for (T item : list) {
            if (filter.test(item)) {
                filteredList.add(item);
            }
        }
        return filteredList;
    }

    public static <T> T find(List<T> list, Predicate<T> predicate) {
        for (T item : list) {
            if (predicate.test(item)) {
                return item;
            }
        }
        return null;
    }

    public static <T> boolean includes(List<T> list, Predicate<T> predicate) {
        return find(list, predicate) != null;
    }

    public static <T> T find(T[] items, Predicate<T> predicate) {
        for (T item : items) {
            if (predicate.test(item)) {
                return item;
            }
        }
        return null;
    }

    public static <T> boolean includes(T[] items, Predicate<T> predicate) {
        return find(items, predicate) != null;
    }
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;

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

    /**
     * Return either the mapped value or null, without throwing any exception. Useful to get nested
     * value.
     *
     * @param val primary object to fetch value from
     * @param mapper function to get the value
     * @return either mapped value or null
     */
    public static <T, U> U safeVal(T val, Function<T, U> mapper) {
        try {
            return mapper.apply(val);
        } catch (Exception ignored) {
        }
        return null;
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

    public static <T> List<T> filter(T[] items, Predicate<T> filter) {
        return filter(Arrays.asList(items), filter);
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

    public static boolean anyNull(Object... items) {
        if (items == null) return true;
        for (Object item : items) {
            if (item == null) return true;
        }
        return false;
    }

    @SafeVarargs
    public static <T> T[] asArray(T... items) {
        return items;
    }

    /**
     * Returns a combined string with a separator or an empty string. Empty and null values are
     * ignored e.g. concatStrings(#,a,b,c,"",null) => a#b#c
     *
     * @param separator to append after each string.
     * @param items of string values.
     * @return a combined or empty string.
     */
    public static String concatStrings(String separator, String... items) {
        if (items == null) return "";
        return Arrays.stream(items)
                .filter(item -> !TextUtils.isEmpty(item))
                .collect(Collectors.joining(separator));
    }
}

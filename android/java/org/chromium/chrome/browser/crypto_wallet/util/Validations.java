/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.text.TextUtils;

import java.util.Locale;

public class Validations {
    /**
     * True when a sign message contains characters that can make its preview disagree with the
     * bytes being signed: null bytes, line breaks, other non-printable ASCII, or non-ASCII.
     */
    public static boolean hasHiddenCharacters(String... values) {
        for (String value : values) {
            if (TextUtils.isEmpty(value)) {
                continue;
            }
            for (int i = 0; i < value.length(); i++) {
                char c = value.charAt(i);
                if (c < 0x20 || c >= 0x7f) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Display-only rendering of a sign message. Null bytes become \0. Line breaks are kept, after a
     * \n, \r, \v or \f marker so they aren't mistaken for text wrapping. Other non-printable ASCII
     * and non-ASCII become \\uXXXX, and a literal backslash is doubled so it can't be mistaken for
     * one of those markers.
     *
     * <p>Uses the same mapping as desktop's unicodeEscape in
     * components/brave_wallet_ui/utils/string-utils.ts, so both platforms show the same markers.
     */
    public static String unicodeEscape(String str) {
        StringBuilder out = new StringBuilder(str.length());
        for (int i = 0; i < str.length(); i++) {
            char c = str.charAt(i);
            switch (c) {
                case '\\' -> out.append("\\\\");
                case 0 -> out.append("\\0");
                case '\n' -> out.append("\\n\n");
                case '\r' -> out.append("\\r\n");
                case 0x0b -> out.append("\\v\n");
                case '\f' -> out.append("\\f\n");
                default -> {
                    if (c < 0x20 || c >= 0x7f) {
                        out.append(String.format(Locale.ENGLISH, "\\u%04x", (int) c));
                    } else {
                        out.append(c);
                    }
                }
            }
        }
        return out.toString();
    }
}

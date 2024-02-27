/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

public class Validations {
    public static boolean hasUnicode(String str) {
        for (int i = 0; i < str.length(); i++) {
            if (str.codePointAt(i) > 127) {
                return true;
            }
        }

        return false;
    }

    public static String unicodeEscape(String str) {
        String res = "";

        for (int i = 0; i < str.length(); i++) {
            if (str.codePointAt(i) > 127) {
                String hexStr = Integer.toHexString(str.codePointAt(i));
                while (hexStr.length() < 4) {
                    hexStr = "0" + hexStr;
                }
                res += "\\u" + hexStr;
            } else {
                res += str.charAt(i);
            }
        }

        return res;
    }
}

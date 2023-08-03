/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.web_ui;

import android.app.Activity;

import org.chromium.chrome.browser.crypto_wallet.util.Utils;

/**
 * Type of WebUi Activity.
 * @see Utils#openBuySendSwapActivity(Activity, WebUiActivityType).
 */
public enum WebUiActivityType { BUY, SEND, SWAP }

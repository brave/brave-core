// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager.settings;

import org.chromium.build.annotations.NullMarked;

/**
 * An interface which a client can use to listen to changes to password and password exception
 * lists.
 */
@NullMarked
public interface PasswordListObserver {
    /**
     * Called when passwords list is updated.
     *
     * @param count Number of entries in the password list.
     */
    void passwordListAvailable(int count);

    /**
     * Called when password exceptions list is updated.
     *
     * @param count Number of entries in the password exception list.
     */
    void passwordExceptionListAvailable(int count);
}

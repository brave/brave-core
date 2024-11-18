/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package com.brave.playlist.listener

interface ItemInteractionListener {
    fun onItemDelete(position: Int) {}
    fun onRemoveFromOffline(position: Int) {}
    fun onShare(position: Int) {}
}

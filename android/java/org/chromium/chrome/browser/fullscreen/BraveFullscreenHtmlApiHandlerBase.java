/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.fullscreen;

/**
 * Super class of {@link FullscreenHtmlApiHandlerBase} introduced via bytecode changes. For more
 * info see {@link org.brave.bytecode.BraveFullscreenHtmlApiHandlerBaseClassAdapter}.
 */
public abstract class BraveFullscreenHtmlApiHandlerBase {
    /**
     * Field accessed using {@code BraveFullscreenHtmlApiHandlerBase.class.cast(this)}. Used to keep
     * track whether a tab was hidden by {@code TabHidingType.CHANGED_TABS} reason.
     *
     * <p>Note: {@link FullscreenHtmlApiHandlerBase} contains a change introduced by a patch.
     */
    protected boolean mTabHiddenByChangedTabs;
}

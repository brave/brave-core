/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

/** This is a handler for mojo connection failure for Brave News */
public class BraveNewsConnectionErrorHandler implements ConnectionErrorHandler {
    /** This is a delegate that is implemented in the object where the connection is created */
    public interface BraveNewsConnectionErrorHandlerDelegate {
        default void initBraveNewsControllerFromAWorkerThread() {}

        default void cleanUpBraveNewsController() {}
    }

    private final BraveNewsConnectionErrorHandlerDelegate mDelegate;

    public BraveNewsConnectionErrorHandler(BraveNewsConnectionErrorHandlerDelegate delegate) {
        assert delegate != null : "delegate has to be initialized";
        mDelegate = delegate;
    }

    @Override
    public void onConnectionError(MojoException e) {
        mDelegate.cleanUpBraveNewsController();
        mDelegate.initBraveNewsControllerFromAWorkerThread();
    }
}

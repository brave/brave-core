/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import org.chromium.ai_chat.mojom.CredentialManagerHelper;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

/** Helper to interact with ai_chat_credential_manager. */
@JNINamespace("ai_chat")
public class BraveLeoCMHelper implements ConnectionErrorHandler {
    private long mNativeAIChatCMHelperAndroid;
    CredentialManagerHelper mCredentialManagerHelper;

    private static final Object sLock = new Object();
    private static BraveLeoCMHelper sInstance;

    public static BraveLeoCMHelper getInstance(BrowserContextHandle browserContextHandle) {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveLeoCMHelper(browserContextHandle);
            }
        }
        return sInstance;
    }

    private BraveLeoCMHelper(BrowserContextHandle browserContextHandle) {
        mNativeAIChatCMHelperAndroid = BraveLeoCMHelperJni.get().init(browserContextHandle);
        long nativeHandle =
                BraveLeoCMHelperJni.get()
                        .getInterfaceToCredentialManagerHelper(mNativeAIChatCMHelperAndroid);
        MessagePipeHandle handle =
                CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
        mCredentialManagerHelper = CredentialManagerHelper.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) mCredentialManagerHelper).getProxyHandler();
        handler.setErrorHandler(this);
    }

    public void destroy() {
        synchronized (sLock) {
            sInstance = null;
            if (mNativeAIChatCMHelperAndroid == 0 && mCredentialManagerHelper == null) {
                return;
            }
            mCredentialManagerHelper.close();
            mCredentialManagerHelper = null;
            BraveLeoCMHelperJni.get().destroy(mNativeAIChatCMHelperAndroid);
            mNativeAIChatCMHelperAndroid = 0;
        }
    }

    public void getPremiumStatus(CredentialManagerHelper.GetPremiumStatus_Response callback) {
        if (mCredentialManagerHelper == null) {
            return;
        }
        mCredentialManagerHelper.getPremiumStatus(callback);
    }

    @Override
    public void onConnectionError(MojoException e) {
        destroy();
    }

    @NativeMethods
    public interface Natives {
        long init(BrowserContextHandle browserContextHandle);

        void destroy(long nativeAIChatCMHelperAndroid);

        long getInterfaceToCredentialManagerHelper(long nativeAIChatCMHelperAndroid);
    }
}

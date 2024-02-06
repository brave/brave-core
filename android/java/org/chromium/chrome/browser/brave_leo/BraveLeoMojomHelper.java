/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.ai_chat.mojom.AiChatAndroidHelper;
import org.chromium.ai_chat.mojom.ModelWithSubtitle;
import org.chromium.ai_chat.mojom.PremiumStatus;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

/**
 * Helper to interact with native AIChatMojomHelperAndroid. Check ai_chat_mojom_helper_android.h,
 * ai_chat_mojom_helper_android.cc for native parts
 */
@JNINamespace("ai_chat")
public class BraveLeoMojomHelper implements ConnectionErrorHandler {
    private long mNativeAIChatCMHelperAndroid;
    AiChatAndroidHelper mAIChatAndroidHelper;

    private static final Object sLock = new Object();
    private static BraveLeoMojomHelper sInstance;

    public static BraveLeoMojomHelper getInstance(BrowserContextHandle browserContextHandle) {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveLeoMojomHelper(browserContextHandle);
            }
        }
        return sInstance;
    }

    private BraveLeoMojomHelper(BrowserContextHandle browserContextHandle) {
        mNativeAIChatCMHelperAndroid = BraveLeoMojomHelperJni.get().init(browserContextHandle);
        long nativeHandle =
                BraveLeoMojomHelperJni.get()
                        .getInterfaceToAndroidHelper(mNativeAIChatCMHelperAndroid);
        MessagePipeHandle handle =
                CoreImpl.getInstance().acquireNativeHandle(nativeHandle).toMessagePipeHandle();
        mAIChatAndroidHelper = AiChatAndroidHelper.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) mAIChatAndroidHelper).getProxyHandler();
        handler.setErrorHandler(this);
    }

    public void destroy() {
        synchronized (sLock) {
            sInstance = null;
            if (mNativeAIChatCMHelperAndroid == 0 && mAIChatAndroidHelper == null) {
                return;
            }
            mAIChatAndroidHelper.close();
            mAIChatAndroidHelper = null;
            BraveLeoMojomHelperJni.get().destroy(mNativeAIChatCMHelperAndroid);
            mNativeAIChatCMHelperAndroid = 0;
        }
    }

    public void getPremiumStatus(AiChatAndroidHelper.GetPremiumStatus_Response callback) {
        if (mAIChatAndroidHelper == null) {
            callback.call(PremiumStatus.INACTIVE, null);
            return;
        }
        mAIChatAndroidHelper.getPremiumStatus(callback);
    }

    public void getModels(AiChatAndroidHelper.GetModelsWithSubtitles_Response callback) {
        if (mAIChatAndroidHelper == null) {
            callback.call(new ModelWithSubtitle[0]);
            return;
        }
        mAIChatAndroidHelper.getModelsWithSubtitles(callback);
    }

    @Override
    public void onConnectionError(MojoException e) {
        destroy();
    }

    @NativeMethods
    public interface Natives {
        long init(BrowserContextHandle browserContextHandle);

        void destroy(long nativeAIChatMojomHelperAndroid);

        long getInterfaceToAndroidHelper(long nativeAIChatMojomHelperAndroid);
    }
}

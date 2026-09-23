/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.ai_chat.mojom.AiChatSettingsHelper;
import org.chromium.ai_chat.mojom.ModelWithSubtitle;
import org.chromium.ai_chat.mojom.PremiumStatus;
import org.chromium.chrome.browser.browsing_data.TimePeriod;
import org.chromium.content_public.browser.BrowserContextHandle;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.bindings.Interface;
import org.chromium.mojo.bindings.Interface.Proxy.Handler;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.impl.CoreImpl;

/**
 * Helper to interact with native AIChatSettingsHelper. Check ai_chat_settings_helper.{h|cc}, for
 * native parts
 */
@JNINamespace("ai_chat")
public class BraveLeoMojomHelper implements ConnectionErrorHandler {
    private long mNativeAIChatCMHelperAndroid;
    AiChatSettingsHelper mAIChatAndroidHelper;

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
        mAIChatAndroidHelper = AiChatSettingsHelper.MANAGER.attachProxy(handle, 0);
        Handler handler = ((Interface.Proxy) mAIChatAndroidHelper).getProxyHandler();
        handler.setErrorHandler(this);
    }

    private void destroy() {
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

    public void getPremiumStatus(AiChatSettingsHelper.GetPremiumStatus_Response callback) {
        if (mAIChatAndroidHelper == null) {
            callback.call(PremiumStatus.INACTIVE, null);
            return;
        }
        mAIChatAndroidHelper.getPremiumStatus(callback);
    }

    public void createOrderId(AiChatSettingsHelper.CreateOrderId_Response callback) {
        if (mAIChatAndroidHelper == null) {
            callback.call("");
            return;
        }
        mAIChatAndroidHelper.createOrderId(callback);
    }

    public void fetchOrderCredentials(
            String orderId, AiChatSettingsHelper.FetchOrderCredentials_Response callback) {
        if (mAIChatAndroidHelper == null) {
            callback.call("{}");
            return;
        }
        mAIChatAndroidHelper.fetchOrderCredentials(orderId, callback);
    }

    public void refreshOrder(String orderId, AiChatSettingsHelper.RefreshOrder_Response callback) {
        if (mAIChatAndroidHelper == null) {
            callback.call("{}");
            return;
        }
        mAIChatAndroidHelper.refreshOrder(orderId, callback);
    }

    public void getModels(AiChatSettingsHelper.GetModelsWithSubtitles_Response callback) {
        if (mAIChatAndroidHelper == null) {
            callback.call(new ModelWithSubtitle[0]);
            return;
        }
        mAIChatAndroidHelper.getModelsWithSubtitles(callback);
    }

    public void getDefaultModelKey(AiChatSettingsHelper.GetDefaultModelKey_Response callback) {
        if (mAIChatAndroidHelper == null) {
            callback.call("");
            return;
        }
        mAIChatAndroidHelper.getDefaultModelKey(callback);
    }

    public void setDefaultModelKey(String modelKey) {
        if (mAIChatAndroidHelper == null) {
            return;
        }

        mAIChatAndroidHelper.setDefaultModelKey(modelKey);
    }

    public void deleteConversations(@TimePeriod int timePeriod) {
        if (mAIChatAndroidHelper == null) {
            return;
        }

        mAIChatAndroidHelper.deleteConversations(timePeriod);
    }

    @Override
    public void onConnectionError(MojoException e) {
        destroy();
    }

    @NativeMethods
    public interface Natives {
        long init(BrowserContextHandle browserContextHandle);

        void destroy(long nativeAIChatSettingsHelper);

        long getInterfaceToAndroidHelper(long nativeAIChatSettingsHelper);
    }
}

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.component_updater;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

// TODO(AlexeyBarabash): rename to BraveComponentUpdaterBridge?

/**
 * Class for handling progress of component update in Java.
 * Basically this is just bridge to native component updater classes.
 */
@JNINamespace("chrome::android")
public class BraveComponentUpdater {
    private static final String TAG = "ComponentUpdater";
    private long mNativeBraveComponentUpdaterAndroid;
    private static BraveComponentUpdater sBraveComponentUpdater;

    public static BraveComponentUpdater get() {
        ThreadUtils.assertOnUiThread();
        if (sBraveComponentUpdater == null) {
            sBraveComponentUpdater = new BraveComponentUpdater();
        }
        return sBraveComponentUpdater;
    }

    private BraveComponentUpdater() {
        init();
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveComponentUpdaterAndroid == 0;
        mNativeBraveComponentUpdaterAndroid = nativePtr;
    }

    private void init() {
        if (mNativeBraveComponentUpdaterAndroid == 0) {
            BraveComponentUpdaterJni.get().init(BraveComponentUpdater.this);
        }
    }

    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeBraveComponentUpdaterAndroid != 0) {
            BraveComponentUpdaterJni.get().destroy(mNativeBraveComponentUpdaterAndroid);
            mNativeBraveComponentUpdaterAndroid = 0;
        }
    }

    /**
     * Listener for the component update changes.
     */
    public interface ComponentUpdaterListener {
        // Invoked when component update info has changed.
        public void onComponentUpdateEvent(int event, String id);
    }

    private final List<ComponentUpdaterListener> mComponentUpdaterListeners =
            new CopyOnWriteArrayList<ComponentUpdaterListener>();

    public void addComponentUpdateEventListener(ComponentUpdaterListener listener) {
        ThreadUtils.assertOnUiThread();
        mComponentUpdaterListeners.add(listener);
    }

    public void removeComponentUpdateEventListener(ComponentUpdaterListener listener) {
        ThreadUtils.assertOnUiThread();
        mComponentUpdaterListeners.remove(listener);
    }

    @CalledByNative
    protected void componentStateUpdated(int event, String id) {
        for (ComponentUpdaterListener listener : mComponentUpdaterListeners) {
            listener.onComponentUpdateEvent(event, id);
        }
    }

    /**
     * Class describing the progress of component download.
     */
    public class CrxUpdateItem {
        public String mId;
        public long mDownloadedBytes = -1;
        public long mTotalBytes = -1;
    }

    public CrxUpdateItem getUpdateState(String id) {
        String json = BraveComponentUpdaterJni.get().getUpdateState(
                mNativeBraveComponentUpdaterAndroid, id);

        CrxUpdateItem crxUpdateItem = new CrxUpdateItem();
        try {
            JSONObject result = new JSONObject(json);
            crxUpdateItem.mId = result.getString("id");
            crxUpdateItem.mDownloadedBytes = (long) result.getDouble("downloaded_bytes");
            crxUpdateItem.mTotalBytes = (long) result.getDouble("total_bytes");
        } catch (JSONException e) {
            Log.e(TAG, "getUpdateState JSONException error " + e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "getUpdateState IllegalStateException error " + e);
        }

        return crxUpdateItem;
    }

    @NativeMethods
    interface Natives {
        void init(BraveComponentUpdater caller);
        void destroy(long nativeBraveComponentUpdaterAndroid);
        String getUpdateState(long nativeBraveComponentUpdaterAndroid, String id);
    }
}

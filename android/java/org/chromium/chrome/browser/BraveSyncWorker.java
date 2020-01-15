/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.os.Looper;
import android.util.Base64;
import android.util.JsonReader;
import android.util.JsonToken;
import android.webkit.JavascriptInterface;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.task.AsyncTask;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.components.bookmarks.BookmarkId;
import org.chromium.components.bookmarks.BookmarkType;
import org.chromium.components.url_formatter.UrlFormatter;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge;
import org.chromium.chrome.browser.bookmarks.BookmarkModel;
import org.chromium.chrome.browser.bookmarks.BookmarkUtils;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkItem;
import org.chromium.chrome.browser.bookmarks.BookmarkBridge.BookmarkModelObserver;
import org.chromium.chrome.browser.bookmarks.BraveBookmarkModel;
import org.chromium.chrome.browser.bookmarks.BraveBookmarkUtils;
import org.chromium.chrome.browser.partnerbookmarks.PartnerBookmarksShim;
import org.chromium.chrome.browser.preferences.BraveSyncScreensObserver;
import org.chromium.chrome.browser.WebContentsFactory;
import org.chromium.content_public.browser.JavascriptInjector;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.components.embedder_support.view.ContentView;
import org.chromium.content_public.browser.ViewEventSink;
import org.chromium.content.browser.ViewEventSinkImpl;
import org.chromium.chrome.browser.ChromeVersionInfo;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.base.ViewAndroidDelegate;
import org.chromium.ui.base.WindowAndroid;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.IllegalArgumentException;
import java.lang.Runnable;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.HashSet;
import java.util.HashMap;
import java.util.Random;
import java.util.Scanner;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.UnsupportedEncodingException;


@JNINamespace("chrome::android")
public class BraveSyncWorker {
    public static final String TAG = "SYNC";
    public static final String PREF_NAME = "SyncPreferences";
    private static final String PREF_LAST_FETCH_NAME = "TimeLastFetch";
    private static final String PREF_LATEST_DEVICE_RECORD_TIMESTAMPT_NAME = "LatestDeviceRecordTime";
    private static final String PREF_LAST_TIME_SEND_NOT_SYNCED_NAME = "TimeLastSendNotSynced";
    public static final String PREF_DEVICE_ID = "DeviceId";
    public static final String PREF_BASE_ORDER = "BaseOrder";
    public static final String PREF_LAST_ORDER = "LastOrder";
    public static final String PREF_SEED = "Seed";
    public static final String PREF_SYNC_DEVICE_NAME = "SyncDeviceName";
    private static final int SYNC_SLEEP_ATTEMPTS_COUNT = 20;
    private static final int INTERVAL_TO_FETCH_RECORDS = 1000 * 60;    // Milliseconds
    private static final int INTERVAL_TO_SEND_SYNC_RECORDS = 1000 * 60;    // Milliseconds
    private static final int INTERVAL_TO_REFETCH_RECORDS = 10000 * 60;    // Milliseconds
    private static final Long INTERVAL_RESEND_NOT_SYNCED = 1000L * 60L * 10L; // 10 minutes
    private static final int SEND_RECORDS_COUNT_LIMIT = 1000;
    private static final int FETCH_RECORDS_CHUNK_SIZE = 300;
    private static final String PREF_SYNC_SWITCH = "sync_switch";
    private static final String PREF_SYNC_BOOKMARKS = "brave_sync_bookmarks";
    public static final String PREF_SYNC_TABS = "brave_sync_tabs";
    public static final String PREF_SYNC_HISTORY = "brave_sync_history";
    public static final String PREF_SYNC_AUTOFILL_PASSWORDS = "brave_sync_autofill_passwords";
    public static final String PREF_SYNC_PAYMENT_SETTINGS = "brave_sync_payment_settings";
    public static final String CREATE_RECORD = "0";
    public static final String UPDATE_RECORD = "1";
    public static final String DELETE_RECORD = "2";
    private static final int ATTEMPTS_BEFORE_SENDING_NOT_SYNCED_RECORDS = 1;

    private final SharedPreferences mSharedPreferences;

    private static final String ANDROID_SYNC_JS = "android_sync.js";
    private static final String BUNDLE_JS = "bundle.js";
    private static final String CRYPTO_JS = "crypto.js";
    private static final String ANDROID_SYNC_WORDS_JS = "android_sync_words.js";

    private static final String ORIGINAL_SEED_KEY = "originalSeed";
    private static final String DEVICES_NAMES = "devicesNames";
    private static final String ORPHAN_BOOKMARKS = "orphanBookmarks";
    private static final String THIS_DEVICE_OBJECT_ID = "thisDeviceObjectId";
    public static final int NICEWARE_WORD_COUNT = 16;
    public static final int BIP39_WORD_COUNT = 24;

    private SyncThread mSyncThread;
    private SendSyncDataThread mSendSyncDataThread;

    private Context mContext;
    private boolean mStopThread;
    private SyncIsReady mSyncIsReady;

    private String mSeed;
    private String mDeviceId;
    private String mDeviceName;
    private String mApiVersion;
    private String mBaseOrder;
    private String mLastOrder;
    //private String mServerUrl = "https://sync-staging.brave.com";
    private String mServerUrl = "https://sync.brave.com";
    private String mDebug = "true";
    private long mTimeLastFetch;   // In milliseconds
    private long mTimeLastFetchExecuted;   // In milliseconds
    private String mLatestRecordTimeStampt = "";
    private boolean mFetchInProgress;
    private BookmarkId mDefaultFolder;
    private BraveBookmarkModel mNewBookmarkModel;
    private boolean mInterruptSyncSleep;

    private BraveSyncScreensObserver mSyncScreensObserver;

    private ArrayList<ResolvedRecordToApply> mOrphanBookmarks = new ArrayList<ResolvedRecordToApply>();

    private WebContents mWebContents;
    private JavascriptInjector mWebContentsInjector;
    private ViewEventSinkImpl mViewEventSink;
    private WebContents mJSWebContents;
    private JavascriptInjector mJSWebContentsInjector;
    private ViewEventSinkImpl mJSViewEventSink;
    private boolean mReorderBookmarks;
    private int mAttepmtsBeforeSendingNotSyncedRecords = ATTEMPTS_BEFORE_SENDING_NOT_SYNCED_RECORDS;
    private String mBulkBookmarkOperations = "";
    private String mLatestFetchRequest = "";

    enum NotSyncedRecordsOperation {
        GetItems, AddItems, DeleteItems
    }

    public static class SyncRecordType {
        public static final String BOOKMARKS = "BOOKMARKS";
        public static final String HISTORY = "HISTORY_SITES";
        public static final String PREFERENCES = "PREFERENCES";

        public static String GetRecordTypeJSArray(String recordType) {
            if (recordType.isEmpty() || (!recordType.equals(BOOKMARKS) &&
                !recordType.equals(HISTORY) && !recordType.equals(PREFERENCES))) {
                assert false;
            }
            return "['" + recordType + "']";
        }
    }

    public static class SyncObjectData {
        public static final String BOOKMARK = "bookmark";
        public static final String HISTORY_SITE = "historySite";
        public static final String SITE_SETTING = "siteSetting";
        public static final String DEVICE = "device";
    }

    class SyncIsReady {

        public boolean mFetchRecordsReady;
        public boolean mResolveRecordsReady;
        public boolean mSendRecordsReady;
        public boolean mDeleteUserReady;
        public boolean mDeleteCategoryReady;
        public boolean mDeleteSiteSettingsReady;
        public boolean mReady;
        public boolean mShouldResetSync;

        public SyncIsReady() {
            mFetchRecordsReady = false;
            mResolveRecordsReady = false;
            mSendRecordsReady = false;
            mDeleteUserReady = false;
            mDeleteCategoryReady = false;
            mDeleteSiteSettingsReady = false;
            mReady = false;
            mShouldResetSync = false;
        }

        public boolean IsReady() {
            return mReady && mFetchRecordsReady && mResolveRecordsReady
                && mSendRecordsReady && mDeleteUserReady && mDeleteCategoryReady
                && mDeleteSiteSettingsReady && !mShouldResetSync;
        }

        public void Reset() {
            mFetchRecordsReady = false;
            mResolveRecordsReady = false;
            mSendRecordsReady = false;
            mDeleteUserReady = false;
            mDeleteCategoryReady = false;
            mDeleteSiteSettingsReady = false;
            mReady = false;
        }
    }

    class BookmarkInternal {
        public String mUrl = "";
        public String mTitle = "";
        public String mCustomTitle = "";
        public String mParentFolderObjectId = "";
        public boolean mIsFolder;
        public long mLastAccessedTime;
        public long mCreationTime;
        public String mFavIcon = "";
        public String mOrder = "";

        public BookmarkInternal() {
            mIsFolder = false;
            mLastAccessedTime = 0;
            mCreationTime = 0;
        }

        public JSONObject toJSONObject() {
            JSONObject jsonObject = new JSONObject();
            try {
                jsonObject.put("url", mUrl);
                jsonObject.put("title", mTitle);
                jsonObject.put("customTitle", mCustomTitle);
                jsonObject.put("parentFolderObjectId", mParentFolderObjectId);
                jsonObject.put("isFolder", mIsFolder);
                jsonObject.put("lastAccessedTime", mLastAccessedTime);
                jsonObject.put("creationTime", mCreationTime);
                jsonObject.put("favIcon", mFavIcon);
                jsonObject.put("order", mOrder);
            } catch (JSONException e) {
                Log.e(TAG, "BookmarkInternal toJSONObject error: " + e);
            }
            return jsonObject;
        }
    }

    private BookmarkInternal BookmarkInternalFromJSONObject(JSONObject jsonObject) {
        BookmarkInternal bookmarkInternal = new BookmarkInternal();
        try {
            if (jsonObject.has("url")) {
                bookmarkInternal.mUrl = jsonObject.getString("url");
            }
            if (jsonObject.has("title")) {
                bookmarkInternal.mTitle = jsonObject.getString("title");
            }
            if (jsonObject.has("customTitle")) {
                bookmarkInternal.mCustomTitle = jsonObject.getString("customTitle");
            }
            if (jsonObject.has("parentFolderObjectId")) {
                bookmarkInternal.mParentFolderObjectId = jsonObject.getString("parentFolderObjectId");
            }
            if (jsonObject.has("isFolder")) {
                bookmarkInternal.mIsFolder = jsonObject.getBoolean("isFolder");
            }
            if (jsonObject.has("lastAccessedTime")) {
                bookmarkInternal.mLastAccessedTime = jsonObject.getLong("lastAccessedTime");
            }
            if (jsonObject.has("creationTime")) {
                bookmarkInternal.mCreationTime = jsonObject.getLong("creationTime");
            }
            if (jsonObject.has("favIcon")) {
                bookmarkInternal.mFavIcon = jsonObject.getString("favIcon");
            }
            if (jsonObject.has("order")) {
                bookmarkInternal.mOrder = jsonObject.getString("order");
            }
        } catch (JSONException e) {
            Log.e(TAG, "BookmarkInternalFromJSONObject error: " + e);
        }
        return bookmarkInternal;
    }

    public class OrderedBookmark implements Comparable<OrderedBookmark> {
        private BookmarkItem bookmark;
        private String order;

        public OrderedBookmark(BookmarkItem bookmark, String order){
            this.bookmark = bookmark;
            this.order = order;
        }

        public BookmarkItem Bookmark() {
            return bookmark;
        }

        @Override
        public int compareTo(OrderedBookmark compare) {
            if (order.isEmpty() || compare.order.isEmpty()) {
                Log.e(TAG, "Incorrect bookmark order");
                // This should not happen
                assert false;
                return 0;
            }
            String[] thisNumbers = order.split("\\.");
            String[] compareNumbers = compare.order.split("\\.");
            int maxSize = thisNumbers.length > compareNumbers.length ? compareNumbers.length : thisNumbers.length;
            for (int i = 0; i < maxSize; i++) {
                if (Integer.parseInt(thisNumbers[i]) > Integer.parseInt(compareNumbers[i])) {
                    return 1;
                } else if (Integer.parseInt(thisNumbers[i]) < Integer.parseInt(compareNumbers[i])){
                    return -1;
                }
            }
            // It means we have equal start parts(ex. 2.2.1.1 vs 2.2.1)
            if (thisNumbers.length > compareNumbers.length) {
                return -1;
            } else if (compareNumbers.length > thisNumbers.length) {
                return 1;
            }
            Log.e(TAG, "Bookmark compare improper state");
            Log.e(TAG, "order == " + order);
            Log.e(TAG, "compare.order == " + compare.order);
            // This should not happen
            assert false;
            return 0;
        }
    }

    public class ResolvedRecordToApply implements Comparable<ResolvedRecordToApply> {
        public ResolvedRecordToApply(String objectId, String action, BookmarkInternal bookMarkInternal, String deviceName, String deviceId, long syncTime) {
            mObjectId = objectId;
            mAction = action;
            mBookmarkInternal = bookMarkInternal;
            mDeviceName = deviceName;
            mDeviceId = deviceId;
            mSyncTime = syncTime;
        }

        public String mObjectId;
        public String mAction;
        public BookmarkInternal mBookmarkInternal;
        public String mDeviceName;
        public String mDeviceId;
        public long mSyncTime;

        @Override
        public int compareTo(ResolvedRecordToApply compare) {
            if (mSyncTime > compare.mSyncTime) {
                return 1;
            } else if (mSyncTime < compare.mSyncTime) {
                return -1;
            } else {
                return 0;
            }
        }

        public JSONObject toJSONObject() {
            JSONObject jsonObject = new JSONObject();
            try {
                jsonObject.put("objectId", mObjectId);
                jsonObject.put("action", mAction);
                jsonObject.put("deviceName", mDeviceName);
                jsonObject.put("deviceId", mDeviceId);
                jsonObject.put("syncTime", mSyncTime);
                if (mBookmarkInternal != null) {
                    jsonObject.put("bookmarkInternal", mBookmarkInternal.toJSONObject());
                }
            } catch (JSONException e) {
                Log.e(TAG, "ResolvedRecordToApply toJSONObject error: " + e);
            }
            return jsonObject;
        }
    }

    private ResolvedRecordToApply ResolvedRecordToApplyFromJSONObject(JSONObject jsonObject) {
        String objectId = "";
        String action = "";
        String deviceName = "";
        String deviceId = "";
        long syncTime = 0;
        BookmarkInternal bookmarkInternal = null;
        try {
            if (jsonObject.has("objectId")) {
                objectId = jsonObject.getString("objectId");
            }
            if (jsonObject.has("action")) {
                action = jsonObject.getString("action");
            }
            if (jsonObject.has("deviceName")) {
                deviceName = jsonObject.getString("deviceName");
            }
            if (jsonObject.has("deviceId")) {
                deviceId = jsonObject.getString("deviceId");
            }
            if (jsonObject.has("syncTime")) {
                syncTime = jsonObject.getLong("syncTime");
            }
            if (jsonObject.has("bookmarkInternal")) {
                String bookmark = jsonObject.getString("bookmarkInternal");
                JSONObject jsonBookmark = new JSONObject(bookmark);
                bookmarkInternal = BookmarkInternalFromJSONObject(jsonBookmark);
            }
        } catch (JSONException e) {
            Log.e(TAG, "ResolvedRecordToApplyFromJSONObject error: " + e);
        }
        return new ResolvedRecordToApply(objectId, action, bookmarkInternal, deviceName, deviceId, syncTime);
    }


    public BraveSyncWorker(Context context) {
        mStopThread = false;
        mSeed = null;
        mDeviceId = null;
        mDeviceName = null;
        mApiVersion = "0";
        mBaseOrder = null;
        mLastOrder = null;
        mReorderBookmarks = false;
        mContext = context;
        mTimeLastFetch = 0;
        mTimeLastFetchExecuted = 0;
        mFetchInProgress = false;
        mNewBookmarkModel = null;
        mInterruptSyncSleep = false;
        mWebContents = null;
        mWebContentsInjector = null;
        mViewEventSink = null;
        mJSWebContents = null;
        mJSWebContentsInjector = null;
        mJSViewEventSink = null;
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
        mSyncIsReady = new SyncIsReady();
        mSendSyncDataThread = new SendSyncDataThread();
        if (null != mSendSyncDataThread) {
            mSendSyncDataThread.start();
        }
        mSyncThread = new SyncThread();
        if (null != mSyncThread) {
            mSyncThread.start();
        }
        GetDefaultFolderId();
    }

    private String convertStreamToString(InputStream is) {
        Scanner s = new Scanner(is).useDelimiter("\\A");
        return s.hasNext() ? s.next() : "";
    }

    public void Stop() {
        mStopThread = true;
        if (null != mNewBookmarkModel) {
            mNewBookmarkModel.destroy();
            mNewBookmarkModel = null;
        }
        if (mSyncThread != null) {
            mSyncThread.interrupt();
            mSyncThread = null;
        }
        if (mSendSyncDataThread != null) {
            mSendSyncDataThread.interrupt();
            mSendSyncDataThread = null;
        }
        new Thread() {
            @Override
            public void run() {
                nativeClear();
            }
        }.start();
    }

    public void CreateUpdateDeleteBookmarks(String action, BookmarkItem[] bookmarks, final boolean addIdsToNotSynced,
              final boolean isInitialSync) {
        assert null != bookmarks;
        if (null == bookmarks || 0 == bookmarks.length || !SyncHasBeenSetup() || !IsSyncBookmarksEnabled()) {
            return;
        }

        // alexeyb: this means we had acquired mNewBookmarkModel
        // and we cannot acquire mNewBookmarkModel in GetBookmarkIdRunnable
        // because we will wait GetBookmarkIdRunnable completion
        // acuiring of mNewBookmarkModel happens above on call-tree
        // in SendAllLocalBookmarks on initial sync
        final boolean newBookmarkModelAcquiredByThisRunnableWaiter = isInitialSync;

        final String actionFinal = action;
        final HashSet<Long> processedFolderIds = new HashSet<Long>();
        final long defaultFolderId = (null != mDefaultFolder ? mDefaultFolder.getId() : 0);
        final List<BookmarkItem> bookmarksParentFolders = new ArrayList<BookmarkItem>();
        boolean uiThread = (Looper.myLooper() == Looper.getMainLooper());
        if (!actionFinal.equals(DELETE_RECORD)) {
            // Fill parent folders recursively, if it's not delete operation
            for (int i = 0; i < bookmarks.length; i++) {
                long processedId = bookmarks[i].getParentId().getId();
                if (defaultFolderId == processedId) {
                    continue;
                }
                int currentSize = bookmarksParentFolders.size();
                if (!processedFolderIds.contains(processedId)) {
                    BookmarkItem item = !uiThread ? GetBookmarkItemByLocalId(String.valueOf(processedId), newBookmarkModelAcquiredByThisRunnableWaiter) :
                        BookmarkItemByBookmarkId(processedId, newBookmarkModelAcquiredByThisRunnableWaiter);
                    while (item != null && !item.getTitle().isEmpty()) {
                        processedFolderIds.add(processedId);
                        bookmarksParentFolders.add(currentSize, item);
                        processedId = item.getParentId().getId();
                        if (processedFolderIds.contains(processedId)) {
                            break;
                        }
                        if (defaultFolderId == processedId) {
                            break;
                        }
                        item = !uiThread ? GetBookmarkItemByLocalId(String.valueOf(processedId), newBookmarkModelAcquiredByThisRunnableWaiter) :
                            BookmarkItemByBookmarkId(processedId, newBookmarkModelAcquiredByThisRunnableWaiter);
                    }
                }
            }
        }
        final BookmarkItem[] bookmarksFinal = bookmarks;

        new Thread() {
            @Override
            public void run() {
                if (!SyncHasBeenSetup()) {
                    return;
                }

                ArrayList<String> ids = new ArrayList<String>();
                StringBuilder bookmarkRequest = new StringBuilder("");
                boolean comesFromPreviousSeed = false;
                if (isInitialSync) {
                    String originalSeed = GetObjectId(ORIGINAL_SEED_KEY);
                    if (originalSeed.equals(mSeed)) {
                        comesFromPreviousSeed = true;
                    }
                }
                if (actionFinal.equals(DELETE_RECORD)) {
                    // On delete we just process delete items
                    formRequestForBookmarks(bookmarksFinal, processedFolderIds, comesFromPreviousSeed, actionFinal, true, bookmarkRequest, ids);
                } else {
                    // On other cases we process parent folders first
                    formRequestForParrentFolders(bookmarksParentFolders, isInitialSync, comesFromPreviousSeed, bookmarkRequest, ids);
                    formRequestForBookmarks(bookmarksFinal, processedFolderIds, comesFromPreviousSeed, actionFinal, false, bookmarkRequest, ids);
                }
                if (bookmarkRequest.length() == 0) {
                    // Nothing to send
                    return;
                }
                //Log.i(TAG, "!!!bookmarkRequest == " + bookmarkRequest);
                SendSyncRecords(SyncRecordType.BOOKMARKS, bookmarkRequest, actionFinal, ids);
            }

            private void formRequestForParrentFolders(List<BookmarkItem> bookmarksParentFolders, boolean isInitialSync, boolean comesFromPreviousSeed,
                                                        StringBuilder bookmarkRequest, ArrayList<String> ids) {
                for (BookmarkItem bookmarkFolder : bookmarksParentFolders) {
                    String localId = String.valueOf(bookmarkFolder.getId().getId());
                    String objectId = GetObjectId(localId);
                    if (!isInitialSync && !objectId.isEmpty()
                          || isInitialSync && comesFromPreviousSeed) {
                        continue;
                    }

                    bookmarkRequest.append(formRequestByBookmarkItem(bookmarkFolder, bookmarkRequest.length() <= 1, CREATE_RECORD, defaultFolderId, comesFromPreviousSeed));
                    if (addIdsToNotSynced) {
                        ids.add(localId);
                    }
                }
            }

            private void formRequestForBookmarks(BookmarkItem[] bookmarksFinal, HashSet<Long> processedFolderIds, boolean comesFromPreviousSeed, String actionFinal, boolean deleteOperation,
                                                  StringBuilder bookmarkRequest, ArrayList<String> ids) {
                if (deleteOperation) {
                    // Delete operation we perform in reverse order
                    for (int i = bookmarksFinal.length - 1; i >=0; i--) {
                        bookmarkRequest.append(formRequestByBookmarkItem(bookmarksFinal[i], bookmarkRequest.length() <= 1, actionFinal, defaultFolderId, comesFromPreviousSeed));
                        if (addIdsToNotSynced) {
                            ids.add(String.valueOf(bookmarksFinal[i].getId().getId()));
                        }
                    }
                } else {
                    for (int i = 0; i < bookmarksFinal.length; i++) {
                        if (bookmarksFinal[i].isFolder() && processedFolderIds.contains(bookmarksFinal[i].getId().getId())) {
                            continue;
                        }
                        bookmarkRequest.append(formRequestByBookmarkItem(bookmarksFinal[i], bookmarkRequest.length() <= 1, actionFinal, defaultFolderId, comesFromPreviousSeed));
                        if (addIdsToNotSynced) {
                            ids.add(String.valueOf(bookmarksFinal[i].getId().getId()));
                        }
                    }
                }
            }

            private StringBuilder formRequestByBookmarkItem(BookmarkItem bookmarkItem, boolean firstRecord, String action,
                    long defaultFolderId, boolean comesFromPreviousSeed) {
                StringBuilder bookmarkRequest = new StringBuilder("");
                String localId = String.valueOf(bookmarkItem.getId().getId());
                String objectId = GetObjectId(localId);
                boolean objectExist = !objectId.isEmpty();
                if (!objectExist && action.equals(DELETE_RECORD)) {
                    // Do not create an object on delete
                    return bookmarkRequest;
                }
                if (objectExist && isInitialSync && comesFromPreviousSeed) {
                    return new StringBuilder("");
                }
                if (!objectExist) {
                    objectId = GenerateObjectId(localId);
                }
                if (!firstRecord) {
                    bookmarkRequest.append(", ");
                }
                String order = GetBookmarkOrder(localId, !objectExist);
                if (order.isEmpty()) {
                    Log.e(TAG, "formRequestByBookmarkItem empty order");
                    assert false;
                }
                long parentId = bookmarkItem.getParentId().getId();
                bookmarkRequest.append(CreateRecord(objectId, SyncObjectData.BOOKMARK, action, mDeviceId, 0));
                bookmarkRequest.append(CreateBookmarkRecord(bookmarkItem.getUrl(),
                    bookmarkItem.getTitle(), bookmarkItem.isFolder(),
                    parentId, "", "", 0, 0, "", order));
                bookmarkRequest.append("}");
                if (!objectExist) {
                    //Log.i(TAG, "Saving object [" + bookmarkItem.getId().getId() + ", " + bookmarkItem.isFolder() + ", " + order + "]: " + objectId);
                    SaveObjectId(String.valueOf(bookmarkItem.getId().getId()), objectId, order, true);
                    // alexeyb - dont do reorder here
                    // when device is connected to sync and already has some bookmarks,
                    // code in formRequestByBookmarkItem gets order for current bookmarkItem
                    // and some bookmarks will not have order
                    // but ReorderBookmarks expects all items have the order
                    // either disable ReorderBookmarks below or make it ready to accept empty orders

                    // alexeyb: on initial sync if device already has some bookmarks,
                    // code in formRequestByBookmarkItem gets order for current bookmarkItem
                    // and some bookmarks will not have order
                    // but ReorderBookmarks expects all items have the order
                    // other way is to make ReorderBookmarks accept empty orders
                    if (!isInitialSync) {
                      ReorderBookmarks();
                    }
                }
                // We will delete the objectId when we ensure that records were transferred
                /*else if (action.equals(DELETE_RECORD)) {
                    nativeDeleteByLocalId(localId);
                }*/

                return bookmarkRequest;
            }
        }.start();
    }

    public void DeleteBookmarks(BookmarkItem[] bookmarks) {
        if (!IsSyncEnabled()) {
            if (0 == mTimeLastFetch && 0 == mTimeLastFetchExecuted) {
                return;
            }
        }
        CreateUpdateDeleteBookmarks(DELETE_RECORD, bookmarks, true, false);
    }

    public void CreateUpdateBookmark(boolean bCreate, BookmarkItem bookmarkItem) {
        if (!IsSyncEnabled()) {
            if (0 == mTimeLastFetch && 0 == mTimeLastFetchExecuted) {
                return;
            }
        }
        BookmarkItem[] bookmarks = new BookmarkItem[1];
        bookmarks[0] = bookmarkItem;
        CreateUpdateDeleteBookmarks((bCreate ? CREATE_RECORD : UPDATE_RECORD), bookmarks, true, false);
    }

    private StringBuilder CreateRecord(String objectId, String objectData, String action, String deviceId, long syncTime) {
        StringBuilder record = new StringBuilder("{ action: ");
        record.append(action).append(", ");
        record.append("deviceId: [").append(deviceId).append("], ");
        record.append("objectId: [").append(objectId).append("], ");
        record.append("objectData: '").append(objectData).append("', ");
        record.append("syncTimestamp: ").append(syncTime).append(", ");

        return record;
    }

    private StringBuilder CreateDeviceCreationRecord(String deviceName, String objectId, String action, String deviceId) {
        //Log.i(TAG, "CreateDeviceCreationRecord: " + deviceName);
        assert !deviceName.isEmpty();
        if (deviceName.isEmpty()) {
            return new StringBuilder(deviceName);
        }
        StringBuilder record = new StringBuilder("{ action: ").append(action).append(", ");
        record.append("deviceId: [").append(deviceId).append("], ");
        record.append("objectId: [").append(objectId).append("], ");
        record.append(SyncObjectData.DEVICE).append(": { name: \"").append(replaceUnsupportedCharacters(deviceName)).append("\"}}");

        //Log.i(TAG, "!!!device record == " + record);
        return record;
    }

    private String replaceUnsupportedCharacters(String in) {
      return in.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    private String UrlEncode(String url) {
        try {
            String urlEncoded = URLEncoder.encode(url, "utf-8");
            return urlEncoded;
        } catch (UnsupportedEncodingException ex) {
            return url;
        }
    }

    private StringBuilder CreateBookmarkRecord(String url, String title, boolean isFolder, long parentFolderId,
              String parentFolderObjectId, String customTitle, long lastAccessedTime, long creationTime, String favIcon,
              String order) {
        StringBuilder bookmarkRequest = new StringBuilder("bookmark:");
        bookmarkRequest.append("{ site:");
        bookmarkRequest.append("{ location: \"").append(UrlEncode(url)).append("\", ");
        if (!isFolder) {
            bookmarkRequest.append("title: \"").append(replaceUnsupportedCharacters(title)).append("\", ");
            bookmarkRequest.append("customTitle: \"").append(replaceUnsupportedCharacters(customTitle)).append("\", ");
        } else {
            bookmarkRequest.append("title: \"\", ");
            if (!customTitle.isEmpty()) {
                bookmarkRequest.append("customTitle: \"").append(replaceUnsupportedCharacters(customTitle)).append("\", ");
            } else {
                bookmarkRequest.append("customTitle: \"").append(replaceUnsupportedCharacters(title)).append("\", ");
            }
        }
        bookmarkRequest.append("favicon: \"").append(UrlEncode(favIcon)).append("\", ");
        bookmarkRequest.append("lastAccessedTime: ").append(lastAccessedTime).append(", ");
        bookmarkRequest.append("creationTime: ").append(creationTime).append("}, ");
        bookmarkRequest.append("isFolder: ").append(isFolder).append(", ");
        bookmarkRequest.append("order: \"").append(order).append("\", ");
        long defaultFolderId = (null != mDefaultFolder ? mDefaultFolder.getId() : 0);
        String parentObjectId = parentFolderObjectId;
        if (defaultFolderId != parentFolderId) {
            parentObjectId = "[" + GetObjectId(String.valueOf(parentFolderId)) + "]";
            assert !parentObjectId.isEmpty();
        }
        if (parentObjectId.isEmpty() || parentObjectId.length() <= 2) {
            parentObjectId = "null";
        }
        bookmarkRequest.append("parentFolderObjectId: ").append(parentObjectId).append("}");

        return bookmarkRequest;
    }

    private StringBuilder CreateDeviceRecord(String deviceName) {
      StringBuilder deviceRequest = new StringBuilder("device:");
      deviceRequest.append("{ name:\"").append(replaceUnsupportedCharacters(deviceName)).append("\"}");

      return deviceRequest;
    }

    private String GetDeviceNameByObjectId(String objectId) {
        String object = nativeGetObjectIdByLocalId(DEVICES_NAMES);
        if (object.isEmpty()) {
            return "";
        }

        String res = "";
        try {
            //Log.i(TAG, "GetDeviceNameByObjectId: trying to read JSON: " + object);
            JSONObject result = new JSONObject(object);
            JSONArray devices = result.getJSONArray("devices");
            for (int i = 0; i < devices.length(); i++) {
                JSONObject device = devices.getJSONObject(i);
                String currentObject = device.getString("objectId");
                if (currentObject.equals(objectId)) {
                    res = device.getString("name");
                    break;
                }
            }
        } catch (JSONException e) {
            Log.e(TAG, "GetDeviceNameByObjectId JSONException error " + e);
        } catch (IllegalStateException e) {
              Log.e(TAG, "GetDeviceNameByObjectId IllegalStateException error " + e);
        }

        //Log.i(TAG, "!!!GetDeviceNameByObjectId res == " + res);

        return res;
    }

    private String GetDeviceObjectIdByLocalId(String localId) {
        String object = nativeGetObjectIdByLocalId(DEVICES_NAMES);
        if (object.isEmpty()) {
            return "";
        }

        String res = "";
        try {
            //Log.i(TAG, "GetDeviceObjectIdByLocalId: trying to read JSON: " + object);
            JSONObject result = new JSONObject(object);
            JSONArray devices = result.getJSONArray("devices");
            for (int i = 0; i < devices.length(); i++) {
                JSONObject device = devices.getJSONObject(i);
                String currentObject = device.getString("deviceId");
                if (currentObject.equals(localId)) {
                    res = device.getString("objectId");
                    break;
                }
            }
        } catch (JSONException e) {
            Log.e(TAG, "GetDeviceObjectIdByLocalId JSONException error " + e);
        } catch (IllegalStateException e) {
              Log.e(TAG, "GetDeviceObjectIdByLocalId IllegalStateException error " + e);
        }

        //Log.i(TAG, "!!!GetDeviceObjectIdByLocalId res == " + res);

        return res;
    }

    public ArrayList<ResolvedRecordToApply> GetAllDevices() {
        ArrayList<ResolvedRecordToApply> result_devices = new ArrayList<ResolvedRecordToApply>();

        String object = nativeGetObjectIdByLocalId(DEVICES_NAMES);
        if (object.isEmpty()) {
            return result_devices;
        }

        JsonReader reader = null;
        try {
            //Log.i(TAG, "GetAllDevices: trying to read JSON: " + object);
            JSONObject result = new JSONObject(object);
            JSONArray devices = result.getJSONArray("devices");
            for (int i = 0; i < devices.length(); i++) {
                JSONObject device = devices.getJSONObject(i);
                String deviceName = device.getString("name");
                String currentObject = device.getString("objectId");
                String deviceId = device.getString("deviceId");
                result_devices.add(new ResolvedRecordToApply(currentObject, "0", null, deviceName, deviceId, 0));
            }
        } catch (JSONException e) {
            Log.e(TAG, "GetAllDevices JSONException error " + e);
        } catch (IllegalStateException e) {
              Log.e(TAG, "GetAllDevices IllegalStateException error " + e);
        }
        return result_devices;
    }

    private String GetBookmarkOrder(String localId) {
        try {
            String objectId = nativeGetObjectIdByLocalId(localId);
            if (objectId.isEmpty()) {
                return "";
            }
            JSONArray bookmarkArray = new JSONArray(objectId);
            JSONObject bookmark = bookmarkArray.getJSONObject(0);
            if (!bookmark.has("order")) {
                Log.e(TAG, "Could not find order for bookmark: " + objectId);
                return "";
            }
            String order = bookmark.getString("order");
            return order;
        } catch (JSONException e) {
            Log.e(TAG, "Could not get order for bookmark: " + e);
            return "";
        }
    }

    private String GetObjectId(String localId) {
        String objectId = nativeGetObjectIdByLocalId(localId);
        if (0 == objectId.length()) {
            return objectId;
        }
        JsonReader reader = null;
        String res = "";
        try {
            reader = new JsonReader(new InputStreamReader(new ByteArrayInputStream(objectId.getBytes()), "UTF-8"));
            reader.beginArray();
            while (reader.hasNext()) {
                reader.beginObject();
                while (reader.hasNext()) {
                    String name = reader.nextName();
                    if (name.equals("objectId")) {
                        res = reader.nextString();
                    } else {
                        reader.skipValue();
                    }
               }
               reader.endObject();
           }
           reader.endArray();
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, "GetObjectId UnsupportedEncodingException error " + e);
        } catch (IOException e) {
            Log.e(TAG, "GetObjectId IOException error " + e);
        } catch (IllegalStateException e) {
              Log.e(TAG, "GetObjectId IllegalStateException error " + e);
        } finally {
            if (null != reader) {
                try {
                    reader.close();
                } catch (IOException e) {
                }
            }
        }
        return res;
    }

    private void SaveObjectId(String localId, String objectId, String order, boolean saveObjectId) {
        String objectIdJSON = "[{\"objectId\": \"" + objectId + "\", \"order\": \"" + order + "\", \"apiVersion\": \"" + mApiVersion + "\"}]";
        if (!saveObjectId) {
            nativeSaveObjectId(localId, objectIdJSON, "");
        } else {
            nativeSaveObjectId(localId, objectIdJSON, objectId);
        }
    }

    private String GenerateObjectIdInternal() {
        String res = "";
        // Generates 16 random 8 bits numbers
        Random random = new Random();
        for (int i = 0; i < 16; i++) {
            if (i != 0) {
                res += ", ";
            }
            try {
                res += String.valueOf(random.nextInt(256));
            } catch (IllegalArgumentException exc) {
                res = "";
                Log.e(TAG, "ObjectId generation exception " + exc);
            }
        }

        return res;
    }

    private String GenerateObjectId(String localId) {
        String res = GetObjectId(localId);
        if (0 != res.length()) {
            return res;
        }
        res = GenerateObjectIdInternal();
        return res;
    }

    private void ResetSyncWebContents() {
        synchronized (mSyncIsReady) {
            mSyncIsReady.mShouldResetSync = true;
            ThreadUtils.runOnUiThreadBlocking(
                new Runnable() {
                    @Override
                    public void run() {
                        ResetSyncWebContentsImpl();
                    }
                }
            );
        }
    }

    private void ResetSyncWebContentsImpl() {
        if (mSyncIsReady.mShouldResetSync) {
            if (null != mWebContentsInjector) {
                mWebContentsInjector.removeInterface("injectedObject");
            }
            if (null != mWebContents) {
                mWebContents.destroy();
            }
            mWebContents = null;
            mViewEventSink = null;
            mWebContentsInjector = null;
            mSyncIsReady.mShouldResetSync = false;
        }
    }

    private void TrySync() {
        try {
            synchronized (mSyncIsReady) {
                ResetSyncWebContentsImpl();
                if (null == mWebContents) {
                    mWebContents = WebContentsFactory.createWebContents(false, true);
                    if (null != mWebContents) {
                        ContentView cv = ContentView.createContentView(mContext, mWebContents);
                        mWebContents.initialize(null, ViewAndroidDelegate.createBasicDelegate(cv), cv, new WindowAndroid(mContext), WebContents.createDefaultInternalsHolder());
                        mViewEventSink = ViewEventSinkImpl.from(mWebContents);
                        if (null != mViewEventSink) {
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
                                initContenViewCore();
                            } else {
                                getWebContentsInjector().addPossiblyUnsafeInterface(new JsObject(), "injectedObject", null);
                            }

                            String toLoad = "<script type='text/javascript'>";
                            try {
                                String script = convertStreamToString(mContext.getAssets().open(ANDROID_SYNC_JS));
                                toLoad += script.replace("%", "%25").replace("\n", "%0A").replace("#", "%23") + "</script><script type='text/javascript'>";
                                script = convertStreamToString(mContext.getAssets().open(BUNDLE_JS));
                                toLoad += script.replace("%", "%25").replace("\n", "%0A").replace("#", "%23") + "</script>";
                            } catch (IOException exc) {
                                Log.e(TAG, "Load script exception: " + exc);
                            }
                            LoadUrlParams loadUrlParams = LoadUrlParams.createLoadDataParamsWithBaseUrl(toLoad, "text/html", false, "file:///android_asset/", null);
                            loadUrlParams.setCanLoadLocalResources(true);
                            mWebContents.getNavigationController().loadUrl(loadUrlParams);
                        }
                    }
                }
            }
        } catch (Exception exc) {
            // Ignoring sync exception, we will try it on a next loop execution
            Log.e(TAG, "TrySync exception: " + exc);
        }
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    private void initContenViewCore() {
        getWebContentsInjector().addPossiblyUnsafeInterface(new JsObject(), "injectedObject", JavascriptInterface.class);
    }

    private void CallScript(StringBuilder strCall) {
        ThreadUtils.runOnUiThread(new EjectedRunnable(strCall));
    }

    private void GotInitData() {
        String deviceId = (null == mDeviceId ? null : "[" + mDeviceId + "]");
        String seed = (null == mSeed ? null : "[" + mSeed + "]");
        if (0 == (mContext.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE)) {
            mDebug = "false";
        }
        CallScript(new StringBuilder(String.format("javascript:callbackList['got-init-data'](null, %1$s, %2$s, {apiVersion: '%3$s', serverUrl: '%4$s', debug: %5$s})", seed, deviceId, mApiVersion, mServerUrl, mDebug)));
    }

    private void SaveInitData(String arg1, String arg2) {
        if (null == arg1 || null == arg2) {
            if (null != mSyncScreensObserver) {
                mSyncScreensObserver.onSyncError("Incorrect args for SaveInitData");
            }
        }
        if (null != arg1 && !arg1.isEmpty()) {
            mSeed = arg1;
        }
        mDeviceId = arg2;
        //Log.i(TAG, "!!!deviceId == " + mDeviceId);
        //Log.i(TAG, "!!!seed == " + mSeed);
        // Save seed and deviceId in preferences
        SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString(PREF_DEVICE_ID, mDeviceId);
        if (null != mSeed && !mSeed.isEmpty()) {
            if (null != mSyncScreensObserver) {
                mSyncScreensObserver.onSeedReceived(mSeed, false, true);
            }
            editor.putString(PREF_SEED, mSeed);
        }
        editor.apply();
        SetSyncEnabled(true);
    }

    public void SendSyncRecords(String recordType, StringBuilder recordsJSON, String action, ArrayList<String> ids) {
        if (!SyncHasBeenSetup()) {
            return;
        }
        synchronized (mSendSyncDataThread) {
            SaveGetDeleteNotSyncedRecords(recordType, action, ids, NotSyncedRecordsOperation.AddItems);
            if (recordType.equals(SyncRecordType.BOOKMARKS)) {
                // Collect bookmarks to send them in a bulk request
                if (!mBulkBookmarkOperations.isEmpty()) {
                    mBulkBookmarkOperations += ",";
                }
                mBulkBookmarkOperations += recordsJSON;
            } else {
                StringBuilder script = new StringBuilder("javascript:callbackList['send-sync-records'](null, '");
                script.append(recordType).append("'");
                script.append(", ").append(recordsJSON).append(")");
                CallScript(script);
            }
        }
    }

    private void SendBulkBookmarks() {
        synchronized (mSendSyncDataThread) {
            if (!mSyncIsReady.IsReady() || mBulkBookmarkOperations.length() == 0) {
                return;
            }
            StringBuilder script = new StringBuilder("javascript:callbackList['send-sync-records'](null, '");
            script.append(SyncRecordType.BOOKMARKS).append("'");
            script.append(", [").append(mBulkBookmarkOperations).append("])");
            CallScript(script);
            mAttepmtsBeforeSendingNotSyncedRecords = ATTEMPTS_BEFORE_SENDING_NOT_SYNCED_RECORDS;
            mBulkBookmarkOperations = "";
        }
    }

    @SuppressWarnings("unchecked")
    private ArrayList<String> GetNotSyncedRecords(String recordId) {
        ArrayList<String> existingList = new ArrayList<String>();
        try {
            String currentArray = GetObjectId(recordId);
            if (!currentArray.isEmpty()) {
                byte[] data = Base64.decode(currentArray, Base64.DEFAULT);
                ObjectInputStream ois = new ObjectInputStream(
                                                new ByteArrayInputStream(data));
                existingList = (ArrayList<String>)ois.readObject();
                ois.close();
            }
        } catch (IOException ioe) {
        } catch (ClassNotFoundException e) {
        }

        return existingList;
    }

    private void SaveNotSyncedRecords(String recordId, ArrayList<String> existingList) {
        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            ObjectOutputStream oos = new ObjectOutputStream(baos);
            oos.writeObject(existingList);
            oos.close();
            SaveObjectId(recordId, Base64.encodeToString(baos.toByteArray(), Base64.DEFAULT), "", false);
        } catch (IOException ioe) {
            Log.e(TAG, "Failed to SaveNotSyncedRecords: " + ioe);
        }
    }

    private synchronized ArrayList<String> SaveGetDeleteNotSyncedRecords(String recordType, String action, ArrayList<String> ids, NotSyncedRecordsOperation operation) {
        if (NotSyncedRecordsOperation.GetItems != operation && 0 == ids.size()) {
            return null;
        }
        String recordId = recordType + action;
        ArrayList<String> existingList = GetNotSyncedRecords(recordId);
        if (NotSyncedRecordsOperation.GetItems == operation) {
            return existingList;
        } else if (NotSyncedRecordsOperation.AddItems == operation) {
            for (String id: ids) {
                if (!existingList.contains(id)) {
                    existingList.add(id);
                }
            }
        } else if (NotSyncedRecordsOperation.DeleteItems == operation) {
            boolean listChanged = false;
            boolean clearLocalDb = action.equals(DELETE_RECORD);
            for (String id: ids) {
                boolean itemRemoved = existingList.remove(id);
                if (!listChanged) {
                    listChanged = itemRemoved;
                }
                // Delete corresponding objectIds
                if (clearLocalDb) {
                    nativeDeleteByLocalId(id);
                }
            }
            if (!listChanged) {
                return null;
            }
        }

        SaveNotSyncedRecords(recordId, existingList);

        return null;
    }

    public void SetUpdateDeleteDeviceName(String action, String deviceName, String deviceId, String objectId) {
        if (action.equals(CREATE_RECORD)) {
            objectId = GetObjectId(THIS_DEVICE_OBJECT_ID);
            if (0 == objectId.length()) {
                objectId = GenerateObjectIdInternal();
                SaveObjectId(THIS_DEVICE_OBJECT_ID, objectId, "", false);
            }
        }
        assert !objectId.isEmpty();
        if (objectId.isEmpty()) {
            return;
        }
        StringBuilder request = new StringBuilder("[");
        request.append(CreateDeviceCreationRecord(deviceName, objectId, action, deviceId)).append("]");
        ArrayList<String> ids = new ArrayList<String>();
        ids.add(deviceId);
        //Log.i(TAG, "!!!device operation request: " + request.toString());
        SendSyncRecords(SyncRecordType.PREFERENCES, request, action, ids);
    }

    private void SendAllLocalBookmarks() {
      if (null == mNewBookmarkModel) {
          return;
      }
      synchronized (mNewBookmarkModel)
      {
          // Grab current existing bookmarksIds to sync them
          List<BookmarkItem> localBookmarks = GetBookmarkItems();
          if (null != localBookmarks) {
              //Log.i(TAG, "!!!localBookmarks.size() == " + localBookmarks.size());
              int listSize = localBookmarks.size();
              for (int i = 0; i < listSize; i += SEND_RECORDS_COUNT_LIMIT) {
                  List<BookmarkItem> subList = localBookmarks.subList(i, Math.min(listSize, i + SEND_RECORDS_COUNT_LIMIT));
                  CreateUpdateDeleteBookmarks(CREATE_RECORD, subList.toArray(new BookmarkItem[subList.size()]), true, true);
              }
          }
      }
    }

    private Long GetLatestDeviceRecordTime() {
      SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
      Long latestDeviceRecordTimeL = sharedPref.getLong(PREF_LATEST_DEVICE_RECORD_TIMESTAMPT_NAME, 0);
      return latestDeviceRecordTimeL;
    }

    private void SetLatestDeviceRecordTime(String latestDeviceRecordTime) {
      try {
          Long latestDeviceRecordTimeL = Long.parseLong(latestDeviceRecordTime);
          // Save last fetch time in preferences
          SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
          SharedPreferences.Editor editor = sharedPref.edit();
          editor.putLong(PREF_LATEST_DEVICE_RECORD_TIMESTAMPT_NAME, latestDeviceRecordTimeL);
          editor.apply();
      } catch (NumberFormatException e) {
      }
    }

    private void FetchSyncRecords(String lastRecordFetchTime, String category) {
        synchronized (mSyncThread) {
            if (!mSyncIsReady.IsReady()) {
                //Log.i(TAG, "!!!Sync is not ready");
                return;
            }
            //Log.i(TAG, "!!!in FetchSyncRecords lastRecordFetchTime == " + lastRecordFetchTime);
            if (0 == mTimeLastFetch && 0 == mTimeLastFetchExecuted) {
                // It is the very first time of the sync start
                // Set device name
                if (null == mDeviceName || mDeviceName.isEmpty()) {
                    SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
                    mDeviceName = sharedPref.getString(PREF_SYNC_DEVICE_NAME, "");
                }
                SetUpdateDeleteDeviceName(CREATE_RECORD, mDeviceName, mDeviceId, "");
                SendAllLocalBookmarks();
                // Initial fetch of devices only
                CallScript(new StringBuilder(String.format("javascript:callbackList['fetch-sync-records'](null, %1$s, %2$s, %3$s)",
                    SyncRecordType.GetRecordTypeJSArray(SyncRecordType.PREFERENCES), 0, FETCH_RECORDS_CHUNK_SIZE)));
                try {
                    Thread.sleep(BraveSyncWorker.INTERVAL_TO_FETCH_RECORDS);
                } catch (InterruptedException e) {
                    Log.w(TAG, "Fetch waiting was interrupted: " + e);
                }
            }
            Calendar currentTime = Calendar.getInstance();
            if (currentTime.getTimeInMillis() - mTimeLastFetchExecuted <= INTERVAL_TO_FETCH_RECORDS && lastRecordFetchTime.isEmpty()) {
                return;
            }
            mInterruptSyncSleep = false;
            mLatestFetchRequest = (lastRecordFetchTime.isEmpty() ? String.valueOf(mTimeLastFetch) : lastRecordFetchTime);

            // We have no yet option to turn on/off sync categories, always sync both
            // preferences and bookmarks.
            // Call them separately
            if (category.isEmpty() || SyncRecordType.BOOKMARKS.equals(category)) {
                CallScript(new StringBuilder(String.format("javascript:callbackList['fetch-sync-records'](null, %1$s, %2$s, %3$s)",
                    SyncRecordType.GetRecordTypeJSArray(SyncRecordType.BOOKMARKS), mLatestFetchRequest, FETCH_RECORDS_CHUNK_SIZE)));
            }

            if (category.isEmpty() || SyncRecordType.PREFERENCES.equals(category)) {
                Long latestDeviceRecordTime = GetLatestDeviceRecordTime();
                CallScript(new StringBuilder(String.format("javascript:callbackList['fetch-sync-records'](null, %1$s, %2$s, %3$s)",
                    SyncRecordType.GetRecordTypeJSArray(SyncRecordType.PREFERENCES), latestDeviceRecordTime, FETCH_RECORDS_CHUNK_SIZE)));
            }

            mTimeLastFetchExecuted = currentTime.getTimeInMillis();
            if (!lastRecordFetchTime.isEmpty()) {
                try {
                    mTimeLastFetch = Long.parseLong(lastRecordFetchTime);
                    // Save last fetch time in preferences
                    SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
                    SharedPreferences.Editor editor = sharedPref.edit();
                    editor.putLong(PREF_LAST_FETCH_NAME, mTimeLastFetch);
                    editor.apply();
                } catch (NumberFormatException e) {
                }
            }
        }
    }

    public StringBuilder GetExistingObjects(String categoryName, String recordsJSON, String latestRecordTimeStampt, boolean isTruncated) {
        if (null == categoryName || null == recordsJSON) {
            return new StringBuilder("");
        }
        if (!SyncRecordType.BOOKMARKS.equals(categoryName) && !SyncRecordType.PREFERENCES.equals(categoryName)) {
            // TODO sync for other categories
            return new StringBuilder("");
        }

        if (latestRecordTimeStampt != null && !latestRecordTimeStampt.isEmpty()
                && SyncRecordType.PREFERENCES.equals(categoryName)) {
            SetLatestDeviceRecordTime(latestRecordTimeStampt);
        }

        mFetchInProgress = true;
        if (SyncRecordType.BOOKMARKS.equals(categoryName)) {
            mLatestRecordTimeStampt = latestRecordTimeStampt;
        }

        StringBuilder res = new StringBuilder("");
        /*if (recordsJSON.length() > 2 && SyncRecordType.BOOKMARKS.equals(categoryName)) {
            Log.i(TAG, "!!!in GetExistingObjects: " + latestRecordTimeStampt + ": " + isTruncated + ": " + recordsJSON);
        }*/

        // Debug
        /*int iPos = recordsJSON.indexOf("NewFolder3");
        if (-1 != iPos) {
            if (iPos + 2000 > recordsJSON.length()) {
                Log.i(TAG, "!!!GetExistingObjects == " + recordsJSON.substring(iPos));
            } else {
                Log.i(TAG, "!!!GetExistingObjects == " + recordsJSON.substring(iPos, iPos + 2000));
            }
            if (iPos > 500) {
                if (iPos + 1500 > recordsJSON.length()) {
                    Log.i(TAG, "!!!GetExistingObjects == " + recordsJSON.substring(iPos - 500));
                } else {
                    Log.i(TAG, "!!!GetExistingObjects == " + recordsJSON.substring(iPos - 500, iPos + 1500));
                }
            }
        }*/
        /*iPos = recordsJSON.indexOf("\"objectId\":{\"0\":26,\"1\":251", iPos + 1);
        if (-1 != iPos) {
            if (iPos + 2000 > recordsJSON.length()) {
                Log.i(TAG, "!!!GetExistingObjects1 == " + recordsJSON.substring(iPos));
            } else {
                Log.i(TAG, "!!!GetExistingObjects1 == " + recordsJSON.substring(iPos, iPos + 2000));
            }
            if (iPos > 500) {
                if (iPos + 1500 > recordsJSON.length()) {
                    Log.i(TAG, "!!!GetExistingObjects1 == " + recordsJSON.substring(iPos - 500));
                } else {
                    Log.i(TAG, "!!!GetExistingObjects1 == " + recordsJSON.substring(iPos - 500, iPos + 1500));
                }
            }
        }*/
        //Log.i(TAG, "!!!recordsJSON == " + recordsJSON);
        /*int iPos = recordsJSON.indexOf("Bobrina");
        if (-1 != iPos) {
            Log.i(TAG, "!!!record == " + recordsJSON.substring(iPos, iPos + 2000));
            if (iPos > 500) {
                Log.i(TAG, "!!!record1 == " + recordsJSON.substring(iPos - 500, iPos + 1500));
            }
        }*/
        /*int step = 2000;
        int count = 0;
        for (;;) {
            int endIndex = count * step + step;
            if (endIndex > recordsJSON.length() - 1) {
                endIndex = recordsJSON.length() - 1;
            }
            String substr = recordsJSON.substring(count * step, endIndex);
            Log.i(TAG, "!!!substr == " + substr);
            if (endIndex != count * step + step) {
                break;
            }
            count++;
        }*/
        //

        HashMap<String, ArrayList<String>> syncedRecordsMap = new HashMap<String, ArrayList<String>>();
        List<ResolvedRecordToApply> bookmarksRecords = new ArrayList<ResolvedRecordToApply>();
        long defaultFolderId = (null != mDefaultFolder ? mDefaultFolder.getId() : 0);
        JsonReader reader = null;
        try {
            reader = new JsonReader(new InputStreamReader(new ByteArrayInputStream(recordsJSON.getBytes()), "UTF-8"));
            reader.beginArray();
            while (reader.hasNext()) {
                StringBuilder action = new StringBuilder(CREATE_RECORD);
                StringBuilder objectId = new StringBuilder("");
                StringBuilder deviceId = new StringBuilder("");
                StringBuilder objectData = new StringBuilder("");
                BookmarkInternal bookmarkInternal = null;
                StringBuilder deviceName = new StringBuilder("");
                long syncTime = 0;
                reader.beginObject();
                while (reader.hasNext()) {
                    String name = reader.nextName();
                    if (name.equals("action")) {
                        action = GetAction(reader);
                    } else if (name.equals("deviceId")) {
                        deviceId = GetDeviceId(reader);
                    } else if (name.equals("objectId")) {
                        objectId = GetObjectIdJSON(reader);
                    } else if (name.equals(SyncObjectData.BOOKMARK)) {
                        bookmarkInternal = GetBookmarkRecord(reader);
                    } else if (name.equals(SyncObjectData.DEVICE)) {
                        deviceName = GetDeviceName(reader);
                    } else if (name.equals("objectData")) {
                        objectData = GetObjectDataJSON(reader);
                    } else if (name.equals("syncTimestamp")) {
                        syncTime = reader.nextLong();
                    } else {
                        reader.skipValue();
                    }
                }
                reader.endObject();
                if (null == bookmarkInternal && 0 == deviceName.length()) {
                    continue;
                }
                StringBuilder serverRecord = new StringBuilder("[");
                StringBuilder localRecord = new StringBuilder("");
                if (null != bookmarkInternal) {
                    bookmarksRecords.add(new ResolvedRecordToApply(objectId.toString(), action.toString(), bookmarkInternal, deviceName.toString(), deviceId.toString(), syncTime));
                } else {
                    serverRecord.append(CreateRecord(objectId.toString(), SyncObjectData.DEVICE,
                           action.toString(), deviceId.toString(), syncTime)).append(CreateDeviceRecord(deviceName.toString())).append(" }");
                    String localDeviceName = GetDeviceNameByObjectId(objectId.toString());
                    if (!localDeviceName.isEmpty()) {
                        localRecord.append(CreateRecord(objectId.toString(), SyncObjectData.DEVICE, CREATE_RECORD,
                             mDeviceId, syncTime)).append(CreateDeviceRecord(localDeviceName)).append(" }]");
                    }
                    if (0 == res.length()) {
                        res.append("[");
                    } else {
                        res.append(", ");
                    }
                    res.append(serverRecord).append(", ").append(0 != localRecord.length() ? localRecord : "null]");
                    // Mark the record as sucessfully sent
                    ArrayList<String> value = syncedRecordsMap.get(action.toString());
                    if (null == value) {
                        value = new ArrayList<String>();
                    }
                    value.add(deviceId.toString());
                    syncedRecordsMap.put(action.toString(), value);
                }
            }
            reader.endArray();
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, "GetExistingObjects UnsupportedEncodingException error " + e);
        } catch (IOException e) {
            Log.e(TAG, "GetExistingObjects IOException error " + e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "GetExistingObjects IllegalStateException error " + e);
        } catch (IllegalArgumentException exc) {
            Log.e(TAG, "GetExistingObjects generation exception " + exc);
        } finally {
            if (null != reader) {
                try {
                    reader.close();
                } catch (IOException e) {
                }
            }
        }
        Collections.sort(bookmarksRecords);
        for (ResolvedRecordToApply bookmarkRecord: bookmarksRecords) {
            if (bookmarkRecord.mBookmarkInternal == null) {
                assert false;
                continue;
            }
            StringBuilder serverRecord = new StringBuilder("[");
            StringBuilder localRecord = new StringBuilder("");
            String localId = nativeGetLocalIdByObjectId(bookmarkRecord.mObjectId.toString());
            serverRecord.append(CreateRecord(bookmarkRecord.mObjectId, SyncObjectData.BOOKMARK,
                bookmarkRecord.mAction, bookmarkRecord.mDeviceId, bookmarkRecord.mSyncTime)).append(CreateBookmarkRecord(bookmarkRecord.mBookmarkInternal.mUrl,
                bookmarkRecord.mBookmarkInternal.mTitle, bookmarkRecord.mBookmarkInternal.mIsFolder, defaultFolderId, "[" + bookmarkRecord.mBookmarkInternal.mParentFolderObjectId + "]",
                bookmarkRecord.mBookmarkInternal.mCustomTitle, bookmarkRecord.mBookmarkInternal.mLastAccessedTime, bookmarkRecord.mBookmarkInternal.mCreationTime,
                bookmarkRecord.mBookmarkInternal.mFavIcon, bookmarkRecord.mBookmarkInternal.mOrder)).append(" }");
            BookmarkItem bookmarkItem = GetBookmarkItemByLocalId(localId, false);
            if (null != bookmarkItem) {
                String order = GetBookmarkOrder(localId, false);
                // TODO pass always CREATE_RECORD, it means action is create
                long parentId = bookmarkItem.getParentId().getId();
                localRecord.append(CreateRecord(bookmarkRecord.mObjectId, SyncObjectData.BOOKMARK, CREATE_RECORD, mDeviceId, bookmarkRecord.mSyncTime))
                    .append(CreateBookmarkRecord(bookmarkItem.getUrl(), bookmarkItem.getTitle(),
                    bookmarkItem.isFolder(), parentId, "", "", 0, 0, "", order)).append(" }]");
            }
            // Mark the record as sucessfully sent
            ArrayList<String> value = syncedRecordsMap.get(bookmarkRecord.mAction.toString());
            if (null == value) {
                value = new ArrayList<String>();
            }
            value.add(localId);
            syncedRecordsMap.put(bookmarkRecord.mAction.toString(), value);
            // Ignore records which needs resolve, but which came from our device
            // No need to reapply them, and also they can confuse the sync lib records merger
            if (!this.mDeviceId.equals(bookmarkRecord.mDeviceId)) {
              if (0 == res.length()) {
                  res.append("[");
              } else {
                  res.append(", ");
              }
              res.append(serverRecord).append(", ").append(0 != localRecord.length() ? localRecord : "null]");
            }
        }
        if (0 != res.length()) {
            res.append("]");
        }
        if (!isTruncated) {
            // We finished fetch in chunks;
            //Log.i(TAG, "!!!finished fetch in chunks: " + categoryName);
            mLatestRecordTimeStampt = "";
        }
        for (Map.Entry<String, ArrayList<String>> entry : syncedRecordsMap.entrySet()) {
            SaveGetDeleteNotSyncedRecords(categoryName, entry.getKey(), entry.getValue(), NotSyncedRecordsOperation.DeleteItems);
        }
        //
        //Log.i(TAG, "!!!GetExistingObjects res == " + res);
        /*int step = 2000;
        int count = 0;
        for (;;) {
            int endIndex = count * step + step;
            if (endIndex > res.length() - 1) {
                endIndex = res.length() - 1;
            }
            String substr = res.substring(count * step, endIndex);
            Log.i(TAG, "!!!substr == " + substr);
            if (endIndex != count * step + step) {
                break;
            }
            count++;
        }*/
        //

        //Log.i(TAG, "!!!res == " + res.toString());
        return res;
    }

    private ArrayList<BookmarkItem> GetBookmarkItemsByLocalIds(ArrayList<String> localIds, String action) {
        if (0 == localIds.size()) {
            return new ArrayList<BookmarkItem>();
        }
        try {
           ArrayList<Long> localIdsLong = new ArrayList<Long>();
           for (String id: localIds) {
              localIdsLong.add(Long.parseLong(id));
           }
           GetBookmarkItemsByLocalIdsRunnable bookmarkRunnable = new GetBookmarkItemsByLocalIdsRunnable(localIdsLong, action);
           if (null == bookmarkRunnable) {
              return new ArrayList<BookmarkItem>();
           }
           synchronized (bookmarkRunnable)
           {
               // Execute code on UI thread
               ThreadUtils.runOnUiThread(bookmarkRunnable);

               // Wait until runnable is finished
               try {
                   bookmarkRunnable.wait();
               } catch (InterruptedException e) {
               }
           }

           return bookmarkRunnable.mBookmarkItems;
        } catch (NumberFormatException e) {
            Log.e(TAG, "NumberFormatException: " + e);
        }

        return null;
    }

    private BookmarkItem GetBookmarkItemByLocalId(String localId, boolean newBookmarkModelAcquired) {
        if (0 == localId.length()) {
            return null;
        }
        try {
           long llocalId = Long.parseLong(localId);
           GetBookmarkIdRunnable bookmarkRunnable = new GetBookmarkIdRunnable(llocalId);
           if (null == bookmarkRunnable) {
              return null;
           }
           synchronized (bookmarkRunnable)
           {
               if (newBookmarkModelAcquired) {
                  bookmarkRunnable.SetNewBookmarkModelAcquiredByThisRunnableWaiter();
               }
               // Execute code on UI thread
               ThreadUtils.runOnUiThread(bookmarkRunnable);

               // Wait until runnable is finished
               try {
                   bookmarkRunnable.wait();
               } catch (InterruptedException e) {
                   Log.e(TAG, "GetBookmarkItemByLocalId error: " + e);
               }
           }

           return bookmarkRunnable.mBookmarkItem;
        } catch (NumberFormatException e) {
           Log.e(TAG, "GetBookmarkItemByLocalId error: " + e);
        }

        return null;
    }

    public void SendResolveSyncRecords(String categoryName, StringBuilder existingRecords) {
        if (!mSyncIsReady.IsReady()
              || null == categoryName || null == existingRecords
              || 0 == categoryName.length()) {
            return;
        }
        if (0 == existingRecords.length()) {
            existingRecords.append("[]");
        }
        StringBuilder script = new StringBuilder("javascript:callbackList['resolve-sync-records'](null, '");
        script.append(categoryName).append("'");
        script.append(", ").append(existingRecords).append(")");

        CallScript(script);
    }

    private void DeleteBookmarkByLocalId(String localId) {
        if (0 == localId.length()) {
            return;
        }
        try {
           long llocalId = Long.parseLong(localId);
           DeleteBookmarkRunnable bookmarkRunnable = new DeleteBookmarkRunnable(llocalId);
           if (null == bookmarkRunnable) {
              return;
           }
           synchronized (bookmarkRunnable)
           {
               // Execute code on UI thread
               ThreadUtils.runOnUiThread(bookmarkRunnable);

               // Wait until runnable is finished
               try {
                   bookmarkRunnable.wait();
               } catch (InterruptedException e) {
               }
           }
           nativeDeleteByLocalId(localId);
           if (null != bookmarkRunnable.mBookmarksItems) {
               for (BookmarkItem item: bookmarkRunnable.mBookmarksItems) {
                  String itemLocalId = String.valueOf(item.getId().getId());
                  nativeDeleteByLocalId(itemLocalId);
               }
           }
        } catch (NumberFormatException e) {
        }

        return;
    }

    class DeleteBookmarkRunnable implements Runnable {
        private long mBookmarkId;
        public List<BookmarkItem> mBookmarksItems;

        public DeleteBookmarkRunnable(long bookmarkId) {
            mBookmarkId = bookmarkId;
        }

        @Override
        public void run() {
            BookmarkId bookmarkId = new BookmarkId(mBookmarkId, BookmarkType.NORMAL);
            if (bookmarkId == null) {
                Log.e(TAG, "bookmarkId == null");
            }
            if (null != mNewBookmarkModel && null != bookmarkId) {
                synchronized (mNewBookmarkModel) {
                    // Get children to clean local leveldb, all children are deleted recursively
                    BookmarkItem bookmarkRoot = mNewBookmarkModel.getBookmarkById(bookmarkId);
                    mBookmarksItems = getBookmarksForFolder(mNewBookmarkModel, bookmarkRoot);
                    // Delete children bookmarks
                    for (BookmarkItem bookmark : mBookmarksItems) {
                        if (!bookmark.isFolder()) {
                            mNewBookmarkModel.deleteBookmarkSilently(bookmark.getId());
                        }
                    }
                    // Delete children bookmark folders
                    for (BookmarkItem bookmark : mBookmarksItems) {
                        if (bookmark.isFolder()) {
                            mNewBookmarkModel.deleteBookmarkSilently(bookmark.getId());
                        }
                    }
                    // Delete root bookmark
                    if (bookmarkRoot == null) {
                        Log.e(TAG, "Failed to find root bookmark: " + bookmarkId);
                    } else {
                        mNewBookmarkModel.deleteBookmarkSilently(bookmarkId);
                    }
                }
            }

            synchronized (this)
            {
                this.notify();
            }
        }
    }

    private void EditBookmarkByLocalId(String localId, String url, String title, String parentLocalId, String objectId, String order) {
        if (0 == localId.length()) {
            return;
        }
        try {
            long llocalId = Long.parseLong(localId);
            long defaultFolderId = (null != mDefaultFolder ? mDefaultFolder.getId() : 0);
            long lparentLocalId = defaultFolderId;
            if (!parentLocalId.isEmpty()) {
                    lparentLocalId = Long.parseLong(parentLocalId);
            }
            EditBookmarkRunnable bookmarkRunnable = new EditBookmarkRunnable(llocalId, url, title, lparentLocalId);
            if (null == bookmarkRunnable) {
                return;
            }
            synchronized (bookmarkRunnable)
            {
                // Execute code on UI thread
                ThreadUtils.runOnUiThread(bookmarkRunnable);

                // Wait until runnable is finished
                try {
                    bookmarkRunnable.wait();
                } catch (InterruptedException e) {
                    Log.e(TAG, "EditBookmarkByLocalId error: " + e);
                }
            }
        } catch (NumberFormatException e) {
            Log.e(TAG, "EditBookmarkByLocalId error: " + e);
        }
        String oldOrder = GetBookmarkOrder(localId);
        SaveObjectId(localId, objectId, order, true);
        if (!oldOrder.equals(order)) {
            mReorderBookmarks = true;
        }
        return;
    }

    private void AddBookmark(String url, String title, boolean isFolder, String objectId, String parentLocalId, String order) {
        try {
           long defaultFolderId = (null != mDefaultFolder ? mDefaultFolder.getId() : 0);
           long lparentLocalId = defaultFolderId;
           if (!parentLocalId.isEmpty()) {
                lparentLocalId = Long.parseLong(parentLocalId);
           }
           AddBookmarkRunnable bookmarkRunnable = new AddBookmarkRunnable(url, title, isFolder, lparentLocalId);
           if (null == bookmarkRunnable) {
                return;
           }
           synchronized (bookmarkRunnable)
           {
               // Execute code on UI thread
               ThreadUtils.runOnUiThread(bookmarkRunnable);

               // Wait until runnable is finished
               try {
                   bookmarkRunnable.wait();
               } catch (InterruptedException e) {
                   Log.e(TAG, "AddBookmark error: " + e);
               }
           }
           if (null != bookmarkRunnable.mBookmarkId) {
               SaveObjectId(String.valueOf(bookmarkRunnable.mBookmarkId.getId()), objectId, order, true);
               mReorderBookmarks = true;
           }
        } catch (NumberFormatException e) {
            Log.e(TAG, "AddBookmark error: " + e);
        }
    }

    private List<BookmarkItem> GetBookmarkItems() {
        try {
           GetBookmarkItemsRunnable bookmarkRunnable = new GetBookmarkItemsRunnable();
           if (null == bookmarkRunnable) {
              return null;
           }
           synchronized (bookmarkRunnable)
           {
               // Execute code on UI thread
               ThreadUtils.runOnUiThread(bookmarkRunnable);

               // Wait until runnable is finished
               try {
                   bookmarkRunnable.wait();
               } catch (InterruptedException e) {
               }
           }

           return bookmarkRunnable.mBookmarksItems;
        } catch (NumberFormatException e) {
        }

        return null;
    }

    class GetBookmarkItemsRunnable implements Runnable {
        private List<BookmarkItem> mBookmarksItems;

        public GetBookmarkItemsRunnable() {
        }

        @Override
        public void run() {
            if (null != mNewBookmarkModel && null != mDefaultFolder) {
                mBookmarksItems = getBookmarksForFolder(mNewBookmarkModel, mNewBookmarkModel.getBookmarkById(mDefaultFolder));
            }

            synchronized (this)
            {
                this.notify();
            }
        }
    }

    private List<BookmarkItem> getBookmarksForFolder(BookmarkModel newBookmarkModel, BookmarkItem parent) {
        List<BookmarkItem> res = new ArrayList<BookmarkItem>();
        if (null == parent || !parent.isFolder()) {
            return res;
        }
        res = newBookmarkModel.getBookmarksForFolder(parent.getId());
        List<BookmarkItem> newList = new ArrayList<BookmarkItem>();
        for (BookmarkItem item : res) {
            if (!item.isFolder()) {
                continue;
            }
            newList.addAll(getBookmarksForFolder(newBookmarkModel, item));
        }
        res.addAll(newList);

        return res;
    }

    public void ResolvedSyncRecords(String categoryName, String recordsJSON) {
        //Log.i(TAG, "!!!in ResolvedSyncRecords");
        if (null == categoryName || null == recordsJSON) {
            assert false;
            return;
        }
        if (!SyncRecordType.BOOKMARKS.equals(categoryName) && !SyncRecordType.PREFERENCES.equals(categoryName)) {
            // TODO sync for other categories
            assert false;
            return;
        }

        // Debug
        /*int iPos = recordsJSON.indexOf("Vim Commands");
        if (-1 != iPos) {
            if (iPos + 2000 > recordsJSON.length()) {
                Log.i(TAG, "!!!Resolvedrecord == " + recordsJSON.substring(iPos));
            } else {
                Log.i(TAG, "!!!Resolvedrecord == " + recordsJSON.substring(iPos, iPos + 2000));
            }
            if (iPos > 500) {
                if (iPos + 1500 > recordsJSON.length()) {
                    Log.i(TAG, "!!!Resolvedrecord == " + recordsJSON.substring(iPos - 500));
                } else {
                    Log.i(TAG, "!!!Resolvedrecord == " + recordsJSON.substring(iPos - 500, iPos + 1500));
                }
            }
        }*/
        //
        /*if (recordsJSON.length() > 3) {
            Log.i(TAG, "ResolvedSyncRecords!!!recordsJSON = " + recordsJSON);
        }*/
        /*String[] records = recordsJSON.split("action");
        for (int i = 0; i < records.length; i++) {
            Log.i(TAG, "!!!record[" + i + "]" + records[i]);
        }*/
        //
        if (SyncRecordType.BOOKMARKS.equals(categoryName)) {
          SetExtensiveBookmarkOperation(true);
        }
        List<ResolvedRecordToApply> devicesRecords = new ArrayList<ResolvedRecordToApply>();
        List<ResolvedRecordToApply> bookmarksRecords = new ArrayList<ResolvedRecordToApply>();
        long syncTime = 0;
        JsonReader reader = null;
        try {
            reader = new JsonReader(new InputStreamReader(new ByteArrayInputStream(recordsJSON.getBytes()), "UTF-8"));
            reader.beginArray();
            while (reader.hasNext()) {
                String objectId = "";
                String action = CREATE_RECORD;
                BookmarkInternal bookmarkInternal = null;
                String deviceName = "";
                String deviceId = "";
                reader.beginObject();
                while (reader.hasNext()) {
                    String name = reader.nextName();
                    if (name.equals("action")) {
                        action = GetAction(reader).toString();
                    } else if (name.equals("deviceId")) {
                        deviceId = GetDeviceId(reader).toString();
                    } else if (name.equals("objectId")) {
                        objectId = GetObjectIdJSON(reader).toString();
                    } else if (name.equals(SyncObjectData.BOOKMARK)) {
                        bookmarkInternal = GetBookmarkRecord(reader);
                    } else if (name.equals(SyncObjectData.DEVICE)) {
                        deviceName = GetDeviceName(reader).toString();
                    } else if (name.equals("syncTimestamp")) {
                        syncTime = reader.nextLong();
                    }
                    else {
                        reader.skipValue();
                    }
               }
               reader.endObject();
               if (null == bookmarkInternal && deviceName.isEmpty()) {
                  continue;
               }
               if (null != bookmarkInternal && (action.equals(DELETE_RECORD) || !bookmarkInternal.mTitle.isEmpty() || !bookmarkInternal.mCustomTitle.isEmpty())) {
                    bookmarksRecords.add(new ResolvedRecordToApply(objectId, action, bookmarkInternal, "", "", syncTime));
               } else if (!deviceName.isEmpty()) {
                    devicesRecords.add(new ResolvedRecordToApply(objectId, action, null, deviceName, deviceId, syncTime));
               } else {
                  Log.e(TAG, "Unknown state");
                  assert false;
               }
           }
           reader.endArray();
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, "ResolvedSyncRecords UnsupportedEncodingException error " + e);
        } catch (IOException e) {
            Log.e(TAG, "ResolvedSyncRecords IOException error " + e);
        } catch (IllegalStateException e) {
            Log.e(TAG, "ResolvedSyncRecords IllegalStateException error " + e);
        } catch (IllegalArgumentException exc) {
            Log.e(TAG, "ResolvedSyncRecords generation exception " + exc);
        } finally {
            if (null != reader) {
                try {
                    reader.close();
                } catch (IOException e) {
                }
            }
        }
        Collections.sort(bookmarksRecords);
        for (ResolvedRecordToApply bookmarkRecord: bookmarksRecords) {
            BookmarkResolver(bookmarkRecord);
        }
        DeviceResolver(devicesRecords);
        if (SyncRecordType.BOOKMARKS.equals(categoryName)) {
          SetExtensiveBookmarkOperation(false);
        }
        if (mLatestRecordTimeStampt.isEmpty()) {
            mTimeLastFetch = mTimeLastFetchExecuted;
            SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
            SharedPreferences.Editor editor = sharedPref.edit();
            editor.putLong(PREF_LAST_FETCH_NAME, mTimeLastFetch);
            editor.apply();
            mFetchInProgress = false;
            SendNotSyncedRecords();
            if (SyncRecordType.BOOKMARKS.equals(categoryName) && mReorderBookmarks) {
                ReorderBookmarks();
                mReorderBookmarks = false;
            }
        } else {
            FetchSyncRecords(mLatestFetchRequest, categoryName);
        }
    }

    private void ReorderBookmarks() {
        // Bookmarks reodering will be triggered once fetch operation is finished
        // So no need in it during fetch
        if (null == mNewBookmarkModel || mFetchInProgress) {
            return;
        }
        synchronized (mNewBookmarkModel) {
            List<BookmarkItem> localBookmarks = GetBookmarkItems();
            List<OrderedBookmark> orderedBookmarks = new ArrayList<OrderedBookmark>();
            for (BookmarkItem bookmark : localBookmarks) {
                String order = GetBookmarkOrder(String.valueOf(bookmark.getId().getId()));
                if (order.isEmpty()) {
                    Log.w(TAG, "ReorderBookmarks skipping bookmark due to empty order for " + bookmark.getId().getId());
                    continue;
                }
                OrderedBookmark orderedBookmark = new OrderedBookmark(bookmark, order);
                orderedBookmarks.add(orderedBookmark);
            }
            Collections.sort(orderedBookmarks);
            ThreadUtils.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    synchronized (mNewBookmarkModel) {
                        for (OrderedBookmark orderedBookmark : orderedBookmarks) {
                            mNewBookmarkModel.moveBookmark(orderedBookmark.Bookmark().getId(), orderedBookmark.Bookmark().getParentId());
                        }
                    }
                }
            });
        }
    }

    public boolean SyncBookmarkModelIsReady() {
        return mNewBookmarkModel != null;
    }

    public void SyncedMoveBookmark(BookmarkId bookmarkId, BookmarkId newParentId) {
        if (null == mNewBookmarkModel) {
            return;
        }
        synchronized (mNewBookmarkModel) {
            mNewBookmarkModel.moveBookmark(bookmarkId, newParentId);
        }
    }

    void SaveLastSendNotSyncedTime(Long lastTimeSendNotSynced) {
      SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
      SharedPreferences.Editor editor = sharedPref.edit();
      editor.putLong(PREF_LAST_TIME_SEND_NOT_SYNCED_NAME, lastTimeSendNotSynced);
      editor.apply();
    }

    Long LoadLastSendNotSyncedTime() {
      SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
      Long lastSendNotSyncedTime = sharedPref.getLong(PREF_LAST_TIME_SEND_NOT_SYNCED_NAME, 0);
      return lastSendNotSyncedTime;
    }

    private void SendNotSyncedRecords() {
        Long lastSendNotSyncedTime = LoadLastSendNotSyncedTime();
        Long currentTime = Calendar.getInstance().getTimeInMillis();

        if (currentTime - lastSendNotSyncedTime < INTERVAL_RESEND_NOT_SYNCED) {
            return;
        }
        SaveLastSendNotSyncedTime(currentTime);
        synchronized (mSendSyncDataThread) {
            // Make sure we don't have pending send bookmarks operations
            if (mBulkBookmarkOperations.length() == 0 && mAttepmtsBeforeSendingNotSyncedRecords-- <= 0) {
                ProcessNotSyncedRecords(SyncRecordType.BOOKMARKS, CREATE_RECORD);
                ProcessNotSyncedRecords(SyncRecordType.BOOKMARKS, UPDATE_RECORD);
                ProcessNotSyncedRecords(SyncRecordType.BOOKMARKS, DELETE_RECORD);
            }
        }
        // Process preferences (aka devices)
        ProcessNotSyncedRecords(SyncRecordType.PREFERENCES, CREATE_RECORD);
        ProcessNotSyncedRecords(SyncRecordType.PREFERENCES, UPDATE_RECORD);
        ProcessNotSyncedRecords(SyncRecordType.PREFERENCES, DELETE_RECORD);
    }

    private void ProcessNotSyncedRecords(String categoryName, String action) {
        ArrayList<String> ids = SaveGetDeleteNotSyncedRecords(categoryName, action, new ArrayList<String>(), NotSyncedRecordsOperation.GetItems);
        if (categoryName.equals(SyncRecordType.BOOKMARKS)) {
            ArrayList<BookmarkItem> items = GetBookmarkItemsByLocalIds(ids, action);
            BookmarkItem[] itemsArray = new BookmarkItem[items.size()];
            itemsArray = items.toArray(itemsArray);
            CreateUpdateDeleteBookmarks(action, itemsArray, false, false);
        } else if (categoryName.equals(SyncRecordType.PREFERENCES)) {
            for (String id : ids) {
                String objectId = GetDeviceObjectIdByLocalId(id);
                String deviceName = GetDeviceNameByObjectId(objectId);
                SetUpdateDeleteDeviceName(action, deviceName, id, objectId);
            }
        } else {
            // Other categories are not handled at the momemt
            assert false;
        }
    }

    private void DeviceResolver(List<ResolvedRecordToApply> resolvedRecords) {
        //Log.i(TAG, "DeviceResolver: resolvedRecords.size(): " + resolvedRecords.size());
        assert null != resolvedRecords;

        if (0 == resolvedRecords.size() &&
            (mSyncScreensObserver == null || !mSyncScreensObserver.shouldLoadDevices())) {
            // When there are no changes in devices from sync cloud, but devices
            // page is currently opened in sync settings, reload in anyway devices list
            return;
        }

        String object = nativeGetObjectIdByLocalId(DEVICES_NAMES);

        List<ResolvedRecordToApply> existingRecords = new ArrayList<ResolvedRecordToApply>();
        if (!object.isEmpty()) {
          try {
              JSONObject result = new JSONObject(object);
              JSONArray devices = result.getJSONArray("devices");
              for (int i = 0; i < devices.length(); i++) {
                  JSONObject device = devices.getJSONObject(i);
                  String deviceName = device.getString("name");
                  String currentObject = device.getString("objectId");
                  String deviceId = device.getString("deviceId");
                  existingRecords.add(new ResolvedRecordToApply(currentObject, "0", null, deviceName, deviceId, 0));
              }
          } catch (JSONException e) {
              Log.e(TAG, "DeviceResolver JSONException error " + e);
          } catch (IllegalStateException e) {
              Log.e(TAG, "DeviceResolver IllegalStateException error " + e);
          }
        }

        for (ResolvedRecordToApply resolvedRecord: resolvedRecords) {
            assert !resolvedRecord.mDeviceName.isEmpty();
            boolean exist = false;
            ResolvedRecordToApply existingRecordToRemove = null;
            for (ResolvedRecordToApply existingRecord: existingRecords) {
                if (existingRecord.mObjectId.equals(resolvedRecord.mObjectId)) {
                    if (resolvedRecord.mAction.equals(DELETE_RECORD)) {
                        existingRecordToRemove = existingRecord;
                    } else if (resolvedRecord.mAction.equals(UPDATE_RECORD)) {
                        existingRecord.mDeviceName = resolvedRecord.mDeviceName;
                    }
                    exist = true;
                    break;
                }
            }
            if (null != existingRecordToRemove) {
                if (existingRecordToRemove.mDeviceId.equals(mDeviceId)) {
                    // We deleted current device, so need to reset sync
                    //Log.i(TAG, "DeviceResolver reset sync for " + resolvedRecord.mDeviceName);
                    ResetSync();
                }
                //Log.i(TAG, "DeviceResolver remove from list device " + resolvedRecord.mDeviceName);
                existingRecords.remove(existingRecordToRemove);
            }
            if (!exist && !resolvedRecord.mAction.equals(DELETE_RECORD)) {
                // TODO add to the list
                existingRecords.add(resolvedRecord);
            }
        }
        // TODO add or remove devices in devices list
        JSONObject result = new JSONObject();
        try {
            JSONArray devices = new JSONArray();
            for (ResolvedRecordToApply existingRecord: existingRecords) {
                JSONObject device = new JSONObject();
                device.put("name", replaceUnsupportedCharacters(existingRecord.mDeviceName));
                device.put("objectId", existingRecord.mObjectId);
                device.put("deviceId", existingRecord.mDeviceId);
                devices.put(device);
            }
            result.put("devices", devices);
        } catch (JSONException e) {
            Log.e(TAG, "DeviceResolver JSONException error " + e);
        }
        nativeSaveObjectId(DEVICES_NAMES, result.toString(), "");
        if (null != mSyncScreensObserver && !mSyncIsReady.mShouldResetSync) {
            mSyncScreensObserver.onDevicesAvailable();
        }
    }

    private boolean BookmarkResolver(ResolvedRecordToApply resolvedRecord) {
        // Return true if we need to skip that folder
        if (resolvedRecord == null || resolvedRecord.mBookmarkInternal == null) {
            assert false;
            return false;
        }
        String localId = nativeGetLocalIdByObjectId(resolvedRecord.mObjectId);
        if (localId.isEmpty() && resolvedRecord.mAction.equals(DELETE_RECORD)) {
            // Just skip that item as it is not locally and was deleted
            return true;
        }
        String parentLocalId = nativeGetLocalIdByObjectId(resolvedRecord.mBookmarkInternal.mParentFolderObjectId);
        if (!resolvedRecord.mBookmarkInternal.mParentFolderObjectId.isEmpty() && parentLocalId.isEmpty()) {
            PushOrphanBookmark(resolvedRecord);
            // Orphan bookmark will be applied once its parent pops up
            return true;
        }
        if (0 != localId.length()) {
            if (resolvedRecord.mAction.equals(UPDATE_RECORD)) {
                EditBookmarkByLocalId(localId, resolvedRecord.mBookmarkInternal.mUrl,
                    (resolvedRecord.mBookmarkInternal.mCustomTitle.isEmpty() ? resolvedRecord.mBookmarkInternal.mTitle : resolvedRecord.mBookmarkInternal.mCustomTitle),
                    parentLocalId, resolvedRecord.mObjectId, resolvedRecord.mBookmarkInternal.mOrder);
            } else if (resolvedRecord.mAction.equals(DELETE_RECORD)) {
                DeleteBookmarkByLocalId(localId);
            } else {
                //assert false;
                // Ignore of adding an existing object
            }
        } else {
            AddBookmark(resolvedRecord.mBookmarkInternal.mUrl,
                (resolvedRecord.mBookmarkInternal.mCustomTitle.isEmpty() ? resolvedRecord.mBookmarkInternal.mTitle : resolvedRecord.mBookmarkInternal.mCustomTitle),
                resolvedRecord.mBookmarkInternal.mIsFolder, resolvedRecord.mObjectId, parentLocalId, resolvedRecord.mBookmarkInternal.mOrder);
            if (resolvedRecord.mBookmarkInternal.mIsFolder) {
                // Check for orphan children
                List<ResolvedRecordToApply> orphanBookmarksRecords = PopOrphanBookmarksForParent(resolvedRecord.mObjectId);
                for (ResolvedRecordToApply bookmarkRecord: orphanBookmarksRecords) {
                    BookmarkResolver(bookmarkRecord);
                }
            }
        }

        return false;
    }

    public void DeleteSyncUser() {
        // TODO
    }

    public void DeleteSyncCategory() {
        // TODO
    }

    public void DeleteSyncSiteSettings() {
        // TODO
    }

    private StringBuilder GetAction(JsonReader reader) throws IOException {
        if (null == reader) {
            return new StringBuilder(CREATE_RECORD);
        }
        int action = reader.nextInt();
        if (1 == action) {
            return new StringBuilder(UPDATE_RECORD);
        } else if (2 == action) {
            return new StringBuilder(DELETE_RECORD);
        }

        return new StringBuilder(CREATE_RECORD);
    }

    private StringBuilder GetDeviceId(JsonReader reader) throws IOException {
        StringBuilder deviceId = new StringBuilder("");
        if (null == reader) {
            return deviceId;
        }

        if (JsonToken.BEGIN_OBJECT == reader.peek()) {
            reader.beginObject();
            while (reader.hasNext()) {
                reader.nextName();
                if (0 != deviceId.length()) {
                    deviceId.append(", ");
                }
                deviceId.append(reader.nextString());
            }
            reader.endObject();
        } else {
            reader.beginArray();
            while (reader.hasNext()) {
                if (0 != deviceId.length()) {
                    deviceId.append(", ");
                }
                deviceId.append(reader.nextInt());
            }
            reader.endArray();
        }

        return deviceId;
    }

    private StringBuilder GetObjectIdJSON(JsonReader reader) throws IOException {
        StringBuilder objectId = new StringBuilder("");
        if (null == reader) {
            return objectId;
        }

        JsonToken objectType = reader.peek();
        if (JsonToken.BEGIN_OBJECT == reader.peek()) {
            reader.beginObject();
            while (reader.hasNext()) {
                reader.nextName();
                if (0 != objectId.length()) {
                    objectId.append(", ");
                }
                objectId.append(reader.nextInt());
            }
            reader.endObject();
        } else if (JsonToken.BEGIN_ARRAY == reader.peek()) {
            reader.beginArray();
            while (reader.hasNext()) {
                if (0 != objectId.length()) {
                    objectId.append(", ");
                }
                objectId.append(reader.nextInt());
            }
            reader.endArray();
        } else if (JsonToken.NULL == reader.peek()) {
            reader.nextNull();
        } else {
            assert false;
            //objectId = String.valueOf(reader.nextInt());
        }

        return objectId;
    }

    private StringBuilder GetObjectDataJSON(JsonReader reader) throws IOException {
        if (null == reader) {
            return new StringBuilder("");
        }

        return new StringBuilder(reader.nextString());
    }

    private BookmarkInternal GetBookmarkRecord(JsonReader reader) throws IOException {
        BookmarkInternal bookmarkInternal = new BookmarkInternal();
        if (null == reader || null == bookmarkInternal) {
            return null;
        }

        reader.beginObject();
        while (reader.hasNext()) {
            String name = reader.nextName();
            if (name.equals("site")) {
                reader.beginObject();
                while (reader.hasNext()) {
                    name = reader.nextName();
                    if (name.equals("location")) {
                        bookmarkInternal.mUrl = reader.nextString();
                    } else if (name.equals("title")) {
                        bookmarkInternal.mTitle = reader.nextString();
                    } else if (name.equals("customTitle")) {
                        bookmarkInternal.mCustomTitle = reader.nextString();
                    } else if (name.equals("lastAccessedTime")) {
                        bookmarkInternal.mLastAccessedTime = reader.nextLong();
                    } else if (name.equals("creationTime")) {
                        bookmarkInternal.mCreationTime = reader.nextLong();
                    } else if (name.equals("favicon")) {
                        bookmarkInternal.mFavIcon = reader.nextString();
                    } else {
                        assert false;
                        reader.skipValue();
                    }
                }
                reader.endObject();
            } else if (name.equals("isFolder")) {
                if (JsonToken.BOOLEAN == reader.peek()) {
                    bookmarkInternal.mIsFolder = reader.nextBoolean();
                } else {
                    bookmarkInternal.mIsFolder = (reader.nextInt() != 0);
                }
            } else if (name.equals("order")) {
                bookmarkInternal.mOrder = reader.nextString();
            } else if (name.equals("parentFolderObjectId")) {
                bookmarkInternal.mParentFolderObjectId = GetObjectIdJSON(reader).toString();
            }
            else {
                reader.skipValue();
            }
        }
        reader.endObject();

        return bookmarkInternal;
    }

    private StringBuilder GetDeviceName(JsonReader reader) throws IOException {
        StringBuilder res = new StringBuilder("");

        if (null == reader) {
            return res;
        }

        reader.beginObject();
        while (reader.hasNext()) {
            String name = reader.nextName();
            if (name.equals("name")) {
                res.append(reader.nextString());
            } else {
                reader.skipValue();
            }
        }
        reader.endObject();

        return res;
    }

    class EjectedRunnable implements Runnable {
        private StringBuilder mJsToExecute;

        public EjectedRunnable(StringBuilder jsToExecute) {
            mJsToExecute = jsToExecute;
            mJsToExecute.insert(0, "javascript:(function() { ");
            mJsToExecute.append(" })()");
        }

        @Override
        public void run() {
            synchronized (mSyncIsReady) {
                if (null == mWebContents || null == mJsToExecute) {
                    Log.e(TAG, "mWebContents is null");
                    return;
                }
                mWebContents.getNavigationController().loadUrl(new LoadUrlParams(mJsToExecute.toString()));
            }
        }
    }

    class GetBookmarkItemsByLocalIdsRunnable implements Runnable {
        public ArrayList<BookmarkItem> mBookmarkItems;
        private ArrayList<Long> mBookmarkIds;
        private String mAction;

        public GetBookmarkItemsByLocalIdsRunnable(ArrayList<Long> bookmarkIds, String action) {
            mBookmarkIds = bookmarkIds;
            mAction = action;
            mBookmarkItems = new ArrayList<BookmarkItem>();
        }

        @Override
        public void run() {
            if (null != mBookmarkIds) {
                for (Long id: mBookmarkIds) {
                    BookmarkItem bookmarkItem = BookmarkItemByBookmarkId(id, /*newBookmarkModelAcquiredByThisRunnableWaiter*/false);
                    if (null != bookmarkItem) {
                        mBookmarkItems.add(bookmarkItem);
                    } else if (mAction.equals(DELETE_RECORD)) {
                        long defaultFolderId = (null != mDefaultFolder ? mDefaultFolder.getId() : 0);
                        mBookmarkItems.add(BraveBookmarkModel.createBookmarkItem(id, BookmarkType.NORMAL, "", "", false,
                            defaultFolderId, BookmarkType.NORMAL, true, true));
                    }
                }
            }

            synchronized (this)
            {
                this.notify();
            }
        }
    }

    class GetBookmarkIdRunnable implements Runnable {
        public BookmarkItem mBookmarkItem;
        private long mBookmarkId;
        private boolean mNewBookmarkModelAcquiredByThisRunnableWaiter;

        public GetBookmarkIdRunnable(long bookmarkId) {
            mBookmarkId = bookmarkId;
            mBookmarkItem = null;
        }

        public void SetNewBookmarkModelAcquiredByThisRunnableWaiter() {
          mNewBookmarkModelAcquiredByThisRunnableWaiter = true;
        }

        @Override
        public void run() {
            mBookmarkItem = BookmarkItemByBookmarkId(mBookmarkId, mNewBookmarkModelAcquiredByThisRunnableWaiter);

            synchronized (this)
            {
                this.notify();
            }
        }
    }

    private BookmarkItem BookmarkItemByBookmarkId(long lBookmarkId, boolean newBookmarkModelAcquiredByThisRunnableWaiter) {
        BookmarkItem bookmarkItem = null;
        BookmarkId bookmarkId = new BookmarkId(lBookmarkId, BookmarkType.NORMAL);
        if (null != mNewBookmarkModel && null != bookmarkId) {
            // alexeyb:
            // In this case it is safe to ignore acquiring of mNewBookmarkModel
            // because it is acquired by caller and the caller does not do
            // anything until GetBookmarkIdRunnable completes.
            // In any other cases `newBookmarkModelAcquiredByThisRunnableWaiter`
            // should be used with complete understanding what is going on and
            // why it is safe.
            if (false == newBookmarkModelAcquiredByThisRunnableWaiter) {
              synchronized (mNewBookmarkModel) {
                  if (mNewBookmarkModel.doesBookmarkExist(bookmarkId)) {
                      bookmarkItem = mNewBookmarkModel.getBookmarkById(bookmarkId);
                    }
             }
            } else {
              if (mNewBookmarkModel.doesBookmarkExist(bookmarkId)) {
                  bookmarkItem = mNewBookmarkModel.getBookmarkById(bookmarkId);
              }
            }
        }

        return bookmarkItem;
    }

    class SetExtensiveBookmarkOperationRunnable implements Runnable {
        private boolean mExtensiveOperation;

        public SetExtensiveBookmarkOperationRunnable(boolean extensiveOperation) {
             mExtensiveOperation = extensiveOperation;
        }

        @Override
        public void run() {
            if (null != mNewBookmarkModel && mNewBookmarkModel.isBookmarkModelLoaded()) {
                if (!mExtensiveOperation) {
                    mNewBookmarkModel.extensiveBookmarkChangesEnded();
                } else {
                    mNewBookmarkModel.extensiveBookmarkChangesBeginning();
                }
            }

            synchronized (this)
            {
                this.notify();
            }
        }
    }

    private void SetExtensiveBookmarkOperation(boolean extensiveOperation) {
        SetExtensiveBookmarkOperationRunnable extensiveOperationRunnable = new SetExtensiveBookmarkOperationRunnable(extensiveOperation);
        if (null == extensiveOperationRunnable) {
           return;
        }
        synchronized (extensiveOperationRunnable)
        {
            // Execute code on UI thread
            ThreadUtils.runOnUiThread(extensiveOperationRunnable);

            // Wait until runnable is finished
            try {
                extensiveOperationRunnable.wait();
            } catch (InterruptedException e) {
            }
        }
    }

    class GetDefaultFolderIdRunnable implements Runnable {
        public GetDefaultFolderIdRunnable() {
        }

        @Override
        public void run() {
            GetDefaultFolderIdInUIThread();

            synchronized (this)
            {
                this.notify();
            }
        }
    }

    private void GetDefaultFolderId() {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            GetDefaultFolderIdInUIThread();

            return;
        }

        GetDefaultFolderIdRunnable folderIdRunnable = new GetDefaultFolderIdRunnable();
        if (null == folderIdRunnable) {
           return;
        }
        synchronized (folderIdRunnable)
        {
            // Execute code on UI thread
            ThreadUtils.runOnUiThread(folderIdRunnable);

            // Wait until runnable is finished
            try {
                folderIdRunnable.wait();
            } catch (InterruptedException e) {
            }
        }
    }

    private void GetDefaultFolderIdInUIThread() {
        if (null == mNewBookmarkModel) {
            mNewBookmarkModel = new BraveBookmarkModel();
        }
        if (null != mNewBookmarkModel) {
            // Partner bookmarks need to be loaded explicitly so that BookmarkModel can be loaded.
            PartnerBookmarksShim.kickOffReading(mContext);
            mNewBookmarkModel.finishLoadingBookmarkModel(new Runnable() {
                @Override
                public void run() {
                  BookmarkId bookmarkId = mNewBookmarkModel.getMobileFolderId();

                  mDefaultFolder = bookmarkId;
                }
            });
        }
    }

    class EditBookmarkRunnable implements Runnable {
        private long mBookmarkId;
        private String mUrl;
        private String mTitle;
        private long mParentLocalId;

        public EditBookmarkRunnable(long bookmarkId, String url, String title, long parentLocalId) {
            mBookmarkId = bookmarkId;
            mUrl = url;
            mTitle = title;
            mParentLocalId = parentLocalId;
        }

        @Override
        public void run() {
            BookmarkId parentBookmarkId = null;
            long defaultFolderId = (null != mDefaultFolder ? mDefaultFolder.getId() : 0);
            if (defaultFolderId != mParentLocalId) {
                parentBookmarkId = new BookmarkId(mParentLocalId, BookmarkType.NORMAL);
            } else {
                parentBookmarkId = mDefaultFolder;
                assert mDefaultFolder != null;
            }
            BookmarkId bookmarkId = new BookmarkId(mBookmarkId, BookmarkType.NORMAL);
            if (null != mNewBookmarkModel && null != bookmarkId) {
                synchronized (mNewBookmarkModel) {
                    if (mNewBookmarkModel.doesBookmarkExist(bookmarkId)) {
                        BookmarkItem bookmarkItem = mNewBookmarkModel.getBookmarkById(bookmarkId);
                        if (null != bookmarkItem) {
                            if (!bookmarkItem.getTitle().equals(mTitle)) {
                                mNewBookmarkModel.setBookmarkTitle(bookmarkId, mTitle);
                            }
                            if (!mUrl.isEmpty() && bookmarkItem.isUrlEditable()) {
                                String fixedUrl = UrlFormatter.fixupUrl(mUrl);
                                if (null != fixedUrl && !fixedUrl.equals(bookmarkItem.getTitle())) {
                                    mNewBookmarkModel.setBookmarkUrl(bookmarkId, fixedUrl);
                                }
                            }
                            if (bookmarkItem.getParentId().getId() != mParentLocalId) {
                                mNewBookmarkModel.moveBookmark(bookmarkId, parentBookmarkId);
                            }
                        }
                    }
                }
            }

            synchronized (this)
            {
                this.notify();
            }
        }
    }

    class AddBookmarkRunnable implements Runnable {
        private BookmarkId mBookmarkId;
        private String mUrl;
        private String mTitle;
        private boolean misFolder;
        private long mParentLocalId;

        public AddBookmarkRunnable(String url, String title, boolean isFolder, long parentLocalId) {
            mUrl = url;
            mTitle = title;
            misFolder = isFolder;
            mParentLocalId = parentLocalId;
        }

        @Override
        public void run() {
            BookmarkId parentBookmarkId = null;
            long defaultFolderId = (null != mDefaultFolder ? mDefaultFolder.getId() : 0);
            if (defaultFolderId != mParentLocalId) {
                parentBookmarkId = new BookmarkId(mParentLocalId, BookmarkType.NORMAL);
            } else {
                parentBookmarkId = mDefaultFolder;
            }
            if (null != mNewBookmarkModel) {
                synchronized (mNewBookmarkModel) {
                    if (!misFolder) {
                        mBookmarkId = BraveBookmarkUtils.addBookmarkSilently(mContext, mNewBookmarkModel, mTitle, mUrl, parentBookmarkId);
                    } else {
                        mBookmarkId = mNewBookmarkModel.addFolder(parentBookmarkId, 0, mTitle);
                    }
                }
            }

            synchronized (this)
            {
                this.notify();
            }
        }
    }

    public boolean IsSyncEnabled() {
        boolean prefSyncDefault = false;
        boolean prefSync = mSharedPreferences.getBoolean(
                PREF_SYNC_SWITCH, prefSyncDefault);
        return prefSync;
    }

    public void SetSyncEnabled(boolean syncEnabled) {
        mSharedPreferences.edit().putBoolean(PREF_SYNC_SWITCH, syncEnabled).apply();
    }

    public boolean IsSyncBookmarksEnabled() {
        boolean prefSyncBookmarksDefault = true;
        boolean prefSyncBookmarks = mSharedPreferences.getBoolean(
                PREF_SYNC_BOOKMARKS, prefSyncBookmarksDefault);
        return prefSyncBookmarks;
    }

    public void SetSyncBookmarksEnabled(boolean syncBookmarksEnabled) {
        mSharedPreferences.edit().putBoolean(PREF_SYNC_BOOKMARKS, syncBookmarksEnabled).apply();
    }

    class SyncThread extends Thread {
        @Override
        public void run() {
            SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
            mTimeLastFetch = sharedPref.getLong(PREF_LAST_FETCH_NAME, 0);
            mDeviceId = sharedPref.getString(PREF_DEVICE_ID, null);
            mDeviceName = sharedPref.getString(PREF_SYNC_DEVICE_NAME, null);
            mBaseOrder = sharedPref.getString(PREF_BASE_ORDER, null);
            mLastOrder = sharedPref.getString(PREF_LAST_ORDER, null);
            InitOrphanBookmarks();

            for (;;) {
                try {
                    if (IsSyncEnabled()) {
                        InitSync(false, false);
                        Calendar currentTime = Calendar.getInstance();
                        long timeLastFetch = currentTime.getTimeInMillis();
                        if (!mFetchInProgress || timeLastFetch - mTimeLastFetchExecuted > INTERVAL_TO_REFETCH_RECORDS) {
                            mFetchInProgress = false;
                            FetchSyncRecords("", "");
                        }
                    }
                    for (int i = 0; i < BraveSyncWorker.SYNC_SLEEP_ATTEMPTS_COUNT; i++) {
                        if (i == BraveSyncWorker.SYNC_SLEEP_ATTEMPTS_COUNT / 2) {
                            // SZ: preventing from page been frozen, we do that on the middle of the loop
                            ThreadUtils.runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    synchronized (mSyncIsReady) {
                                        if (null != mWebContents) {
                                            mWebContents.onHide();
                                            mWebContents.onShow();
                                        }
                                        if (mJSWebContents != null) {
                                            mJSWebContents.onHide();
                                            mJSWebContents.onShow();
                                        }
                                    }
                                }
                            });
                        }
                        Thread.sleep(BraveSyncWorker.INTERVAL_TO_FETCH_RECORDS / BraveSyncWorker.SYNC_SLEEP_ATTEMPTS_COUNT);
                        if (mInterruptSyncSleep) {
                            break;
                        }
                    }
                }
                catch(Exception exc) {
                    // Just ignore it if we cannot sync
                    Log.e(TAG, "Sync loop exception: " + exc);
                }
                if (mStopThread) {
                    break;
                }
            }
        }
    }

    class SendSyncDataThread extends Thread {
        @Override
        public void run() {
            for (;;) {
                try {
                    Thread.sleep(BraveSyncWorker.INTERVAL_TO_SEND_SYNC_RECORDS);
                    if (IsSyncEnabled() && mSyncIsReady.mReady) {
                        SendBulkBookmarks();
                    }
                } catch (InterruptedException e) {
                    Log.w(TAG, "Send sync data thread interrupted: " + e);
                }
                if (mStopThread) {
                    break;
                }
            }
        }
    }

    public void ResetSync() {
        ResetSyncWebContents();
        SetSyncEnabled(false);
        mSyncIsReady.Reset();
        mSyncIsReady.mShouldResetSync = true;
        SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.remove(PREF_LAST_FETCH_NAME);
        editor.remove(PREF_DEVICE_ID);
        editor.remove(PREF_BASE_ORDER);
        editor.remove(PREF_LAST_ORDER);
        editor.remove(PREF_SEED);
        editor.remove(PREF_SYNC_DEVICE_NAME);
        editor.apply();
        final String seed = mSeed;
        mSeed = null;
        mDeviceId = null;
        mDeviceName = null;
        mBaseOrder = null;
        mLastOrder = null;
        mTimeLastFetch = 0;
        mTimeLastFetchExecuted = 0;
        if (null != mSyncScreensObserver) {
            mSyncScreensObserver.onResetSync();
        }
        new Thread() {
            @Override
            public void run() {
              nativeResetSync(ORIGINAL_SEED_KEY);
              nativeResetSync(SyncRecordType.BOOKMARKS + CREATE_RECORD);
              nativeResetSync(SyncRecordType.BOOKMARKS + UPDATE_RECORD);
              nativeResetSync(SyncRecordType.BOOKMARKS + DELETE_RECORD);
              nativeResetSync(SyncRecordType.PREFERENCES + CREATE_RECORD);
              nativeResetSync(SyncRecordType.PREFERENCES + UPDATE_RECORD);
              nativeResetSync(SyncRecordType.PREFERENCES + DELETE_RECORD);
              nativeResetSync(DEVICES_NAMES);
              nativeResetSync(ORPHAN_BOOKMARKS);
              nativeResetSync(THIS_DEVICE_OBJECT_ID);
              mOrphanBookmarks.clear();
              SaveObjectId(ORIGINAL_SEED_KEY, seed, "", true);
              // TODO for other categories type
            }
        }.start();
    }

    public void InitSync(boolean calledFromUIThread, boolean startNewChain) {
          if (!startNewChain) {
              // Here we already supposed to get existing seed
              SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
              if (null == mSeed || mSeed.isEmpty()) {
                  mSeed = sharedPref.getString(PREF_SEED, null);
              }
              if (null == mSeed || mSeed.isEmpty()) {
                  return;
              }
          }
          // Init sync WebView
          if (!calledFromUIThread) {
            ThreadUtils.runOnUiThread(new Runnable() {
                  @Override
                  public void run() {
                     TrySync();
                  }
              });
          } else {
              TrySync();
          }
    }

    class JsObject {
        @JavascriptInterface
        public void handleMessage(String message, String arg1, String arg2, String arg3, boolean arg4) {
            if (!message.equals("sync-debug")) {
                Log.i(TAG, "!!!message == " + message);
            }
            switch (message) {
              case "get-init-data":
                break;
              case "got-init-data":
                GotInitData();
                break;
              case "save-init-data":
                SaveInitData(arg1, arg2);
                break;
              case "sync-debug":
                if (null != arg1) {
                    Log.i(TAG, "!!!sync-debug: " + arg1);
                }
                break;
              case "fetch-sync-records":
                mSyncIsReady.mFetchRecordsReady = true;
                break;
              case "resolve-sync-records":
                mSyncIsReady.mResolveRecordsReady = true;
                break;
              case "resolved-sync-records":
                ResolvedSyncRecords(arg1, arg2);
                break;
              case "send-sync-records":
                mSyncIsReady.mSendRecordsReady = true;
                break;
              case "delete-sync-user":
                mSyncIsReady.mDeleteUserReady = true;
                break;
              case "deleted-sync-user":
                break;
              case "delete-sync-category":
                mSyncIsReady.mDeleteCategoryReady = true;
                break;
              case "delete-sync-site-settings":
                mSyncIsReady.mDeleteSiteSettingsReady = true;
                break;
              case "sync-ready":
                if (mBaseOrder == null || mBaseOrder.isEmpty()) {
                    // Get sync order prefix
                    CallScript(new StringBuilder(String.format("javascript:callbackList['get-bookmarks-base-order'](null, %1$s, 'android')", mDeviceId)));
                } else {
                    StartSync();
                }
                break;
              case "get-existing-objects":
                SendResolveSyncRecords(arg1, GetExistingObjects(arg1, arg2, arg3, arg4));
                break;
              case "get-bookmarks-base-order":
                break;
              case "save-bookmarks-base-order":
                assert arg1 != null;
                // Save base order before sending local bookmarks
                mBaseOrder = arg1;
                SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putString(PREF_BASE_ORDER, mBaseOrder);
                editor.apply();
                StartSync();
                break;
              case "get-bookmark-order":
                break;
              case "save-bookmark-order":
                //Log.i(TAG, "!!!save-bookmark-order1 arg1 == " + arg1 + ", arg2 == " + arg2 + ", arg3 == " + arg3);
                break;
              case "sync-setup-error":
                Log.e(TAG, "sync-setup-error , !!!arg1 == " + arg1 + ", arg2 == " + arg2);
                if (!SyncHasBeenSetup()) {
                    // We need to reset state before the next attempt to set up sync
                    ResetSync();
                } else {
                    // We need to reset web contents to recreate it when network is up again
                    ResetSyncWebContents();
                }
                if (null != mSyncScreensObserver) {
                    mSyncScreensObserver.onSyncError(arg1);
                }
                break;
              default:
                Log.w(TAG, "!!!message == " + message + ", !!!arg1 == " + arg1 + ", arg2 == " + arg2);
                break;
            }
        }
    }

    class JsObjectWordsToBytes {
        @JavascriptInterface
        public void cryptoOutput(String result) {
            if (null == result || 0 == result.length()) {
                if (null != mSyncScreensObserver) {
                    mSyncScreensObserver.onSyncError("Incorrect crypto output");
                }
                return;
            }

            JsonReader reader = null;
            String seed = "";
            try {
                JSONObject data = new JSONObject(result);
                Iterator<String> it = data.keys();
                // Pick up keys to sort them, as it's not always returned in proper order
                // We use Integer for proper sorting, as strings are sorted incorrectly
                ArrayList<Integer> keys = new ArrayList<Integer>();
                while (it.hasNext()) {
                    String key = it.next();
                    keys.add(Integer.parseInt(key));
                }
                Collections.sort(keys);
                // Get data by sorted keys
                for (Integer key : keys) {
                    String value = data.getString(Integer.toString(key));
                    if (0 != seed.length()) {
                        seed += ",";
                    }
                    seed += value;
                }
            } catch (JSONException e) {
                Log.e(TAG, "cryptoOutput JSONException error " + e);
                if (null != mSyncScreensObserver) {
                    mSyncScreensObserver.onSyncError("cryptoOutput JSONException error " + e);
                }
            }
            //Log.i(TAG, "!!!seed == " + seed);

            if (null != mSyncScreensObserver) {
                mSyncScreensObserver.onSeedReceived(seed, true, false);
            }
        }

        @JavascriptInterface
        public void cryptoOutputCodeWords(String result) {
            if (null == result || 0 == result.length()) {
                if (null != mSyncScreensObserver) {
                    mSyncScreensObserver.onSyncError("Incorrect crypto output for code words");
                }
                return;
            }

            String[] codeWords = result.replace('\"', ' ').trim().split(" ");

            if (NICEWARE_WORD_COUNT != codeWords.length && BIP39_WORD_COUNT != codeWords.length) {
                Log.e(TAG, "Incorrect number of code words");
                if (null != mSyncScreensObserver) {
                    mSyncScreensObserver.onSyncError("Incorrect number of code words");
                }
                return;
            }

            if (null != mSyncScreensObserver) {
                mSyncScreensObserver.onCodeWordsReceived(codeWords);
            }
        }

        @JavascriptInterface
        public void cryptoOutputError(String error) {
            if (null != mSyncScreensObserver) {
                mSyncScreensObserver.onSyncError(error);
            }
        }
    }

    public void InitJSWebView(BraveSyncScreensObserver syncScreensObserver) {
        try {
            synchronized (mSyncIsReady) {
                if (null == mJSWebContents) {
                    mJSWebContents = WebContentsFactory.createWebContents(false, true);
                    if (null != mJSWebContents) {
                        ContentView cv = ContentView.createContentView(mContext, mJSWebContents);
                        mJSWebContents.initialize(null, ViewAndroidDelegate.createBasicDelegate(cv), cv, new WindowAndroid(mContext), WebContents.createDefaultInternalsHolder());
                        mJSViewEventSink = ViewEventSinkImpl.from(mJSWebContents);
                        if (null != mJSViewEventSink) {
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
                                initJSContenViewCore();
                            } else {
                                getJSWebContentsInjector().addPossiblyUnsafeInterface(new JsObjectWordsToBytes(), "injectedObject", null);
                            }

                            String toLoad = "<script type='text/javascript'>";
                            try {
                                String script = convertStreamToString(mContext.getAssets().open(ANDROID_SYNC_WORDS_JS));
                                toLoad += script.replace("%", "%25").replace("\n", "%0A").replace("#", "%23") + "</script><script type='text/javascript'>";
                                script = convertStreamToString(mContext.getAssets().open(CRYPTO_JS));
                                toLoad += script.replace("%", "%25").replace("\n", "%0A").replace("#", "%23") + "</script>";
                            } catch (IOException exc) {}
                            LoadUrlParams loadUrlParams = LoadUrlParams.createLoadDataParamsWithBaseUrl(toLoad, "text/html", false, "file:///android_asset/", null);
                            loadUrlParams.setCanLoadLocalResources(true);
                            mJSWebContents.getNavigationController().loadUrl(loadUrlParams);
                        }
                    }
                }
            }
        } catch (Exception exc) {
            // Ignoring sync exception, we will try it on next loop execution
            Log.e(TAG, "InitJSWebView exception: " + exc);
        }
        // Always overwrite observer since it's focused on specific activity
        mSyncScreensObserver = syncScreensObserver;
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    private void initJSContenViewCore() {
        getJSWebContentsInjector().addPossiblyUnsafeInterface(new JsObjectWordsToBytes(), "injectedObject", JavascriptInterface.class);
    }

    public void GetNumber(String[] words) {
        synchronized (mSyncIsReady) {
            if (null == mJSWebContents) {
                return;
            }
            String wordsJSArray = "";
            for (int i = 0; i < words.length; i++) {
                if (0 == i) {
                    wordsJSArray = "'";
                } else {
                    wordsJSArray += " ";
                }
                wordsJSArray += words[i];
                if (words.length - 1 == i) {
                    wordsJSArray += "'";
                }
            }
            //Log.i(TAG, "!!!words == " + wordsJSArray);
            mJSWebContents.getNavigationController().loadUrl(
                    new LoadUrlParams("javascript:(function() { " + String.format("javascript:getBytesFromWords(%1$s)", wordsJSArray) + " })()"));
        }
    }

    private JavascriptInjector getWebContentsInjector() {
        synchronized (mSyncIsReady) {
            if (mWebContentsInjector == null) {
                mWebContentsInjector = JavascriptInjector.fromWebContents(mWebContents);
            }
            return mWebContentsInjector;
        }
    }

    private JavascriptInjector getJSWebContentsInjector() {
        synchronized (mSyncIsReady) {
            if (mJSWebContentsInjector == null) {
                mJSWebContentsInjector = JavascriptInjector.fromWebContents(mJSWebContents);
            }
            return mJSWebContentsInjector;
        }
    }

    public void GetCodeWords() {
        synchronized (mSyncIsReady) {
            if (null == mJSWebContents) {
                Log.e(TAG, "Error on receiving code words. JSWebContents is null.");
                return;
            }
            if (null == mSeed || mSeed.isEmpty()) {
                Log.e(TAG, "Error on receiving code words. Seed is empty.");
                return;
            }
            mJSWebContents.getNavigationController().loadUrl(
                    new LoadUrlParams("javascript:(function() { " + String.format("javascript:getCodeWordsFromSeed([%1$s])", mSeed) + " })()"));
        }
    }

    public void InterruptSyncSleep() {
        mInterruptSyncSleep = true;
    }

    private String GetBookmarkOrder(String localId, boolean generateIfEmpty) {
        String currentOrder = GetBookmarkOrder(localId);
        if (!currentOrder.isEmpty() || !generateIfEmpty) {
            return currentOrder;
        }
        if (mLastOrder == null || mLastOrder.isEmpty()) {
            assert mBaseOrder != null;
            // It is the very first element
            mLastOrder = mBaseOrder + "1";
        } else {
            assert !mLastOrder.isEmpty();
            String[] numbers = mLastOrder.split("\\.");
            assert numbers.length > 0;
            int newLastNumber = Integer.parseInt(numbers[numbers.length - 1]) + 1;
            mLastOrder = numbers[0] + ".";
            for (int i = 1; i < numbers.length - 1; i++) {
                mLastOrder += numbers[i] + ".";
            }
            mLastOrder += newLastNumber;
        }
        SharedPreferences sharedPref = mContext.getSharedPreferences(PREF_NAME, 0);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString(PREF_LAST_ORDER, mLastOrder);
        editor.apply();
        return mLastOrder;
    }

    private void StartSync() {
        mSyncIsReady.mReady = true;
        new Thread() {
            @Override
            public void run() {
                FetchSyncRecords("", "");
            }
        }.start();
    }

    private void PushOrphanBookmark(ResolvedRecordToApply record) {
        synchronized (mOrphanBookmarks) {
            mOrphanBookmarks.add(record);
            SaveOrphanBookmarks();
        }
    }

    private ArrayList<ResolvedRecordToApply> PopOrphanBookmarksForParent(String parentId) {
        ArrayList<ResolvedRecordToApply> result = new ArrayList<ResolvedRecordToApply>();
        synchronized (mOrphanBookmarks) {
            boolean saveOrphanBookmarks = false;
            for (int i = 0; i < mOrphanBookmarks.size(); i++) {
                if (mOrphanBookmarks.get(i).mBookmarkInternal == null) {
                    assert false;
                    continue;
                }
                if (mOrphanBookmarks.get(i).mBookmarkInternal.mParentFolderObjectId.equals(parentId)) {
                    result.add(mOrphanBookmarks.remove(i--));
                    saveOrphanBookmarks = true;
                }
            }
            if (saveOrphanBookmarks) {
                SaveOrphanBookmarks();
            }
        }
        return result;
    }

    private void InitOrphanBookmarks() {
        synchronized (mOrphanBookmarks) {
            String object = nativeGetObjectIdByLocalId(ORPHAN_BOOKMARKS);
            if (object.isEmpty()) {
                return;
            }
            try {
                JSONObject result = new JSONObject(object);
                JSONArray orphans = result.getJSONArray("orphans");
                for (int i = 0; i < orphans.length(); i++) {
                    JSONObject orphan = orphans.getJSONObject(i);
                    mOrphanBookmarks.add(ResolvedRecordToApplyFromJSONObject(orphan));
                }
            } catch (JSONException e) {
                Log.e(TAG, "InitOrphanBookmarks error: " + e);
            }
        }
    }

    private void SaveOrphanBookmarks() {
        JSONObject result = new JSONObject();
        try {
            JSONArray orphans = new JSONArray();
            for (ResolvedRecordToApply orphan: mOrphanBookmarks) {
                orphans.put(orphan.toJSONObject());
            }
            result.put("orphans", orphans);
        } catch (JSONException e) {
            Log.e(TAG, "SaveOrphanBookmarks error: " + e);
        }
        nativeSaveObjectId(ORPHAN_BOOKMARKS, result.toString(), "");
    }

    private boolean SyncHasBeenSetup() {
        // If base order is null or empty, it means that sync hasn't been set up yet
        return mBaseOrder != null && !mBaseOrder.isEmpty();
    }

    private native String nativeGetObjectIdByLocalId(String localId);
    private native String nativeGetLocalIdByObjectId(String objectId);
    private native void nativeSaveObjectId(String localId, String objectIdJSON, String objectId);
    private native void nativeDeleteByLocalId(String localId);
    private native void nativeClear();
    private native void nativeResetSync(String key);
  }

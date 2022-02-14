/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.view.View;
import android.widget.ImageView;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.content_public.browser.ImageDownloadCallback;
import org.chromium.content_public.browser.WebContents;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.GURL;

import java.util.Iterator;
import java.util.List;

@JNINamespace("permissions")
public class BravePermissionDialogDelegate
        implements ConnectionErrorHandler, ImageDownloadCallback {
    static final int MAX_BITMAP_SIZE_FOR_DOWNLOAD = 2048;
    /** Text to show before lifetime options. */
    private String mLifetimeOptionsText;

    /** Lifetime options to show to the user. Can be null if no options should be shown. */
    private String[] mLifetimeOptions;

    /** Lifetime option index selected by the user. */
    private int mSelectedLifetimeOption;

    private boolean mUseWalletLayout;
    private KeyringService mKeyringService;
    private String mConnectWalletTitle;
    private String mConnectWalletSubTitle;
    private String mConnectWalletAccountsTitle;
    private String mWalletWarningTitle;
    private String mConnectButtonText;
    private String mBackButtonText;
    private String mDomain;
    private String mFavIconURL;
    private ImageView mFavIcon;
    private WebContents mWebContents;
    // The pending download image request id, which is set when calling
    // {@link WebContents#downloadImage()}, and reset when image download completes
    private int mRequestId;
    private BraveAccountsListAdapter mAccountsListAdapter;

    public BravePermissionDialogDelegate() {
        mSelectedLifetimeOption = -1;
        mRequestId = -1;
        mKeyringService = null;
        mUseWalletLayout = false;
    }

    @CalledByNative
    public void setLifetimeOptionsText(String lifetimeOptionsText) {
        mLifetimeOptionsText = lifetimeOptionsText;
    }

    public String getLifetimeOptionsText() {
        return mLifetimeOptionsText;
    }

    @CalledByNative
    public void setLifetimeOptions(String[] lifetimeOptions) {
        mLifetimeOptions = lifetimeOptions;
    }

    public String[] getLifetimeOptions() {
        return mLifetimeOptions;
    }

    public void setSelectedLifetimeOption(int idx) {
        mSelectedLifetimeOption = idx;
    }

    @CalledByNative
    public int getSelectedLifetimeOption() {
        return mSelectedLifetimeOption;
    }

    public boolean getUseWalletLayout() {
        return mUseWalletLayout;
    }

    @CalledByNative
    public void setUseWalletLayout(boolean useWalletLayout) {
        mUseWalletLayout = useWalletLayout;
        if (mUseWalletLayout) {
            InitKeyringService();
        }
    }

    @CalledByNative
    public void setWalletConnectTitle(String title) {
        mConnectWalletTitle = title;
    }

    public String getWalletConnectTitle() {
        return mConnectWalletTitle;
    }

    @CalledByNative
    public void setWalletConnectSubTitle(String subTitle) {
        mConnectWalletSubTitle = subTitle;
    }

    public String getWalletConnectSubTitle() {
        return mConnectWalletSubTitle;
    }

    @CalledByNative
    public void setWalletConnectAccountsTitle(String subTitle) {
        mConnectWalletAccountsTitle = subTitle;
    }

    public String getWalletConnectAccountsTitle() {
        return mConnectWalletAccountsTitle;
    }

    @CalledByNative
    public void setWalletWarningTitle(String warningTitle) {
        mWalletWarningTitle = warningTitle;
    }

    public String getWalletWarningTitle() {
        return mWalletWarningTitle;
    }

    @CalledByNative
    public void setConnectButtonText(String connectButtonText) {
        mConnectButtonText = connectButtonText;
    }

    public String getConnectButtonText() {
        return mConnectButtonText;
    }

    @CalledByNative
    public void setBackButtonText(String backButtonText) {
        mBackButtonText = backButtonText;
    }

    public String getBackButtonText() {
        return mBackButtonText;
    }

    @CalledByNative
    public void setDomain(String domain) {
        mDomain = domain;
    }

    public String getDomain() {
        return mDomain;
    }

    @CalledByNative
    public void setWebContents(WebContents webContents) {
        mWebContents = webContents;
    }

    @CalledByNative
    public void setFavIconURL(String url) {
        mFavIconURL = url;
    }

    public void setFavIcon(ImageView imageView) {
        mFavIcon = imageView;
        if (mFavIconURL.isEmpty()) {
            return;
        }
        mRequestId = mWebContents.downloadImage(new GURL(mFavIconURL), // url
                true, // isFavicon
                MAX_BITMAP_SIZE_FOR_DOWNLOAD, // maxBitmapSize
                false, // bypassCache
                this); // callback
    }

    @Override
    public void onFinishDownloadImage(int id, int httpStatusCode, GURL imageUrl,
            List<Bitmap> bitmaps, List<Rect> originalImageSizes) {
        if (id != mRequestId) return;

        Iterator<Bitmap> iterBitmap = bitmaps.iterator();
        Iterator<Rect> iterSize = originalImageSizes.iterator();

        Bitmap bestBitmap = null;
        Rect bestSize = new Rect(0, 0, 0, 0);
        while (iterBitmap.hasNext() && iterSize.hasNext()) {
            Bitmap bitmap = iterBitmap.next();
            Rect size = iterSize.next();
            if (size.width() > bestSize.width() && size.height() > bestSize.height()) {
                bestBitmap = bitmap;
                bestSize = size;
            }
        }
        if (mFavIcon == null || bestSize.width() == 0 || bestSize.height() == 0) {
            return;
        }

        mFavIcon.setImageBitmap(bestBitmap);
        mFavIcon.setVisibility(View.VISIBLE);
    }

    @CalledByNative
    public String[] getSelectedAccounts() {
        assert mAccountsListAdapter != null;
        AccountInfo[] accountInfo = mAccountsListAdapter.getCheckedAccounts();
        String[] accounts = new String[accountInfo.length];
        for (int i = 0; i < accountInfo.length; i++) {
            accounts[i] = accountInfo[i].address;
        }

        return accounts;
    }

    @CalledByNative
    public void disconnectMojoServices() {
        if (mKeyringService == null) {
            return;
        }
        mKeyringService.close();
        mKeyringService = null;
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mKeyringService = null;
        InitKeyringService();
    }

    protected void InitKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
    }

    public KeyringService getKeyringService() {
        return mKeyringService;
    }

    public BraveAccountsListAdapter getAccountsListAdapter(AccountInfo[] accountInfo) {
        assert mAccountsListAdapter == null;
        mAccountsListAdapter = new BraveAccountsListAdapter(accountInfo);

        return mAccountsListAdapter;
    }
}

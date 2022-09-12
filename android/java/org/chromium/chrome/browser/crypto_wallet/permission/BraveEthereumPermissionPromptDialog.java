/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.permission;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.components.browser_ui.modaldialog.ModalDialogView;
import org.chromium.content_public.browser.ImageDownloadCallback;
import org.chromium.content_public.browser.WebContents;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.LayoutInflaterUtils;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.DialogDismissalCause;
import org.chromium.ui.modaldialog.ModalDialogManager;
import org.chromium.ui.modaldialog.ModalDialogManager.ModalDialogType;
import org.chromium.ui.modaldialog.ModalDialogProperties;
import org.chromium.ui.modaldialog.ModalDialogProperties.ButtonType;
import org.chromium.ui.modelutil.PropertyModel;
import org.chromium.url.GURL;

import java.util.Iterator;
import java.util.List;

public class BraveEthereumPermissionPromptDialog
        implements ModalDialogProperties.Controller, ImageDownloadCallback, ConnectionErrorHandler {
    static final int MAX_BITMAP_SIZE_FOR_DOWNLOAD = 2048;

    private final ModalDialogManager mModalDialogManager;
    private int mCoinType;
    private final Context mContext;
    private long mNativeDialogController;
    private PropertyModel mPropertyModel;
    private WebContents mWebContents;
    private String mFavIconURL;
    private ImageView mFavIconImage;
    private RecyclerView mRecyclerView;
    private BraveEthereumPermissionAccountsListAdapter mAccountsListAdapter;
    private int mRequestId; // Used for favicon downloader
    private KeyringService mKeyringService;
    private boolean mMojoServicesClosed;
    private BraveWalletService mBraveWalletService;
    private View mPermissionDialogPositiveButton;
    private WalletModel mWalletModel;

    @CalledByNative
    private static BraveEthereumPermissionPromptDialog create(long nativeDialogController,
            @NonNull WindowAndroid windowAndroid, WebContents webContents, String favIconURL,
            @CoinType.EnumType int coinType) {
        return new BraveEthereumPermissionPromptDialog(
                nativeDialogController, windowAndroid, webContents, favIconURL, coinType);
    }

    public BraveEthereumPermissionPromptDialog(long nativeDialogController,
            WindowAndroid windowAndroid, WebContents webContents, String favIconURL,
            @CoinType.EnumType int coinType) {
        mNativeDialogController = nativeDialogController;
        mWebContents = webContents;
        mFavIconURL = favIconURL;
        mContext = windowAndroid.getActivity().get();

        mModalDialogManager = windowAndroid.getModalDialogManager();
        mCoinType = coinType;
        mMojoServicesClosed = false;
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
        }
    }

    @CalledByNative
    void show() {
        View customView = LayoutInflaterUtils.inflate(
                mContext, R.layout.brave_permission_prompt_dialog, null);

        mFavIconImage = customView.findViewById(R.id.favicon);
        setFavIcon();
        mRecyclerView = customView.findViewById(R.id.accounts_list);

        InitBraveWalletService();
        TextView domain = customView.findViewById(R.id.domain);
        mBraveWalletService.geteTldPlusOneFromOrigin(Utils.getCurrentMojomOrigin(),
                origin -> { domain.setText(Utils.geteTLD(origin.eTldPlusOne)); });

        mPropertyModel =
                new PropertyModel.Builder(ModalDialogProperties.ALL_KEYS)
                        .with(ModalDialogProperties.CONTROLLER, this)
                        .with(ModalDialogProperties.CUSTOM_VIEW, customView)
                        .with(ModalDialogProperties.POSITIVE_BUTTON_TEXT,
                                mContext.getString(
                                        R.string.permissions_connect_brave_wallet_connect_button_text))
                        .with(ModalDialogProperties.NEGATIVE_BUTTON_TEXT,
                                mContext.getString(
                                        R.string.permissions_connect_brave_wallet_back_button_text))
                        .with(ModalDialogProperties.FILTER_TOUCH_FOR_SECURITY, true)
                        .build();
        mModalDialogManager.showDialog(mPropertyModel, ModalDialogType.APP);
        InitKeyringService();
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.dismissWalletPanelOrDialog();
        }
        ViewGroup container = getPermissionModalViewContainer(customView);
        mPermissionDialogPositiveButton =
                container.findViewById(activity.getResources().getIdentifier(
                        WalletConstants.PERMISSION_DIALOG_POSITIVE_BUTTON_ID,
                        WalletConstants.RESOURCE_ID, activity.getPackageName()));
        if (mPermissionDialogPositiveButton != null) {
            mPermissionDialogPositiveButton.setEnabled(false);
        }
        initAccounts();
    }

    @NonNull
    private ViewGroup getPermissionModalViewContainer(View customView) {
        ViewParent viewParent = customView.getParent();
        while (viewParent.getParent() != null) {
            viewParent = viewParent.getParent();
            if (viewParent instanceof ModalDialogView) {
                break;
            }
        }
        return (ViewGroup) viewParent;
    }

    private void InitBraveWalletService() {
        if (mBraveWalletService != null) {
            return;
        }
        mBraveWalletService = BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
    }

    @SuppressLint("NotifyDataSetChanged")
    private void initAccounts() {
        assert mKeyringService != null;
        assert mWalletModel != null;
        mAccountsListAdapter = new BraveEthereumPermissionAccountsListAdapter(new AccountInfo[0],
                true,
                new BraveEthereumPermissionAccountsListAdapter.BraveEthereumPermissionDelegate() {
                    @Override
                    public void onAccountCheckChanged(AccountInfo account, boolean isChecked) {
                        if (mPermissionDialogPositiveButton != null) {
                            mPermissionDialogPositiveButton.setEnabled(
                                    getSelectedAccounts().length > 0);
                        }
                    }
                });
        mRecyclerView.setAdapter(mAccountsListAdapter);
        LinearLayoutManager layoutManager = new LinearLayoutManager(mContext);
        mRecyclerView.setLayoutManager(layoutManager);
        mWalletModel.getDappsModel().fetchAccountsForConnectionReq(
                mCoinType, selectedAccountAllAccounts -> {
                    String selectedAccount = selectedAccountAllAccounts.first;
                    List<AccountInfo> accounts = selectedAccountAllAccounts.second;
                    mAccountsListAdapter.setAccounts(accounts.toArray(new AccountInfo[0]));
                    if (accounts.size() > 0) {
                        mAccountsListAdapter.setSelectedAccount(selectedAccount);
                        mPermissionDialogPositiveButton.setEnabled(true);
                    }
                    mAccountsListAdapter.notifyDataSetChanged();
                });
    }

    private void setFavIcon() {
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
        if (bestSize.width() == 0 || bestSize.height() == 0) {
            return;
        }

        mFavIconImage.setImageBitmap(bestBitmap);
        mFavIconImage.setVisibility(View.VISIBLE);
    }

    public String[] getSelectedAccounts() {
        assert mAccountsListAdapter != null;
        AccountInfo[] accountInfo = mAccountsListAdapter.getCheckedAccounts();
        String[] accounts = new String[accountInfo.length];
        for (int i = 0; i < accountInfo.length; i++) {
            accounts[i] = accountInfo[i].address;
        }

        return accounts;
    }

    @Override
    public void onClick(PropertyModel model, @ButtonType int buttonType) {
        if (buttonType == ButtonType.POSITIVE) {
            BraveEthereumPermissionPromptDialogJni.get().onPrimaryButtonClicked(
                    mNativeDialogController, getSelectedAccounts());
            mModalDialogManager.dismissDialog(
                    mPropertyModel, DialogDismissalCause.POSITIVE_BUTTON_CLICKED);
        } else if (buttonType == ButtonType.NEGATIVE) {
            BraveEthereumPermissionPromptDialogJni.get().onNegativeButtonClicked(
                    mNativeDialogController);
            mModalDialogManager.dismissDialog(
                    mPropertyModel, DialogDismissalCause.NEGATIVE_BUTTON_CLICKED);
        }
    }

    @Override
    public void onDismiss(PropertyModel model, int dismissalCause) {
        DisconnectMojoServices();
        BraveEthereumPermissionPromptDialogJni.get().onDialogDismissed(mNativeDialogController);
        mNativeDialogController = 0;
    }

    @CalledByNative
    private void dismissDialog() {
        mModalDialogManager.dismissDialog(mPropertyModel, DialogDismissalCause.DISMISSED_BY_NATIVE);
    }

    public void DisconnectMojoServices() {
        mMojoServicesClosed = true;
        if (mKeyringService != null) {
            mKeyringService.close();
            mKeyringService = null;
        }
        if (mBraveWalletService != null) {
            mBraveWalletService.close();
            mBraveWalletService = null;
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
        if (mMojoServicesClosed || mKeyringService == null) {
            return;
        }
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

    @NativeMethods
    interface Natives {
        void onPrimaryButtonClicked(
                long nativeBraveEthereumPermissionPromptDialogController, String[] accounts);
        void onNegativeButtonClicked(long nativeBraveEthereumPermissionPromptDialogController);
        void onDialogDismissed(long nativeBraveEthereumPermissionPromptDialogController);
    }
}

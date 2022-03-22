/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.crypto_wallet.modal;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.PopupWindow;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.SysUtils;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.HashSet;

public class BraveWalletPanel implements ConnectionErrorHandler, DialogInterface {
    private static final String TAG = "BraveWalletPanel";
    private final View mAnchorViewHost;
    private final PopupWindow mPopupWindow;
    private ViewGroup mPopupView;
    private final AppCompatActivity mActivity;
    private OnDismissListener mOnDismissListener;
    private Button mBtnConnectedStatus;
    private Button mBtnSelectedNetwork;
    private AccountInfo[] mAccountInfos;
    private BraveWalletService mBraveWalletService;
    protected JsonRpcService mJsonRpcService;

    public BraveWalletPanel(View anchorViewHost, OnDismissListener onDismissListener,
            JsonRpcService jsonRpcService, AccountInfo[] accountInfos) {
        mAnchorViewHost = anchorViewHost;
        mOnDismissListener = onDismissListener;
        mActivity = BraveActivity.getChromeTabbedActivity();
        mJsonRpcService = jsonRpcService;
        mAccountInfos = accountInfos;

        mPopupWindow = new PopupWindow(mAnchorViewHost.getContext());
        mPopupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mPopupWindow.setElevation(20);

        mPopupWindow.setTouchInterceptor(new View.OnTouchListener() {
            @SuppressLint("ClickableViewAccessibility")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                    dismiss();
                    return true;
                }
                return false;
            }
        });
        mPopupWindow.setOnDismissListener(new PopupWindow.OnDismissListener() {
            @Override
            public void onDismiss() {
                dismiss();
            }
        });
        setUpViews();
    }

    public void showLikePopDownMenu() {
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setOutsideTouchable(true);

        mPopupWindow.setAnimationStyle(R.style.EndIconMenuAnim);

        if (SysUtils.isLowEndDevice()) {
            mPopupWindow.setAnimationStyle(0);
        }

        mPopupWindow.showAsDropDown(mAnchorViewHost, 0, 0);
    }

    @Override
    public void cancel() {
        dismiss();
    }

    @Override
    public void dismiss() {
        mPopupWindow.dismiss();
        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss(this);
        }
        if (mBraveWalletService != null) {
            mBraveWalletService.close();
            mBraveWalletService = null;
        }
    }

    public void resume() {
        if (isShowing()) {
            updateState();
        }
    }

    public boolean isShowing() {
        return mPopupWindow != null && mPopupWindow.isShowing();
    }

    private void updateState() {
        assert mJsonRpcService != null;
        mJsonRpcService.getChainId(CoinType.ETH, chainId -> {
            mJsonRpcService.getAllNetworks(CoinType.ETH, chains -> {
                NetworkInfo[] customNetworks = Utils.getCustomNetworks(chains);
                String strNetwork = Utils.getNetworkShortText(mActivity, chainId).toString();
                mBtnSelectedNetwork.setText(strNetwork);
            });
        });
    }

    private void updateStatus() {
        assert mBraveWalletService != null;
        AccountsPermissionsHelper accountsPermissionsHelper = new AccountsPermissionsHelper(
                mBraveWalletService, mAccountInfos, getCurrentHostHttpAddress());
        accountsPermissionsHelper.checkAccounts(() -> {
            HashSet<AccountInfo> mAccountsWithPermissions =
                    accountsPermissionsHelper.getAccountsWithPermissions();
            if (mAccountsWithPermissions.size() != 0) {
                mBtnConnectedStatus.setText(mPopupView.getResources().getString(
                        R.string.dapp_wallet_panel_connectivity_status_connected));
                mBtnConnectedStatus.setCompoundDrawablesRelativeWithIntrinsicBounds(
                        R.drawable.ic_check_white, 0, 0, 0);
            }
        });
    }

    private void setUpViews() {
        InitBraveWalletService();
        LayoutInflater inflater = LayoutInflater.from(mAnchorViewHost.getContext());
        mPopupView = (ViewGroup) inflater.inflate(R.layout.brave_wallet_panel_layout, null);

        int deviceWidth = ConfigurationUtils.getDisplayMetrics(mActivity).get("width");
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity);

        mPopupWindow.setWidth((int) (isTablet ? (deviceWidth * 0.6) : (deviceWidth * 0.95)));

        mBtnSelectedNetwork = mPopupView.findViewById(R.id.btn_dapps_panel_networks);
        mBtnSelectedNetwork.setOnClickListener(v -> {
            Intent intent = new Intent(mActivity, NetworkSelectorActivity.class);
            mActivity.startActivity(intent);
        });
        mBtnConnectedStatus = mPopupView.findViewById(R.id.sp_dapps_panel_state);
        mPopupWindow.setContentView(mPopupView);
        updateState();
        updateStatus();
        // TODO: show connected or disconnected account page
    }

    private String getCurrentHostHttpAddress() {
        ChromeTabbedActivity activity = BraveActivity.getChromeTabbedActivity();
        if (activity != null) {
            return activity.getActivityTab().getUrl().getOrigin().getSpec();
        }
        return "";
    }

    @Override
    public void onConnectionError(MojoException e) {
        mBraveWalletService.close();
        mBraveWalletService = null;
        InitBraveWalletService();
    }

    private void InitBraveWalletService() {
        if (mBraveWalletService != null) {
            return;
        }

        mBraveWalletService = BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
    }
}

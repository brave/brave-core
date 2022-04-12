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
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import org.chromium.base.SysUtils;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.SingleTokenBalanceHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.url.GURL;
import org.chromium.url.internal.mojom.Origin;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BraveWalletPanel implements DialogInterface {
    private static final String TAG = "BraveWalletPanel";
    private final View mAnchorViewHost;
    private final PopupWindow mPopupWindow;
    private final AppCompatActivity mActivity;
    private ViewGroup mPopupView;
    private OnDismissListener mOnDismissListener;
    private Button mBtnConnectedStatus;
    private Button mBtnSelectedNetwork;
    private ImageView mAccountImage;
    private TextView mAccountName;
    private TextView mAccountAddress;
    private TextView mAmountAsset;
    private TextView mAmountFiat;
    private AccountInfo[] mAccountInfos;
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private BraveWalletPanelServices mBraveWalletPanelServices;

    public interface BraveWalletPanelServices {
        AssetRatioService getAssetRatioService();
        BraveWalletService getBraveWalletService();
        KeyringService getKeyringService();
        JsonRpcService getJsonRpcService();
    }

    public BraveWalletPanel(View anchorViewHost, OnDismissListener onDismissListener,
            BraveWalletPanelServices braveWalletPanelServices, AccountInfo[] accountInfos) {
        mAccountsWithPermissions = new HashSet<AccountInfo>();
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mAnchorViewHost = anchorViewHost;
        mOnDismissListener = onDismissListener;
        mActivity = BraveActivity.getChromeTabbedActivity();
        mAccountInfos = accountInfos;
        mBraveWalletPanelServices = braveWalletPanelServices;

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
        mBraveWalletPanelServices.getJsonRpcService().getChainId(CoinType.ETH, chainId -> {
            mBraveWalletPanelServices.getJsonRpcService().getAllNetworks(CoinType.ETH, chains -> {
                NetworkInfo[] customNetworks = Utils.getCustomNetworks(chains);
                String strNetwork = Utils.getNetworkShortText(mActivity, chainId).toString();
                mBtnSelectedNetwork.setText(strNetwork);
            });
        });
    }

    private void updateStatus() {
        GURL address = getCurrentHostHttpAddress();
        org.chromium.url.internal.mojom.Origin hostOrigin =
                new org.chromium.url.internal.mojom.Origin();
        hostOrigin.scheme = address.getScheme();
        hostOrigin.host = address.getHost();
        hostOrigin.port = 0;
        if (address.getPort().isEmpty()) {
            if (hostOrigin.scheme.equals("http")) {
                hostOrigin.port = 80;
            } else if (hostOrigin.scheme.equals("https")) {
                hostOrigin.port = 443;
            }
        } else {
            try {
                hostOrigin.port = Short.parseShort(address.getPort());
            } catch (Exception e) {
            }
        }

        AccountsPermissionsHelper accountsPermissionsHelper =
                new AccountsPermissionsHelper(mBraveWalletPanelServices.getBraveWalletService(),
                        mAccountInfos, hostOrigin);
        accountsPermissionsHelper.checkAccounts(() -> {
            mAccountsWithPermissions = accountsPermissionsHelper.getAccountsWithPermissions();
            updateAccount();
        });
    }

    private void updateAccount() {
        mBraveWalletPanelServices.getKeyringService().getSelectedAccount(CoinType.ETH, address -> {
            String selectedAccount = "";
            if (address != null && !address.isEmpty()) {
                selectedAccount = address;
                updateAccountInfo(selectedAccount);
            } else {
                if (!mAccountsWithPermissions.isEmpty()) {
                    selectedAccount = mAccountsWithPermissions.iterator().next().address;
                } else if (mAccountInfos.length > 0) {
                    selectedAccount = mAccountInfos[0].address;
                }
                setSelectedAccount(selectedAccount);
            }
        });
    }

    private void setSelectedAccount(String selectedAccount) {
        mBraveWalletPanelServices.getKeyringService().setSelectedAccount(
                selectedAccount, CoinType.ETH, success -> {
                    if (success) {
                        updateAccountInfo(selectedAccount);
                    }
                });
    }

    private void updateAccountInfo(String selectedAccount) {
        Utils.setBlockiesBitmapResource(mExecutor, mHandler, mAccountImage, selectedAccount, true);
        for (AccountInfo accountInfo : mAccountInfos) {
            if (accountInfo.address.equals(selectedAccount)) {
                mAccountName.setText(accountInfo.name);
                mAccountAddress.setText(Utils.stripAccountAddress(accountInfo.address));
                getBalance(new AccountInfo[] {accountInfo});
                updateAccountConnected(selectedAccount);
                break;
            }
        }
    }

    private void updateAccountConnected(String selectedAccount) {
        Iterator<AccountInfo> it = mAccountsWithPermissions.iterator();
        boolean found = false;
        while (it.hasNext()) {
            if (it.next().address.equals(selectedAccount)) {
                found = true;
                break;
            }
        }
        mBtnConnectedStatus.setText(mPopupView.getResources().getString(found
                        ? R.string.dapp_wallet_panel_connectivity_status_connected
                        : R.string.dapp_wallet_panel_connectivity_status));
        mBtnConnectedStatus.setCompoundDrawablesRelativeWithIntrinsicBounds(
                found ? R.drawable.ic_check_white : 0, 0, 0, 0);
    }

    private void getBalance(AccountInfo[] selectedAccount) {
        if (selectedAccount.length == 0) {
            return;
        }
        mBraveWalletPanelServices.getJsonRpcService().getChainId(CoinType.ETH, chainId -> {
            SingleTokenBalanceHelper singleTokenBalanceHelper =
                    new SingleTokenBalanceHelper(mBraveWalletPanelServices.getAssetRatioService(),
                            mBraveWalletPanelServices.getJsonRpcService());
            singleTokenBalanceHelper.getPerAccountBalances(
                    chainId, "", "eth", 18, selectedAccount, () -> {
                        Double thisAccountFiatBalance = Utils.getOrDefault(
                                singleTokenBalanceHelper.getPerAccountFiatBalance(),
                                selectedAccount[0].address, 0.0d);
                        String fiatBalanceString = String.format(
                                Locale.getDefault(), "$%,.2f", thisAccountFiatBalance);

                        Double thisAccountCryptoBalance = Utils.getOrDefault(
                                singleTokenBalanceHelper.getPerAccountCryptoBalance(),
                                selectedAccount[0].address, 0.0d);
                        String cryptoBalanceString = String.format(
                                Locale.getDefault(), "%.4f %s", thisAccountCryptoBalance, "ETH");
                        mAmountAsset.setText(cryptoBalanceString);
                        mAmountFiat.setText(fiatBalanceString);
                    });
        });
    }

    private void setUpViews() {
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
        mAccountImage = mPopupView.findViewById(R.id.iv_dapps_panel_account_image);
        mAccountName = mPopupView.findViewById(R.id.tv_dapps_panel_from_to);
        mAccountAddress = mPopupView.findViewById(R.id.tv_dapps_panel_account_address);
        mAmountAsset = mPopupView.findViewById(R.id.tv_dapps_panel_amount_asset);
        mAmountFiat = mPopupView.findViewById(R.id.tv_dapps_panel_amount_fiat);
        updateState();
        updateStatus();
        // TODO: show connected or disconnected account page
    }

    private GURL getCurrentHostHttpAddress() {
        ChromeTabbedActivity activity = BraveActivity.getChromeTabbedActivity();
        if (activity != null) {
            return activity.getActivityTab().getUrl().getOrigin();
        }
        return new GURL();
    }
}

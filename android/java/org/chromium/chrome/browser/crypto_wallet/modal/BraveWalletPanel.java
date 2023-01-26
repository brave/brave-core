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
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.StringRes;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.view.menu.MenuBuilder;
import androidx.lifecycle.Observer;

import org.chromium.base.SysUtils;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletProviderDelegateImplHelper;
import org.chromium.chrome.browser.crypto_wallet.activities.AccountSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AssetsPricesHelper;
import org.chromium.chrome.browser.crypto_wallet.util.BalanceHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;

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
    private ImageView mExpandWalletImage;
    private ImageView mOptionsImage;
    private Button mBtnConnectedStatus;
    private Button mBtnSelectedNetwork;
    private ImageView mAccountImage;
    private TextView mAccountName;
    private TextView mAccountAddress;
    private TextView mAmountAsset;
    private TextView mAmountFiat;
    private View mCvSolConnectionStatus;
    private AccountInfo[] mAccountInfos;
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private BraveWalletPanelServices mBraveWalletPanelServices;
    private ImageView mAccountChangeAnchor;
    private View mContainerConstraintLayout;
    private WalletModel mWalletModel;
    private AccountInfo mSelectedAccount;
    private NetworkInfo mSelectedNetwork;
    private final Observer<AccountInfo> mAccountInfoObserver = accountInfo -> {
        if (accountInfo == null) return;
        mSelectedAccount = accountInfo;
        mBraveWalletPanelServices.getKeyringService().getKeyringInfo(
                AssetUtils.getKeyringForCoinType(mSelectedAccount.coin), keyringInfo -> {
                    if (keyringInfo != null) {
                        mAccountInfos = keyringInfo.accountInfos;
                    }
                    AccountsPermissionsHelper accountsPermissionsHelper =
                            new AccountsPermissionsHelper(
                                    mBraveWalletPanelServices.getBraveWalletService(),
                                    mAccountInfos, Utils.getCurrentMojomOrigin());
                    accountsPermissionsHelper.checkAccounts(() -> {
                        mAccountsWithPermissions =
                                accountsPermissionsHelper.getAccountsWithPermissions();
                        updateAccountInfo(mSelectedAccount);
                    });
                });
    };

    private final Observer<NetworkInfo> mDefaultNetworkObserver = networkInfo -> {
        mSelectedNetwork = networkInfo;
        mBtnSelectedNetwork.setText(Utils.getShortNameOfNetwork(networkInfo.chainName));
    };

    public interface BraveWalletPanelServices {
        AssetRatioService getAssetRatioService();
        BraveWalletService getBraveWalletService();
        KeyringService getKeyringService();
        JsonRpcService getJsonRpcService();
    }

    public BraveWalletPanel(View anchorViewHost, OnDismissListener onDismissListener,
            BraveWalletPanelServices braveWalletPanelServices) {
        mAccountsWithPermissions = new HashSet<AccountInfo>();
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mAnchorViewHost = anchorViewHost;
        mOnDismissListener = onDismissListener;
        mActivity = BraveActivity.getChromeTabbedActivity();
        mAccountInfos = new AccountInfo[0];
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
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
        }
        setUpViews();
    }

    private void showPopupMenu() {
        // On Android 6 and 7 androidx.appcompat.widget.PopupMenu crashes with an exception
        // `android.view.WindowManager$BadTokenException: Unable to add window --
        // token android.view.ViewRootImpl$W@f1adfa6 is not valid; is your activity running?`
        // The same exception appears if we try anchor to a panel's view. That's why we
        // use android.widget.PopupMenu and anchor to an URL bar there.
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.N_MR1) {
            androidx.appcompat.widget.PopupMenu menu = new androidx.appcompat.widget.PopupMenu(
                    mOptionsImage.getContext(), (View) mOptionsImage);
            menu.getMenuInflater().inflate(R.menu.menu_dapps_panel, menu.getMenu());
            menu.setOnMenuItemClickListener(item -> { return handleMenuItemClick(item); });

            if (menu.getMenu() instanceof MenuBuilder) {
                ((MenuBuilder) menu.getMenu()).setOptionalIconsVisible(true);
            }
            menu.show();
        } else {
            android.widget.PopupMenu menu = new android.widget.PopupMenu(
                    mAnchorViewHost.getContext(), (View) mAnchorViewHost);
            menu.getMenuInflater().inflate(R.menu.menu_dapps_panel, menu.getMenu());
            menu.setOnMenuItemClickListener(item -> { return handleMenuItemClick(item); });
            menu.show();
        }
    }

    private boolean handleMenuItemClick(MenuItem item) {
        if (item.getItemId() == R.id.action_lock_wallet) {
            mBraveWalletPanelServices.getKeyringService().lock();
            dismiss();
        } else if (item.getItemId() == R.id.action_connected_sites) {
            BraveActivity activity = BraveActivity.getBraveActivity();
            if (activity != null) {
                activity.openBraveConnectedSitesSettings();
                dismiss();
            }
        } else if (item.getItemId() == R.id.action_settings) {
            BraveActivity activity = BraveActivity.getBraveActivity();
            if (activity != null) {
                activity.openBraveWalletSettings();
                dismiss();
            }
        } else if (item.getItemId() == R.id.action_view_on_block_explorer) {
            BraveActivity activity = BraveActivity.getBraveActivity();
            if (activity != null) {
                activity.viewOnBlockExplorer(mSelectedAccount.address, mSelectedAccount.coin);
                dismiss();
            }
        }
        return true;
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
        cleanUpObservers();
        mPopupWindow.dismiss();
        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss(this);
        }
    }

    public void resume() {
        setUpObservers();
    }

    public void pause() {
        cleanUpObservers();
    }

    public boolean isShowing() {
        return mPopupWindow != null && mPopupWindow.isShowing();
    }

    private void setUpObservers() {
        cleanUpObservers();
        mWalletModel.getKeyringModel().getSelectedAccountOrAccountPerOrigin().observeForever(
                mAccountInfoObserver);
        mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.observeForever(
                mDefaultNetworkObserver);
    }

    private void cleanUpObservers() {
        mWalletModel.getKeyringModel().getSelectedAccountOrAccountPerOrigin().removeObserver(
                mAccountInfoObserver);
        mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.removeObserver(
                mDefaultNetworkObserver);
    }

    private void updateAccountInfo(AccountInfo selectedAccount) {
        Utils.setBlockiesBitmapResource(
                mExecutor, mHandler, mAccountImage, selectedAccount.address, true);
        Utils.setBlockiesBackground(
                mExecutor, mHandler, mContainerConstraintLayout, selectedAccount.address, true);
        mAccountName.setText(selectedAccount.name);
        mAccountAddress.setText(Utils.stripAccountAddress(selectedAccount.address));
        getBalance(selectedAccount);
        updateAccountConnected(selectedAccount);
    }

    private void updateAccountConnected(AccountInfo selectedAccount) {
        if (CoinType.SOL == selectedAccount.coin) {
            if (mAccountsWithPermissions.size() == 0) {
                mBtnConnectedStatus.setText("");
                mBtnConnectedStatus.setVisibility(View.GONE);
                mCvSolConnectionStatus.setVisibility(View.GONE);
            } else {
                BraveActivity activity = BraveActivity.getBraveActivity();
                if (activity != null && activity.getActivityTab() != null) {
                    BraveWalletProviderDelegateImplHelper.IsSolanaConnected(
                            activity.getActivityTab().getWebContents(), selectedAccount.address,
                            isConnected -> {
                                updateConnectedState(isConnected,
                                        isConnected
                                                ? R.string.dapp_wallet_panel_connectivity_status_connected
                                                : R.string.dapp_wallet_panel_disconnect_status,
                                        true);
                            });
                }
            }
        } else {
            Iterator<AccountInfo> it = mAccountsWithPermissions.iterator();
            boolean isConnected = false;
            while (it.hasNext()) {
                if (it.next().address.equals(selectedAccount.address)) {
                    isConnected = true;
                    break;
                }
            }
            updateConnectedState(isConnected,
                    isConnected ? R.string.dapp_wallet_panel_connectivity_status_connected
                                : R.string.dapp_wallet_panel_connectivity_status,
                    false);
        }
    }

    private void updateConnectedState(
            boolean isConnected, @StringRes int connectedText, boolean showSolStatus) {
        mBtnConnectedStatus.setVisibility(View.VISIBLE);
        mBtnConnectedStatus.setText(mPopupView.getResources().getString(connectedText));
        if (showSolStatus) {
            mCvSolConnectionStatus.setBackgroundResource(isConnected
                            ? R.drawable.rounded_dot_success_status
                            : R.drawable.rounded_dot_error_status);
            mCvSolConnectionStatus.setVisibility(View.VISIBLE);
            mBtnConnectedStatus.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, 0, 0);
        } else {
            mCvSolConnectionStatus.setVisibility(View.GONE);
            mBtnConnectedStatus.setCompoundDrawablesRelativeWithIntrinsicBounds(
                    isConnected ? R.drawable.ic_check_white : 0, 0, 0, 0);
        }
    }

    private void getBalance(AccountInfo selectedAccount) {
        if (selectedAccount == null) {
            return;
        }
        mBraveWalletPanelServices.getJsonRpcService().getNetwork(
                selectedAccount.coin, selectedNetwork -> {
                    BlockchainToken asset = Utils.makeNetworkAsset(selectedNetwork);
                    AssetsPricesHelper.fetchPrices(mBraveWalletPanelServices.getAssetRatioService(),
                            new BlockchainToken[] {asset}, assetPrices -> {
                                BalanceHelper.getNativeAssetsBalances(
                                        mBraveWalletPanelServices.getJsonRpcService(),
                                        selectedNetwork, new AccountInfo[] {selectedAccount},
                                        (coinType, nativeAssetsBalances) -> {
                                            double price = Utils.getOrDefault(assetPrices,
                                                    asset.symbol.toLowerCase(Locale.getDefault()),
                                                    0.0d);
                                            double balance =
                                                    Utils.getOrDefault(nativeAssetsBalances,
                                                            selectedAccount.address.toLowerCase(
                                                                    Locale.getDefault()),
                                                            0.0d);
                                            String fiatBalanceString = String.format(
                                                    Locale.getDefault(), "$%,.2f", balance * price);
                                            String cryptoBalanceString =
                                                    String.format(Locale.getDefault(), "%.4f %s",
                                                            balance, selectedNetwork.symbol);
                                            mAmountAsset.setText(cryptoBalanceString);
                                            mAmountFiat.setText(fiatBalanceString);
                                        });
                            });
                });
    }

    private void setUpViews() {
        LayoutInflater inflater = LayoutInflater.from(mAnchorViewHost.getContext());
        mPopupView = (ViewGroup) inflater.inflate(R.layout.brave_wallet_panel_layout, null);

        int deviceWidth = ConfigurationUtils.getDisplayMetrics(mActivity).get("width");
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity);

        mPopupWindow.setWidth((int) (isTablet ? (deviceWidth * 0.6) : (deviceWidth * 0.95)));

        mExpandWalletImage = mPopupView.findViewById(R.id.iv_dapp_panel_expand);
        mExpandWalletImage.setOnClickListener(v -> {
            dismiss();
            BraveActivity activity = BraveActivity.getBraveActivity();
            if (activity != null) {
                activity.openBraveWallet(false, false, false);
            }
        });
        mOptionsImage = mPopupView.findViewById(R.id.iv_dapp_panel_menu);
        mOptionsImage.setOnClickListener(v -> { showPopupMenu(); });

        mBtnSelectedNetwork = mPopupView.findViewById(R.id.btn_dapps_panel_networks);
        mBtnSelectedNetwork.setOnClickListener(v -> {
            Intent intent = new Intent(mActivity, NetworkSelectorActivity.class);
            mActivity.startActivity(intent);
        });
        mCvSolConnectionStatus = mPopupView.findViewById(R.id.v_dapps_panel_sol_connection_status);
        mBtnConnectedStatus = mPopupView.findViewById(R.id.sp_dapps_panel_state);
        mBtnConnectedStatus.setOnClickListener(onConnectedAccountClick);
        mCvSolConnectionStatus.setOnClickListener(onConnectedAccountClick);
        mPopupWindow.setContentView(mPopupView);
        mContainerConstraintLayout = mPopupView.findViewById(R.id.container_constraint_panel);
        mAccountImage = mPopupView.findViewById(R.id.iv_dapps_panel_account_image);
        mAccountName = mPopupView.findViewById(R.id.tv_dapps_panel_from_to);
        mAccountAddress = mPopupView.findViewById(R.id.tv_dapps_panel_account_address);
        mAmountAsset = mPopupView.findViewById(R.id.tv_dapps_panel_amount_asset);
        mAmountFiat = mPopupView.findViewById(R.id.tv_dapps_panel_amount_fiat);
        mAccountChangeAnchor = mPopupView.findViewById(R.id.iv_dapps_panel_down_arrow_anchor);
        mAccountChangeAnchor.setOnClickListener(v -> {
            BraveActivity activity = BraveActivity.getBraveActivity();
            if (activity != null) {
                activity.startActivity(new Intent(activity, AccountSelectorActivity.class));
            }
        });
        mBtnSelectedNetwork.setOnLongClickListener(v -> {
            NetworkInfo networkInfo =
                    mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
            if (networkInfo != null) {
                Toast.makeText(mActivity, networkInfo.chainName, Toast.LENGTH_SHORT).show();
            }
            return true;
        });
        setUpObservers();
    }

    private final View.OnClickListener onConnectedAccountClick = v -> {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.openBraveWalletDAppsActivity(
                    BraveWalletDAppsActivity.ActivityType.CONNECT_ACCOUNT);
        }
    };
}

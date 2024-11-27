/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.modal;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
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

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.view.menu.MenuBuilder;
import androidx.lifecycle.Observer;

import org.chromium.base.Log;
import org.chromium.base.SysUtils;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AllAccountsInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletProviderDelegateImplHelper;
import org.chromium.chrome.browser.crypto_wallet.activities.AccountSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.AssetsPricesHelper;
import org.chromium.chrome.browser.crypto_wallet.util.BalanceHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.components.embedder_support.util.BraveUrlConstants;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.url.GURL;

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
    private final OnDismissListener mOnDismissListener;
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
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private final ExecutorService mExecutor;
    private final Handler mHandler;
    private ImageView mAccountChangeAnchor;
    private View mContainerConstraintLayout;
    private WalletModel mWalletModel;
    private AllAccountsInfo mAllAccountsInfo;
    private AccountInfo mSelectedAccount;
    private NetworkInfo mSelectedNetwork;
    private final Context mContext;
    private final Observer<AllAccountsInfo> mAllAccountsInfoObserver;

    private final boolean mShowExpandButton;

    private final Observer<NetworkInfo> mDefaultNetworkObserver;

    @SuppressLint("ClickableViewAccessibility")
    public BraveWalletPanel(
            @NonNull final View anchorViewHost,
            @NonNull final OnDismissListener onDismissListener,
            final boolean showExpandButton) {
        try {
            mWalletModel = BraveActivity.getBraveActivity().getWalletModel();
            // Update network model to use network per origin
            getNetworkModel().updateMode(NetworkModel.Mode.PANEL_MODE);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "BraveWalletPanel Constructor", e);
        }

        mShowExpandButton = showExpandButton;
        mAccountsWithPermissions = new HashSet<>();
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mAnchorViewHost = anchorViewHost;
        mOnDismissListener = onDismissListener;
        mContext = mAnchorViewHost.getContext();
        mActivity = BraveActivity.getChromeTabbedActivity();

        mDefaultNetworkObserver =
                networkInfo -> {
                    mSelectedNetwork = networkInfo;
                    mBtnSelectedNetwork.setText(Utils.getShortNameOfNetwork(networkInfo.chainName));

                    BlockchainToken asset = Utils.makeNetworkAsset(mSelectedNetwork);
                    final AssetRatioService assetRatioService = mWalletModel.getAssetRatioService();
                    final JsonRpcService jsonRpcService = mWalletModel.getJsonRpcService();
                    if (assetRatioService == null || jsonRpcService == null) {
                        return;
                    }
                    AssetsPricesHelper.fetchPrices(
                            assetRatioService,
                            new BlockchainToken[] {asset},
                            assetPrices ->
                                    BalanceHelper.getNativeAssetsBalances(
                                            jsonRpcService,
                                            mSelectedNetwork,
                                            new AccountInfo[] {mSelectedAccount},
                                            (coinType, nativeAssetsBalances) -> {
                                                double price =
                                                        Utils.getOrDefault(
                                                                assetPrices,
                                                                asset.symbol.toLowerCase(
                                                                        Locale.getDefault()),
                                                                0.0d);
                                                double balance =
                                                        Utils.getOrDefault(
                                                                nativeAssetsBalances,
                                                                mSelectedAccount.address
                                                                        .toLowerCase(
                                                                                Locale
                                                                                        .getDefault()),
                                                                0.0d);
                                                String fiatBalanceString =
                                                        String.format(
                                                                Locale.getDefault(),
                                                                "$%,.2f",
                                                                balance * price);
                                                String cryptoBalanceString =
                                                        String.format(
                                                                Locale.getDefault(),
                                                                "%.4f %s",
                                                                balance,
                                                                mSelectedNetwork.symbol);
                                                mAmountAsset.setText(cryptoBalanceString);
                                                mAmountFiat.setText(fiatBalanceString);
                                            }));
                };

        mAllAccountsInfoObserver =
                allAccountsInfo -> {
                    mAllAccountsInfo = allAccountsInfo;
                    mSelectedAccount = mAllAccountsInfo.selectedAccount;

                    final BraveWalletService braveWalletService =
                            mWalletModel.getBraveWalletService();
                    if (braveWalletService == null) {
                        return;
                    }
                    AccountsPermissionsHelper accountsPermissionsHelper =
                            new AccountsPermissionsHelper(
                                    braveWalletService,
                                    Utils.filterAccountsByCoin(
                                                    mAllAccountsInfo.accounts,
                                                    mSelectedAccount.accountId.coin)
                                            .toArray(new AccountInfo[0]));
                    accountsPermissionsHelper.checkAccounts(
                            () -> {
                                mAccountsWithPermissions =
                                        accountsPermissionsHelper.getAccountsWithPermissions();
                                updateSelectedAccountInfo();
                            });
                };

        mPopupWindow = new PopupWindow(mContext);
        mPopupWindow.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mPopupWindow.setElevation(20);

        mPopupWindow.setTouchInterceptor(
                (v, event) -> {
                    if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                        dismiss();
                        return true;
                    }
                    return false;
                });
        mPopupWindow.setOnDismissListener(this::dismiss);
        setUpViews();
    }

    private void showPopupMenu() {
        // On Android 6 and 7 androidx.appcompat.widget.PopupMenu crashes with an exception
        // `android.view.WindowManager$BadTokenException: Unable to add window --
        // token android.view.ViewRootImpl$W@f1adfa6 is not valid; is your activity running?`
        // The same exception appears if we try anchor to a panel's view. That's why we
        // use android.widget.PopupMenu and anchor to an URL bar there.
        androidx.appcompat.widget.PopupMenu menu =
                new androidx.appcompat.widget.PopupMenu(mOptionsImage.getContext(), mOptionsImage);
        menu.getMenuInflater().inflate(R.menu.menu_dapps_panel, menu.getMenu());
        menu.setOnMenuItemClickListener(this::handleMenuItemClick);

        if (menu.getMenu() instanceof MenuBuilder) {
            ((MenuBuilder) menu.getMenu()).setOptionalIconsVisible(true);
        }
        menu.show();
    }

    private boolean handleMenuItemClick(MenuItem item) {
        if (item.getItemId() == R.id.action_lock_wallet) {
            final KeyringService keyringService = mWalletModel.getKeyringService();
            if (keyringService != null) {
                keyringService.lock();
            }
            dismiss();
        } else if (item.getItemId() == R.id.action_connected_sites) {
            try {
                BraveActivity activity = BraveActivity.getBraveActivity();
                activity.openBraveConnectedSitesSettings();
                dismiss();
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "handleMenuItemClick action_connected_sites " + e);
            }
        } else if (item.getItemId() == R.id.action_settings) {
            try {
                BraveActivity activity = BraveActivity.getBraveActivity();
                activity.openBraveWalletSettings();
                dismiss();
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "handleMenuItemClick action_settings " + e);
            }
        } else if (item.getItemId() == R.id.action_view_on_block_explorer) {
            try {
                BraveActivity activity = BraveActivity.getBraveActivity();
                // TODO(apaymyshev): address might be null for bitcoin?
                activity.viewOnBlockExplorer(
                        mSelectedAccount.address,
                        mSelectedAccount.accountId.coin,
                        mSelectedNetwork);
                dismiss();
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "handleMenuItemClick action_view_on_block_explorer " + e);
            }
        } else if (item.getItemId() == R.id.action_help_center) {
            WalletUtils.openWalletHelpCenter(mContext);
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
        mWalletModel.getKeyringModel().mAllAccountsInfo.observeForever(mAllAccountsInfoObserver);
        getNetworkModel().mDefaultNetwork.observeForever(mDefaultNetworkObserver);
    }

    private void cleanUpObservers() {
        mWalletModel.getKeyringModel().mAllAccountsInfo.removeObserver(mAllAccountsInfoObserver);
        getNetworkModel().mDefaultNetwork.removeObserver(mDefaultNetworkObserver);
    }

    private void updateSelectedAccountInfo() {
        Utils.setTextGeneratedBlockies(
                mExecutor, mHandler, mAccountImage, mSelectedAccount.address, true);
        Utils.setBlockiesBackground(
                mExecutor, mHandler, mContainerConstraintLayout, mSelectedAccount.address, true);
        mAccountName.setText(mSelectedAccount.name);
        mAccountAddress.setText(Utils.stripAccountAddress(mSelectedAccount.address));
        updateSelectedAccountConnectionState();
    }

    private void updateSelectedAccountConnectionState() {
        if (CoinType.SOL == mSelectedAccount.accountId.coin) {
            if (mAccountsWithPermissions.size() == 0) {
                mBtnConnectedStatus.setText("");
                mBtnConnectedStatus.setVisibility(View.GONE);
                mCvSolConnectionStatus.setVisibility(View.GONE);
            } else {
                updateSolanaConnected(mSelectedAccount);
            }
        } else if (CoinType.ETH == mSelectedAccount.accountId.coin) {
            Iterator<AccountInfo> it = mAccountsWithPermissions.iterator();
            boolean isConnected = false;
            while (it.hasNext()) {
                if (WalletUtils.accountIdsEqual(it.next(), mSelectedAccount)) {
                    isConnected = true;
                    break;
                }
            }
            updateConnectedState(
                    isConnected,
                    isConnected
                            ? R.string.dapp_wallet_panel_connectivity_status_connected
                            : R.string.dapp_wallet_panel_connectivity_status,
                    false);
        }
    }

    private void updateSolanaConnected(AccountInfo selectedAccount) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            if (activity.getActivityTab() != null) {
                BraveWalletProviderDelegateImplHelper.isSolanaConnected(
                        activity.getActivityTab().getWebContents(),
                        selectedAccount.address,
                        isConnected -> {
                            onIsSolanaConnected(isConnected);
                        });
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "updateSolanaConnected " + e);
        }
    }

    private void onIsSolanaConnected(Boolean isConnected) {
        @StringRes int connected = R.string.dapp_wallet_panel_connectivity_status_connected;
        @StringRes int disconnected = R.string.dapp_wallet_panel_disconnect_status;
        updateConnectedState(isConnected, isConnected ? connected : disconnected, true);
    }

    private void updateConnectedState(
            boolean isConnected, @StringRes int connectedText, boolean showSolStatus) {
        mBtnConnectedStatus.setVisibility(View.VISIBLE);
        mBtnConnectedStatus.setText(mPopupView.getResources().getString(connectedText));
        if (showSolStatus) {
            mCvSolConnectionStatus.setBackgroundResource(
                    isConnected
                            ? R.drawable.rounded_dot_success_status
                            : R.drawable.rounded_dot_error_status);
            mCvSolConnectionStatus.setVisibility(View.VISIBLE);
            mBtnConnectedStatus.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, 0, 0);
        } else {
            mCvSolConnectionStatus.setVisibility(View.GONE);
            try {
                BraveActivity activity = BraveActivity.getBraveActivity();
                mBtnConnectedStatus.setVisibility(View.GONE);
                if (activity.getActivityTab() != null) {
                    GURL lastCommittedUrl =
                            activity.getActivityTab().getWebContents().getLastCommittedUrl();
                    if (!lastCommittedUrl.getScheme().equals(BraveUrlConstants.BRAVE_SCHEME)
                            && !lastCommittedUrl.getScheme().equals(UrlConstants.CHROME_SCHEME)
                            && !lastCommittedUrl
                                    .getScheme()
                                    .equals(UrlConstants.CHROME_NATIVE_SCHEME)) {
                        mBtnConnectedStatus.setVisibility(View.VISIBLE);
                        mBtnConnectedStatus.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                isConnected ? R.drawable.ic_check_white : 0, 0, 0, 0);
                    }
                }
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "updateConnectedState " + e);
            }
        }
    }

    private void setUpViews() {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        mPopupView = (ViewGroup) inflater.inflate(R.layout.brave_wallet_panel_layout, null);

        int deviceWidth = ConfigurationUtils.getDisplayMetrics(mActivity).get("width");
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(mActivity);

        mPopupWindow.setWidth((int) (isTablet ? (deviceWidth * 0.6) : (deviceWidth * 0.95)));

        mExpandWalletImage = mPopupView.findViewById(R.id.iv_dapp_panel_expand);
        if (mShowExpandButton) {
            mExpandWalletImage.setVisibility(View.VISIBLE);
            mExpandWalletImage.setOnClickListener(
                    v -> {
                        dismiss();
                        try {
                            BraveActivity activity = BraveActivity.getBraveActivity();
                            activity.openBraveWallet(false, false, false);
                        } catch (BraveActivity.BraveActivityNotFoundException e) {
                            Log.e(TAG, "ExpandWalletImage", e);
                        }
                    });
        }
        mOptionsImage = mPopupView.findViewById(R.id.iv_dapp_panel_menu);
        mOptionsImage.setOnClickListener(v -> showPopupMenu());

        mBtnSelectedNetwork = mPopupView.findViewById(R.id.btn_dapps_panel_networks);
        mBtnSelectedNetwork.setOnClickListener(
                v -> {
                    Intent intent = new Intent(mActivity, NetworkSelectorActivity.class);
                    intent.setAction(Intent.ACTION_VIEW);
                    mActivity.startActivity(intent);
                });
        mCvSolConnectionStatus = mPopupView.findViewById(R.id.v_dapps_panel_sol_connection_status);
        mBtnConnectedStatus = mPopupView.findViewById(R.id.sp_dapps_panel_state);
        mBtnConnectedStatus.setOnClickListener(mOnConnectedAccountClick);
        mCvSolConnectionStatus.setOnClickListener(mOnConnectedAccountClick);
        mPopupWindow.setContentView(mPopupView);
        mContainerConstraintLayout = mPopupView.findViewById(R.id.container_constraint_panel);
        mAccountImage = mPopupView.findViewById(R.id.iv_dapps_panel_account_image);
        mAccountName = mPopupView.findViewById(R.id.tv_dapps_panel_from_to);
        mAccountAddress = mPopupView.findViewById(R.id.tv_dapps_panel_account_address);
        mAmountAsset = mPopupView.findViewById(R.id.tv_dapps_panel_amount_asset);
        mAmountFiat = mPopupView.findViewById(R.id.tv_dapps_panel_amount_fiat);
        mAccountChangeAnchor = mPopupView.findViewById(R.id.iv_dapps_panel_down_arrow_anchor);
        mAccountChangeAnchor.setOnClickListener(
                v -> {
                    try {
                        BraveActivity activity = BraveActivity.getBraveActivity();
                        Intent intent = new Intent(activity, AccountSelectorActivity.class);
                        intent.setAction(Intent.ACTION_VIEW);
                        activity.startActivity(intent);
                    } catch (BraveActivity.BraveActivityNotFoundException e) {
                        Log.e(TAG, "setUpViews AccountChangeAnchor click " + e);
                    }
                });
        mBtnSelectedNetwork.setOnLongClickListener(
                v -> {
                    NetworkInfo networkInfo = getNetworkModel().mDefaultNetwork.getValue();
                    if (networkInfo != null) {
                        Toast.makeText(mActivity, networkInfo.chainName, Toast.LENGTH_SHORT).show();
                    }
                    return true;
                });
        setUpObservers();
    }

    private final View.OnClickListener mOnConnectedAccountClick =
            v -> {
                try {
                    BraveActivity activity = BraveActivity.getBraveActivity();
                    activity.openBraveWalletDAppsActivity(
                            BraveWalletDAppsActivity.ActivityType.CONNECT_ACCOUNT);
                } catch (BraveActivity.BraveActivityNotFoundException e) {
                    Log.e(TAG, "ConnectedAccountClick click " + e);
                }
            };

    private NetworkModel getNetworkModel() {
        return mWalletModel.getCryptoModel().getNetworkModel();
    }
}

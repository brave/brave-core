/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.tabs.TabLayout;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.ApproveTxFragmentPageAdapter;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ApproveTxBottomSheetDialogFragment extends BottomSheetDialogFragment {
    public static final String TAG_FRAGMENT = ApproveTxBottomSheetDialogFragment.class.getName();

    private TransactionInfo mTxInfo;
    private String mAccountName;
    private boolean mRejected;
    private boolean mApproved;
    private double mTotalPrice;
    private ApprovedTxObserver mApprovedTxObserver;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private String mChainSymbol;
    private int mChainDecimals;

    public static ApproveTxBottomSheetDialogFragment newInstance(
            TransactionInfo txInfo, String accountName) {
        return new ApproveTxBottomSheetDialogFragment(txInfo, accountName);
    }

    ApproveTxBottomSheetDialogFragment(TransactionInfo txInfo, String accountName) {
        mTxInfo = txInfo;
        mAccountName = accountName;
        mRejected = false;
        mApproved = false;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mChainSymbol = "ETH";
        mChainDecimals = 18;
    }

    // TODO: This needs to be changed to something that broadcasts to all classes implementing
    // ApprovedTxObserver.
    public void setApprovedTxObserver(ApprovedTxObserver approvedTxObserver) {
        mApprovedTxObserver = approvedTxObserver;
    }

    private AssetRatioService getAssetRatioService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getAssetRatioService();
        }
        return null;
    }

    private TxService getTxService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getTxService();
        }
        return null;
    }

    private JsonRpcService getJsonRpcService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getJsonRpcService();
        }

        return null;
    }

    private BlockchainRegistry getBlockchainRegistry() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getBlockchainRegistry();
        }

        return null;
    }

    private BraveWalletService getBraveWalletService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getBraveWalletService();
        }
        return null;
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            ApproveTxBottomSheetDialogFragment fragment =
                    (ApproveTxBottomSheetDialogFragment) manager.findFragmentByTag(
                            ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("ApproveTxBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        if (mApprovedTxObserver != null) {
            if (mRejected || mApproved) {
                mApprovedTxObserver.OnTxApprovedRejected(mApproved, mAccountName, mTxInfo.id);
            } else {
                mApprovedTxObserver.OnTxPending(mAccountName, mTxInfo.id);
            }
        }
    }

    @Override
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);

        @SuppressLint("InflateParams")
        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.approve_tx_bottom_sheet, null);

        dialog.setContentView(view);
        ViewParent parent = view.getParent();
        ((View) parent).getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
        JsonRpcService jsonRpcService = getJsonRpcService();
        assert jsonRpcService != null;
        jsonRpcService.getChainId(CoinType.ETH, chainId -> {
            jsonRpcService.getAllNetworks(CoinType.ETH, chains -> {
                NetworkInfo[] customNetworks = Utils.getCustomNetworks(chains);
                TextView networkName = view.findViewById(R.id.network_name);
                networkName.setText(
                        Utils.getNetworkText(getActivity(), chainId, customNetworks).toString());
                String chainSymbol = "ETH";
                int chainDecimals = 18;
                for (NetworkInfo chain : chains) {
                    if (chainId.equals(chain.chainId)) {
                        if (Utils.isCustomNetwork(chainId)) {
                            chainSymbol = chain.symbol;
                            chainDecimals = chain.decimals;
                            break;
                        }
                    }
                }
                mChainSymbol = chainSymbol;
                mChainDecimals = chainDecimals;
                TextView txType = view.findViewById(R.id.tx_type);
                txType.setText(getResources().getString(R.string.send));
                if (mTxInfo.txType == TransactionType.ERC20_TRANSFER
                        || mTxInfo.txType == TransactionType.ERC20_APPROVE) {
                    BlockchainRegistry blockchainRegistry = getBlockchainRegistry();
                    assert blockchainRegistry != null;
                    TokenUtils.getAllTokensFiltered(
                            getBraveWalletService(), blockchainRegistry, chainId, tokens -> {
                                for (BlockchainToken token : tokens) {
                                    // Replace USDC and DAI contract addresses for Ropsten network
                                    token.contractAddress = Utils.getContractAddress(
                                            chainId, token.symbol, token.contractAddress);
                                    String symbol = token.symbol;
                                    int decimals = token.decimals;
                                    if (mTxInfo.txType == TransactionType.ERC20_APPROVE) {
                                        txType.setText(String.format(
                                                getResources().getString(R.string.activate_erc20),
                                                symbol));
                                        symbol = mChainSymbol;
                                        decimals = mChainDecimals;
                                    }
                                    if (token.contractAddress.toLowerCase(Locale.getDefault())
                                                    .equals(mTxInfo.txDataUnion.getEthTxData1559()
                                                                    .baseData.to.toLowerCase(
                                                                            Locale.getDefault()))) {
                                        fillAssetDependentControls(symbol, view, decimals);
                                        break;
                                    }
                                }
                            });
                } else {
                    if (mTxInfo.txDataUnion.getEthTxData1559()
                                    .baseData.to.toLowerCase(Locale.getDefault())
                                    .equals(Utils.SWAP_EXCHANGE_PROXY.toLowerCase(
                                            Locale.getDefault()))) {
                        txType.setText(getResources().getString(R.string.swap));
                    }
                    fillAssetDependentControls(mChainSymbol, view, mChainDecimals);
                }
            });
        });
        ImageView icon = (ImageView) view.findViewById(R.id.account_picture);
        Utils.setBlockiesBitmapResource(mExecutor, mHandler, icon, mTxInfo.fromAddress, true);

        Button reject = view.findViewById(R.id.reject);
        reject.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                rejectTransaction(true);
            }
        });

        Button approve = view.findViewById(R.id.approve);
        approve.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                approveTransaction();
            }
        });
    }

    private void fillAssetDependentControls(String asset, View view, int decimals) {
        String valueToConvert = mTxInfo.txDataUnion.getEthTxData1559().baseData.value;
        String to = mTxInfo.txDataUnion.getEthTxData1559().baseData.to;
        if (mTxInfo.txType == TransactionType.ERC20_TRANSFER && mTxInfo.txArgs.length > 1) {
            valueToConvert = mTxInfo.txArgs[1];
            to = mTxInfo.txArgs[0];
        }
        TextView fromTo = view.findViewById(R.id.from_to);
        fromTo.setText(String.format(getResources().getString(R.string.crypto_wallet_from_to),
                mAccountName, Utils.stripAccountAddress(to)));
        TextView amountAsset = view.findViewById(R.id.amount_asset);
        amountAsset.setText(
                String.format(getResources().getString(R.string.crypto_wallet_amount_asset),
                        String.format(Locale.getDefault(), "%.4f",
                                Utils.fromHexWei(valueToConvert, decimals)),
                        asset));
        AssetRatioService assetRatioService = getAssetRatioService();
        assert assetRatioService != null;
        String[] assets = {asset.toLowerCase(Locale.getDefault())};
        String[] toCurr = {"usd"};
        assetRatioService.getPrice(
                assets, toCurr, AssetPriceTimeframe.LIVE, (success, values) -> {
                    String valueFiat = "0";
                    if (values.length != 0) {
                        valueFiat = values[0].price;
                    }
                    String valueAsset = mTxInfo.txDataUnion.getEthTxData1559().baseData.value;
                    if (mTxInfo.txType == TransactionType.ERC20_TRANSFER
                            && mTxInfo.txArgs.length > 1) {
                        valueAsset = mTxInfo.txArgs[1];
                    }
                    double value = Utils.fromHexWei(valueAsset, decimals);
                    double price = Double.valueOf(valueFiat);
                    mTotalPrice = value * price;
                    TextView amountFiat = view.findViewById(R.id.amount_fiat);
                    amountFiat.setText(String.format(
                            getResources().getString(R.string.crypto_wallet_amount_fiat),
                            String.format(Locale.getDefault(), "%.2f", mTotalPrice)));
                    ViewPager viewPager = view.findViewById(R.id.navigation_view_pager);
                    ApproveTxFragmentPageAdapter adapter = new ApproveTxFragmentPageAdapter(
                            getChildFragmentManager(), mTxInfo, asset, decimals, mChainSymbol,
                            mChainDecimals, mTotalPrice, getActivity());
                    viewPager.setAdapter(adapter);
                    viewPager.setOffscreenPageLimit(adapter.getCount() - 1);
                    TabLayout tabLayout = view.findViewById(R.id.tabs);
                    tabLayout.setupWithViewPager(viewPager);
                });
    }

    private void rejectTransaction(boolean dismiss) {
        TxService txService = getTxService();
        if (txService == null) {
            return;
        }
        txService.rejectTransaction(CoinType.ETH, mTxInfo.id, success -> {
            assert success;
            if (!success || !dismiss) {
                return;
            }
            mRejected = true;
            dismiss();
        });
    }

    private void approveTransaction() {
        TxService txService = getTxService();
        if (txService == null) {
            return;
        }
        txService.approveTransaction(CoinType.ETH, mTxInfo.id, (success, error, errorMessage) -> {
            assert success;
            Utils.warnWhenError(ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT,
                    "approveTransaction", error.getProviderError(), errorMessage);
            if (!success) {
                return;
            }
            mApproved = true;
            dismiss();
        });
    }
}

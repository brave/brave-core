/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.fromHexWei;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.ERCTokenRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AsyncUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;

public class PortfolioFragment extends Fragment
        implements OnWalletListItemClick, AdapterView.OnItemSelectedListener, ApprovedTxObserver {
    private static String TAG = "PortfolioFragment";
    private Spinner mSpinner;
    private TextView mBalance;
    private HashMap<String, TransactionInfo[]> mPendingTxInfos;

    private String mFiatSumString;

    private int mPreviousCheckedRadioId;
    private int mCurrentTimeframeType;

    PortfolioHelper mPortfolioHelper;

    public static PortfolioFragment newInstance() {
        return new PortfolioFragment();
    }

    private EthJsonRpcController getEthJsonRpcController() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getEthJsonRpcController();
        }

        return null;
    }

    private EthTxController getEthTxController() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getEthTxController();
        }

        return null;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_portfolio, container, false);

        view.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                SmoothLineChartEquallySpaced chartES = view.findViewById(R.id.line_chart);
                if (chartES == null) {
                    return true;
                }
                if (event.getAction() == MotionEvent.ACTION_MOVE
                        || event.getAction() == MotionEvent.ACTION_DOWN) {
                    chartES.drawLine(event.getRawX(), mBalance);
                } else if (event.getAction() == MotionEvent.ACTION_UP
                        || event.getAction() == MotionEvent.ACTION_CANCEL) {
                    mBalance.setText(mFiatSumString);
                    mBalance.invalidate();
                    chartES.drawLine(-1, null);
                }

                return true;
            }
        });

        mSpinner = view.findViewById(R.id.spinner);
        // class PortfolioFragment in fact is used both for Portfolio and Apps
        // screens, see CryptoFragmentPageAdapter.getItem.
        // Without post task with delay it happens that when 2nd instance is
        // created, then 1st instance onItemSelected is triggered with
        // position=0. That makes onItemSelected to switch network, but the
        // actual displayed value of spinner's item on 1st instance remains
        // unchanged.
        PostTask.postDelayedTask(UiThreadTaskTraits.DEFAULT,
                () -> { mSpinner.setOnItemSelectedListener(this); }, 500);

        mBalance = view.findViewById(R.id.balance);
        mBalance.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                updatePortfolioGetPendingTx(false);
            }
        });

        // Creating adapter for spinner
        NetworkSpinnerAdapter dataAdapter = new NetworkSpinnerAdapter(getActivity(),
                Utils.getNetworksList(getActivity()), Utils.getNetworksAbbrevList(getActivity()));
        mSpinner.setAdapter(dataAdapter);
        updatePortfolioGetPendingTx(true);

        return view;
    }

    private void updateNetwork() {
        EthJsonRpcController ethJsonRpcController = getEthJsonRpcController();
        if (ethJsonRpcController != null) {
            ethJsonRpcController.getChainId(chain_id -> {
                String chainName = mSpinner.getSelectedItem().toString();
                String chainId = Utils.getNetworkConst(getActivity(), chainName);
                if (chainId.equals(chain_id)) {
                    return;
                }
                mSpinner.setSelection(getIndexOf(mSpinner, chain_id));
            });
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        updateNetwork();
    }

    private int getIndexOf(Spinner spinner, String chain_id) {
        String strNetwork = Utils.getNetworkText(getActivity(), chain_id).toString();
        for (int i = 0; i < spinner.getCount(); i++) {
            if (spinner.getItemAtPosition(i).toString().equalsIgnoreCase(strNetwork)) {
                return i;
            }
        }

        return 0;
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        String item = parent.getItemAtPosition(position).toString();
        EthJsonRpcController ethJsonRpcController = getEthJsonRpcController();
        if (ethJsonRpcController != null) {
            ethJsonRpcController.setNetwork(
                    Utils.getNetworkConst(getActivity(), item), (success) -> {
                        if (!success) {
                            Log.e(TAG, "Could not set network");
                            return;
                        }
                        updatePortfolioGetPendingTx(true);
                    });
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> arg0) {}

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        assert getActivity() != null;

        Button editVisibleAssets = view.findViewById(R.id.edit_visible_assets);
        editVisibleAssets.setOnClickListener(v -> {
            String chainName = mSpinner.getSelectedItem().toString();
            String chainId = Utils.getNetworkConst(getActivity(), chainName);

            EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                    EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                            WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST);

            bottomSheetDialogFragment.setChainId(chainId);
            bottomSheetDialogFragment.setDismissListener(
                    new EditVisibleAssetsBottomSheetDialogFragment.DismissListener() {
                        @Override
                        public void onDismiss(Boolean isAssetsListChanged) {
                            if (isAssetsListChanged != null && isAssetsListChanged) {
                                updatePortfolioGetPendingTx(false);
                            }
                        }
                    });

            bottomSheetDialogFragment.show(
                    getFragmentManager(), EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
        });

        RadioGroup radioGroup = view.findViewById(R.id.portfolio_duration_radio_group);
        mPreviousCheckedRadioId = radioGroup.getCheckedRadioButtonId();
        mCurrentTimeframeType = Utils.getTimeframeFromRadioButtonId(mPreviousCheckedRadioId);
        radioGroup.setOnCheckedChangeListener((group, checkedId) -> {
            int leftDot = R.drawable.ic_live_dot;
            ((RadioButton) view.findViewById(mPreviousCheckedRadioId))
                    .setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            RadioButton button = view.findViewById(checkedId);
            button.setCompoundDrawablesWithIntrinsicBounds(leftDot, 0, 0, 0);
            mCurrentTimeframeType = Utils.getTimeframeFromRadioButtonId(checkedId);
            mPreviousCheckedRadioId = checkedId;
            updatePortfolioGraph();
        });
    }

    private void setUpCoinList(ErcToken[] userAssets, HashMap<String, Double> perTokenCryptoSum,
            HashMap<String, Double> perTokenFiatSum) {
        View view = getView();
        assert view != null;

        RecyclerView rvCoins = view.findViewById(R.id.rvCoins);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        String tokensPath = ERCTokenRegistryFactory.getInstance().getTokensIconsLocation();
        for (ErcToken userAsset : userAssets) {
            String currentAssetSymbol = userAsset.symbol.toLowerCase(Locale.getDefault());
            Double fiatBalance = Utils.getOrDefault(perTokenFiatSum, currentAssetSymbol, 0.0d);
            String fiatBalanceString = String.format(Locale.getDefault(), "$%,.2f", fiatBalance);
            Double cryptoBalance = Utils.getOrDefault(perTokenCryptoSum, currentAssetSymbol, 0.0d);
            String cryptoBalanceString =
                    String.format(Locale.getDefault(), "%.4f %s", cryptoBalance, userAsset.symbol);

            WalletListItemModel walletListItemModel =
                    new WalletListItemModel(R.drawable.ic_eth, userAsset.name, userAsset.symbol,
                            // Amount in USD
                            fiatBalanceString,
                            // Amount in current crypto currency/token
                            cryptoBalanceString);
            walletListItemModel.setIconPath("file://" + tokensPath + "/" + userAsset.logo);
            walletListItemModelList.add(walletListItemModel);
        }

        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(PortfolioFragment.this);
        walletCoinAdapter.setWalletListItemType(Utils.ASSET_ITEM);
        rvCoins.setAdapter(walletCoinAdapter);
        rvCoins.setLayoutManager(new LinearLayoutManager(getActivity()));
    }

    @Override
    public void onAssetClick() {
        Utils.openAssetDetailsActivity(getActivity());
    }

    private AssetRatioController getAssetRatioController() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getAssetRatioController();
        }

        return null;
    }

    private KeyringController getKeyringController() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringController();
        }

        return null;
    }

    BraveWalletService getBraveWalletService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getBraveWalletService();
        } else {
            assert false;
        }

        return null;
    }

    private void AdjustTrendControls() {
        TextView trendTimeframe = getView().findViewById(R.id.trend_timeframe);
        TextView trendPercentage = getView().findViewById(R.id.trend_percentage);

        if (mPortfolioHelper.isFiatHistoryEmpty()) {
            trendTimeframe.setVisibility(View.GONE);
            trendPercentage.setVisibility(View.GONE);
        } else {
            trendTimeframe.setText(Utils.getTimeframeString(mCurrentTimeframeType));

            Double currentFiatSum = mPortfolioHelper.getTotalFiatSum();
            Double mostPreviousFiatSum = mPortfolioHelper.getMostPreviousFiatSum();

            Double percents = ((currentFiatSum - mostPreviousFiatSum) / mostPreviousFiatSum) * 100;
            trendPercentage.setText(
                    String.format(Locale.getDefault(), "%.2f%%", Math.abs(percents)));
            if (mostPreviousFiatSum > currentFiatSum) {
                trendPercentage.setTextColor(
                        getResources().getColor(R.color.wallet_negative_trend_color));
                trendPercentage.setCompoundDrawablesWithIntrinsicBounds(
                        R.drawable.ic_down_icon, 0, 0, 0);
            } else {
                trendPercentage.setTextColor(
                        getResources().getColor(R.color.wallet_positive_trend_color));
                trendPercentage.setCompoundDrawablesWithIntrinsicBounds(
                        R.drawable.ic_up_icon, 0, 0, 0);
            }

            trendTimeframe.setVisibility(View.VISIBLE);
            trendPercentage.setVisibility(View.VISIBLE);
        }
    }

    private void updatePortfolioGraph() {
        AssetPriceTimeframe.validate(mCurrentTimeframeType);

        if (mPortfolioHelper == null) {
            updatePortfolioGetPendingTx(false);
            return;
        }

        mPortfolioHelper.setFiatHistoryTimeframe(mCurrentTimeframeType);
        mPortfolioHelper.calculateFiatHistory(() -> {
            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
                SmoothLineChartEquallySpaced chartES = getView().findViewById(R.id.line_chart);
                chartES.setColors(new int[] {0xFFF73A1C, 0xFFBF14A2, 0xFF6F4CD2});
                chartES.setData(mPortfolioHelper.getFiatHistory());

                AdjustTrendControls();
            });
        });
    }

    private void updatePortfolioGetPendingTx(boolean getPendingTx) {
        KeyringController keyringController = getKeyringController();
        assert keyringController != null;
        keyringController.getDefaultKeyringInfo(keyringInfo -> {
            AccountInfo[] accountInfos = new AccountInfo[] {};
            if (keyringInfo != null) {
                accountInfos = keyringInfo.accountInfos;
            }

            if (mPortfolioHelper == null) {
                mPortfolioHelper = new PortfolioHelper(getBraveWalletService(),
                        getAssetRatioController(), getEthJsonRpcController(), accountInfos);
            }

            String chainName = mSpinner.getSelectedItem().toString();
            String chainId = Utils.getNetworkConst(getActivity(), chainName);
            mPortfolioHelper.setChainId(chainId);
            mPortfolioHelper.calculateBalances(() -> {
                final String fiatSumString = String.format(
                        Locale.getDefault(), "$%,.2f", mPortfolioHelper.getTotalFiatSum());
                PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
                    mFiatSumString = fiatSumString;
                    mBalance.setText(mFiatSumString);
                    mBalance.invalidate();

                    setUpCoinList(mPortfolioHelper.getUserAssets(),
                            mPortfolioHelper.getPerTokenCryptoSum(),
                            mPortfolioHelper.getPerTokenFiatSum());

                    updatePortfolioGraph();
                });
            });
            if (getPendingTx) {
                getPendingTx(accountInfos);
            }
        });
    }

    @Override
    public void OnTxApprovedRejected(boolean approved, String accountName, String txId) {
        assert mPendingTxInfos != null;
        if (mPendingTxInfos == null) {
            return;
        }
        TransactionInfo[] txInfos = mPendingTxInfos.get(accountName);
        if (txInfos == null) {
            return;
        }
        int index = -1;
        for (int i = 0; i < txInfos.length; i++) {
            if (txInfos[i].id.equals(txId)) {
                index = i;
                break;
            }
        }
        if (index != -1) {
            TransactionInfo[] copy = new TransactionInfo[txInfos.length - 1];
            // copy elements from original array from beginning till index into copy
            System.arraycopy(txInfos, 0, copy, 0, index);
            // copy elements from original array from index + 1 till end into copy
            System.arraycopy(txInfos, index + 1, copy, index, txInfos.length - index - 1);
            mPendingTxInfos.put(accountName, copy);
        }
        callAnotherApproveDialog();
    }

    private void callAnotherApproveDialog() {
        assert mPendingTxInfos != null;
        if (mPendingTxInfos == null) {
            return;
        }
        for (String key : mPendingTxInfos.keySet()) {
            TransactionInfo[] txInfos = mPendingTxInfos.get(key);
            if (txInfos.length == 0) {
                continue;
            }
            ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment =
                    ApproveTxBottomSheetDialogFragment.newInstance(txInfos[0], key);
            approveTxBottomSheetDialogFragment.setApprovedTxObserver(this);
            approveTxBottomSheetDialogFragment.show(
                    getFragmentManager(), ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
            break;
        }
    }

    private void getPendingTx(AccountInfo[] accountInfos) {
        PendingTxHelper pendingTxHelper = new PendingTxHelper(getEthTxController(), accountInfos);
        pendingTxHelper.fetchTransactions(() -> {
            mPendingTxInfos = pendingTxHelper.getTransactions();
            callAnotherApproveDialog();
        });
    }
}

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.method.LinkMovementMethod;
import android.text.style.BulletSpan;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.Toolbar;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.chromium.base.Callback;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AssetDetailActivity
        extends BraveWalletBaseActivity implements OnWalletListItemClick, ApprovedTxObserver {
    private SmoothLineChartEquallySpaced chartES;
    private int checkedTimeframeType;
    private String mAssetSymbol;
    private String mAssetName;
    private String mAssetId;
    private String mChainId;
    private String mContractAddress;
    private String mAssetLogo;
    private int mAssetDecimals;
    private int mCoinType;
    private BlockchainToken mAsset;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private AccountInfo[] accountInfos;
    private WalletCoinAdapter mWalletTxCoinAdapter;
    private Button mBtnBuy;
    private Button mBtnSwap;
    private Button mBtnBridgeToAurora;
    private boolean mHasNewTx;
    private boolean mNativeInitialized;
    private boolean mShouldShowDialog;
    private WalletModel mWalletModel;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_asset_detail);

        String chainId = "";
        if (getIntent() != null) {
            mChainId = getIntent().getStringExtra(Utils.CHAIN_ID);
            mAssetSymbol = getIntent().getStringExtra(Utils.ASSET_SYMBOL);
            mAssetName = getIntent().getStringExtra(Utils.ASSET_NAME);
            mAssetId = getIntent().getStringExtra(Utils.ASSET_ID);
            mContractAddress = getIntent().getStringExtra(Utils.ASSET_CONTRACT_ADDRESS);
            mAssetLogo = getIntent().getStringExtra(Utils.ASSET_LOGO);
            mAssetDecimals =
                    getIntent().getIntExtra(Utils.ASSET_DECIMALS, Utils.ETH_DEFAULT_DECIMALS);
            mCoinType = getIntent().getIntExtra(Utils.COIN_TYPE, CoinType.ETH);

            getBlockchainToken(() -> {});

            if (mAssetSymbol.equals("ETH")) {
                mAssetLogo = "eth.png";
            }
        }
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        TextView assetTitleText = findViewById(R.id.asset_title_text);
        assetTitleText.setText(mAssetName);
        if (!mAssetLogo.isEmpty()) {
            String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
            String iconPath =
                    mAssetLogo.isEmpty() ? null : ("file://" + tokensPath + "/" + mAssetLogo);
            Utils.setBitmapResource(mExecutor, mHandler, this, iconPath,
                    Utils.getCoinIcon(mCoinType), null, assetTitleText, false);
        } else {
            Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, null, mContractAddress,
                    mAssetSymbol, getResources().getDisplayMetrics().density, assetTitleText, this,
                    false, (float) 0.5);
        }

        TextView assetPriceText = findViewById(R.id.asset_price_text);
        assetPriceText.setText(String.format(
                getResources().getString(R.string.asset_price), mAssetName, mAssetSymbol));

        mBtnBuy = findViewById(R.id.btn_buy);
        mBtnBuy.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Utils.openBuySendSwapActivity(AssetDetailActivity.this,
                        BuySendSwapActivity.ActivityType.BUY, mAssetSymbol);
            }
        });
        Button btnSend = findViewById(R.id.btn_send);
        btnSend.setOnClickListener(v
                -> Utils.openBuySendSwapActivity(AssetDetailActivity.this,
                        BuySendSwapActivity.ActivityType.SEND, mAssetSymbol));

        mBtnSwap = findViewById(R.id.btn_swap);

        if (AssetUtils.isAuroraAddress(mContractAddress, mChainId)) {
            mBtnSwap.setVisibility(View.GONE);
            mBtnBridgeToAurora = findViewById(R.id.btn_aurora_bridge);
            mBtnBridgeToAurora.setVisibility(View.VISIBLE);
            SpannableString rainBowLearnMore = Utils.createSpanForSurroundedPhrase(
                    this, R.string.brave_wallet_rainbow_bridge_learn_more, (v) -> {
                        TabUtils.openLinkWithFocus(
                                this, WalletConstants.URL_RAINBOW_BRIDGE_OVERVIEW);
                    });
            rainBowLearnMore.setSpan(
                    new BulletSpan(
                            15, getResources().getColor(R.color.brave_wallet_day_night_text_color)),
                    0, rainBowLearnMore.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            SpannableString rainBowRisksLearnMore = Utils.createSpanForSurroundedPhrase(
                    this, R.string.brave_wallet_rainbow_bridge_risks_learn_more, (v) -> {
                        TabUtils.openLinkWithFocus(this, WalletConstants.URL_RAINBOW_BRIDGE_RISKS);
                    });
            rainBowRisksLearnMore.setSpan(
                    new BulletSpan(
                            15, getResources().getColor(R.color.brave_wallet_day_night_text_color)),
                    0, rainBowRisksLearnMore.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            SpannableStringBuilder spannableStringBuilder =
                    new SpannableStringBuilder(rainBowLearnMore);
            spannableStringBuilder.append(System.getProperty(WalletConstants.LINE_SEPARATOR));
            spannableStringBuilder.append(System.getProperty(WalletConstants.LINE_SEPARATOR));
            spannableStringBuilder.append(rainBowRisksLearnMore);

            MaterialAlertDialogBuilder auroraDialogBuilder =
                    new MaterialAlertDialogBuilder(this, R.style.BraveWalletAlertDialogTheme);

            View dialogView = getLayoutInflater().inflate(R.layout.dialog_aurora_bridge, null);
            auroraDialogBuilder.setView(dialogView);
            AlertDialog auroraDialog = auroraDialogBuilder.create();

            SharedPreferencesManager preferencesManager = SharedPreferencesManager.getInstance();
            mShouldShowDialog = preferencesManager.readBoolean(
                    WalletConstants.PREF_SHOW_BRIDGE_INFO_DIALOG, true);

            TextView message = dialogView.findViewById(R.id.dialog_aurora_desc);
            TextView title = dialogView.findViewById(R.id.dialog_aurora_tv_title);
            TextView learnMore = dialogView.findViewById(R.id.dialog_aurora_tv_learn_more);
            Button btnOpenRainBow = dialogView.findViewById(R.id.btn_open_rainbow_app);

            title.setText(getString(R.string.brave_wallet_aurora_modal_title,
                    getString(R.string.brave_wallet_rainbow_bridge)));
            learnMore.setMovementMethod(LinkMovementMethod.getInstance());
            learnMore.setText(spannableStringBuilder);
            btnOpenRainBow.setText(getString(R.string.brave_wallet_aurora_modal_open_button_text,
                    getString(R.string.brave_wallet_rainbow_bridge)));
            CheckBox checkBox = dialogView.findViewById(R.id.dialog_aurora_cb_dont_show);
            checkBox.setChecked(!mShouldShowDialog);
            checkBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
                preferencesManager.writeBoolean(
                        WalletConstants.PREF_SHOW_BRIDGE_INFO_DIALOG, !isChecked);
                mShouldShowDialog = !isChecked;
            });

            message.setMovementMethod(LinkMovementMethod.getInstance());
            message.setText(getString(R.string.brave_wallet_aurora_modal_description,
                    getString(R.string.brave_wallet_rainbow_bridge)));
            btnOpenRainBow.setOnClickListener(
                    v -> { TabUtils.openLinkWithFocus(this, WalletConstants.URL_RAINBOW_AURORA); });

            mBtnBridgeToAurora.setOnClickListener(v -> {
                if (mShouldShowDialog) {
                    auroraDialog.show();
                } else {
                    TabUtils.openLinkWithFocus(this, WalletConstants.URL_RAINBOW_AURORA);
                }
            });
        } else {
            mBtnSwap.setOnClickListener(v
                    -> Utils.openBuySendSwapActivity(AssetDetailActivity.this,
                            BuySendSwapActivity.ActivityType.SWAP, mAssetSymbol));
        }

        adjustButtonsVisibilities();

        RadioGroup radioGroup = findViewById(R.id.asset_duration_radio_group);
        checkedTimeframeType = radioGroup.getCheckedRadioButtonId();
        radioGroup.setOnCheckedChangeListener((group, checkedId) -> {
            ((RadioButton) findViewById(checkedTimeframeType))
                    .setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            RadioButton button = findViewById(checkedId);
            int timeframeType = Utils.getTimeframeFromRadioButtonId(checkedId);
            getPriceHistory(mAssetSymbol, "usd", timeframeType);
            checkedTimeframeType = checkedId;
        });

        final TextView assetPrice = findViewById(R.id.asset_price);
        chartES = findViewById(R.id.line_chart);
        chartES.setColors(new int[] {getResources().getColor(R.color.wallet_asset_graph_color)});
        chartES.drawLine(0, assetPrice);
        chartES.setNoDrawText(true);
        chartES.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                v.getParent().requestDisallowInterceptTouchEvent(true);
                SmoothLineChartEquallySpaced chartES = (SmoothLineChartEquallySpaced) v;
                if (chartES == null) {
                    return true;
                }
                if (event.getAction() == MotionEvent.ACTION_MOVE
                        || event.getAction() == MotionEvent.ACTION_DOWN) {
                    chartES.drawLine(event.getRawX(), assetPrice);
                } else if (event.getAction() == MotionEvent.ACTION_UP
                        || event.getAction() == MotionEvent.ACTION_CANCEL) {
                    chartES.drawLine(-1, null);
                }

                return true;
            }
        });

        onInitialLayoutInflationComplete();
    }

    @Override
    public void onStart() {
        super.onStart();
        if (mHasNewTx) {
            setUpAccountList();
            mHasNewTx = false;
        }
    }

    private void getPriceHistory(String asset, String vsAsset, int timeframe) {
        if (mAssetRatioService != null) {
            mAssetRatioService.getPriceHistory(asset, vsAsset, timeframe,
                    (result, priceHistory) -> { chartES.setData(priceHistory); });
        }
    }

    private void getPrice(String asset, String vsAsset, int timeframe) {
        assert mAssetRatioService != null;
        String[] fromAssets = new String[] {asset.toLowerCase(Locale.getDefault())};
        String[] toAssets = new String[] {vsAsset.toLowerCase(Locale.getDefault())};
        mAssetRatioService.getPrice(fromAssets, toAssets, timeframe, (success, price) -> {
            if (!success && price.length == 0) {
                return;
            }
            String btcPriceText = price[0].price + " BTC";
            TextView btcPrice = findViewById(R.id.asset_price_btc_text);
            btcPrice.setText(btcPriceText);
        });
    }

    private void setUpAccountList() {
        RecyclerView rvAccounts = findViewById(R.id.rv_accounts);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.ACCOUNTS_LIST);
        mWalletTxCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        KeyringService keyringService = getKeyringService();
        JsonRpcService jsonRpcService = getJsonRpcService();
        if (keyringService != null && jsonRpcService != null && mWalletModel != null) {
            keyringService.getKeyringInfo(
                    AssetUtils.getKeyringForCoinType(mCoinType), keyringInfo -> {
                        if (keyringInfo == null) return;
                        accountInfos = keyringInfo.accountInfos;
                        jsonRpcService.getNetwork(mCoinType, selectedNetwork -> {
                            WalletListItemModel thisAssetItemModel =
                                    new WalletListItemModel(R.drawable.ic_eth, mAsset.name,
                                            mAsset.symbol, mAsset.tokenId, "", "");
                            LiveDataUtil.observeOnce(
                                    mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks,
                                    allNetworks -> {
                                        Utils.getTxExtraInfo(this, allNetworks, selectedNetwork,
                                                accountInfos, new BlockchainToken[] {mAsset}, false,
                                                (assetPrices, fullTokenList, nativeAssetsBalances,
                                                        blockchainTokensBalances) -> {
                                                    thisAssetItemModel.setBlockchainToken(mAsset);
                                                    Utils.setUpTransactionList(this, accountInfos,
                                                            thisAssetItemModel, assetPrices,
                                                            fullTokenList, nativeAssetsBalances,
                                                            blockchainTokensBalances,
                                                            findViewById(R.id.rv_transactions),
                                                            this, mWalletTxCoinAdapter);

                                                    double thisPrice =
                                                            Utils.getOrDefault(assetPrices,
                                                                    mAsset.symbol.toLowerCase(
                                                                            Locale.getDefault()),
                                                                    0.0d);
                                                    List<WalletListItemModel>
                                                            walletListItemModelList =
                                                                    new ArrayList<>();
                                                    for (AccountInfo accountInfo : accountInfos) {
                                                        final String accountAddressLower =
                                                                accountInfo.address.toLowerCase(
                                                                        Locale.getDefault());
                                                        double thisAccountBalance =
                                                                Utils.isNativeToken(
                                                                        selectedNetwork, mAsset)
                                                                ? Utils.getOrDefault(
                                                                        nativeAssetsBalances,
                                                                        accountAddressLower, 0.0d)
                                                                : Utils.getOrDefault(
                                                                        Utils.getOrDefault(
                                                                                blockchainTokensBalances,
                                                                                accountAddressLower,
                                                                                new HashMap<String,
                                                                                        Double>()),
                                                                        Utils.tokenToString(mAsset),
                                                                        0.0d);
                                                        final String fiatBalanceString =
                                                                String.format(Locale.getDefault(),
                                                                        "$%,.2f",
                                                                        thisPrice
                                                                                * thisAccountBalance);
                                                        final String cryptoBalanceString =
                                                                String.format(Locale.getDefault(),
                                                                        "%.4f %s",
                                                                        thisAccountBalance,
                                                                        mAsset.symbol);

                                                        // if NFT, only show the account that owns
                                                        // it (i.e. balance = 1)
                                                        if (mAsset.isNft
                                                                && thisAccountBalance != 1.)
                                                            continue;

                                                        WalletListItemModel model =
                                                                new WalletListItemModel(
                                                                        R.drawable.ic_eth,
                                                                        accountInfo.name,
                                                                        accountInfo.address,
                                                                        fiatBalanceString,
                                                                        cryptoBalanceString,
                                                                        accountInfo.isImported);
                                                        model.setAccountInfo(accountInfo);
                                                        walletListItemModelList.add(model);
                                                    }

                                                    if (walletCoinAdapter != null) {
                                                        walletCoinAdapter
                                                                .setWalletListItemModelList(
                                                                        walletListItemModelList);
                                                        walletCoinAdapter.setOnWalletListItemClick(
                                                                AssetDetailActivity.this);
                                                        walletCoinAdapter.setWalletListItemType(
                                                                Utils.ACCOUNT_ITEM);
                                                        rvAccounts.setAdapter(walletCoinAdapter);
                                                        rvAccounts.setLayoutManager(
                                                                new LinearLayoutManager(
                                                                        AssetDetailActivity.this));
                                                    }
                                                });
                                    });
                        });
                    });
        }
    }

    // Get back token from native. If cannot find then something is wrong
    private void getBlockchainToken(Runnable callback) {
        if (mAsset != null || !mNativeInitialized) {
            callback.run();
            return;
        }

        getJsonRpcService().getNetwork(mCoinType, selectedNetwork -> {
            TokenUtils.getExactUserAsset(getBraveWalletService(), selectedNetwork,
                    selectedNetwork.coin, mAssetSymbol, mAssetName, mAssetId, mContractAddress,
                    mAssetDecimals, new Callback<BlockchainToken>() {
                        @Override
                        public void onResult(BlockchainToken token) {
                            assert token != null;
                            mAsset = token;
                            callback.run();
                        }
                    });
        });
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        mNativeInitialized = true;
        getPriceHistory(mAssetSymbol, "usd", AssetPriceTimeframe.ONE_DAY);
        getPrice(mAssetSymbol, "btc", AssetPriceTimeframe.LIVE);
        getBlockchainToken(() -> setUpAccountList());
    }

    @Override
    public void onAccountClick(WalletListItemModel walletListItemModel) {
        Intent accountDetailActivityIntent =
                new Intent(AssetDetailActivity.this, AccountDetailActivity.class);
        accountDetailActivityIntent.putExtra(Utils.NAME, walletListItemModel.getTitle());
        accountDetailActivityIntent.putExtra(Utils.ADDRESS, walletListItemModel.getSubTitle());
        accountDetailActivityIntent.putExtra(
                Utils.ISIMPORTED, walletListItemModel.getIsImportedAccount());
        if (walletListItemModel.getAccountInfo() != null) {
            accountDetailActivityIntent.putExtra(
                    Utils.COIN_TYPE, walletListItemModel.getAccountInfo().coin);
        }
        startActivityForResult(accountDetailActivityIntent, Utils.ACCOUNT_REQUEST_CODE);
    }

    @Override
    public void onTransactionClick(TransactionInfo txInfo) {
        Utils.openTransaction(txInfo, mJsonRpcService, this, accountInfos, mCoinType);
    }

    @Override
    public void onTxApprovedRejected(boolean approved, String accountName, String txId) {}

    @Override
    public void onTxPending(String accountName, String txId) {}

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {
        mWalletTxCoinAdapter.onTransactionUpdate(txInfo);
    }

    @Override
    public void onNewUnapprovedTx(TransactionInfo txInfo) {
        mHasNewTx = true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == Utils.ACCOUNT_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                setUpAccountList();
            }
        }
    }

    private void adjustButtonsVisibilities() {
        showHideBuyUi();
        if (Utils.allowSwap(mChainId)) {
            if (!AssetUtils.isAuroraAddress(mContractAddress, mChainId)) {
                mBtnSwap.setVisibility(View.VISIBLE);
            }
        } else {
            mBtnSwap.setVisibility(View.GONE);
        }
    }

    private void showHideBuyUi() {
        if (!Utils.allowBuy(mChainId)) {
            AndroidUtils.gone(mBtnBuy);
            return;
        }
        if (mWalletModel == null) return;

        LiveDataUtil.observeOnce(mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork,
                selectedNetwork -> {
                    mWalletModel.getCryptoModel().isBuySupported(selectedNetwork, mAssetSymbol,
                            mContractAddress, mChainId, isBuyEnabled -> {
                                if (isBuyEnabled) {
                                    AndroidUtils.show(mBtnBuy);
                                } else {
                                    AndroidUtils.gone(mBtnBuy);
                                }
                            });
                });
    }
}

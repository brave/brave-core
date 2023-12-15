/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Looper;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.method.LinkMovementMethod;
import android.text.style.BulletSpan;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.Toolbar;
import androidx.cardview.widget.CardView;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.chromium.base.ContextUtils;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.MarketModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.crypto_wallet.web_ui.WebUiActivityType;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AssetDetailActivity
        extends BraveWalletBaseActivity implements OnWalletListItemClick, ApprovedTxObserver {
    private static final int ASSET_LOGO_SIZE_DP = 23;

    private SmoothLineChartEquallySpaced chartES;
    private int checkedTimeframeType;
    private String mAssetSymbol;
    private String mAssetName;
    private String mAssetId;
    private String mChainId;
    private boolean mIsMarketCoin;
    private int mMarketCapRank;
    private double mVolume24Hour;
    private double mMarketCap;
    private String mContractAddress;
    private String mAssetLogo;
    private int mAssetDecimals;
    private int mCoinType;
    private BlockchainToken mAsset;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private AccountInfo[] mAccountInfos;
    private WalletCoinAdapter mWalletTxCoinAdapter;
    private Button mBtnBuy;
    private Button mBtnSwap;
    private Button mBtnBridgeToAurora;
    private boolean mHasNewTx;
    private boolean mNativeInitialized;
    private boolean mShouldShowDialog;
    private WalletModel mWalletModel;
    private MarketModel mMarketModel;
    private NetworkInfo mAssetNetwork;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_asset_detail);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mMarketModel = mWalletModel.getMarketModel();
            if (mWalletModel == null || mMarketModel == null) {
                finish();
                return;
            }
        } catch (BraveActivity.BraveActivityNotFoundException ignored) {
            finish();
            return;
        }

        final Intent intent = getIntent();
        if (intent != null) {
            mChainId = intent.getStringExtra(Utils.CHAIN_ID);
            mAssetSymbol = intent.getStringExtra(Utils.ASSET_SYMBOL);
            mAssetName = intent.getStringExtra(Utils.ASSET_NAME);
            mAssetId = intent.getStringExtra(Utils.ASSET_ID);
            mIsMarketCoin = intent.getBooleanExtra(Utils.IS_MARKET_COIN, false);
            mMarketCapRank = intent.getIntExtra(Utils.MARKET_CAP_RANK, -1);
            mVolume24Hour = intent.getDoubleExtra(Utils.TOTAL_VOLUME, -1);
            mMarketCap = intent.getDoubleExtra(Utils.MARKET_CAP, -1);
            mContractAddress = intent.getStringExtra(Utils.ASSET_CONTRACT_ADDRESS);
            mAssetLogo = intent.getStringExtra(Utils.ASSET_LOGO);
            mAssetDecimals = intent.getIntExtra(Utils.ASSET_DECIMALS, Utils.ETH_DEFAULT_DECIMALS);
            mCoinType = intent.getIntExtra(Utils.COIN_TYPE, CoinType.ETH);
            if (mAssetSymbol.equals("ETH")) {
                mAssetLogo = "eth.png";
            }
            mAssetNetwork = mWalletModel.getNetworkModel().getNetwork(mChainId);
            getBlockchainToken(() -> {});
        }
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        TextView assetTitleText = findViewById(R.id.asset_title_text);
        assetTitleText.setText(mAssetName);
        mBtnBuy = findViewById(R.id.btn_buy);
        Button btnSend = findViewById(R.id.btn_send);
        mBtnSwap = findViewById(R.id.btn_swap);

        if (mIsMarketCoin) {
            TextView informationLabel = findViewById(R.id.information);
            CardView marketCoinInfo = findViewById(R.id.card_view_market_coin_info);
            AndroidUtils.show(informationLabel, marketCoinInfo);

            if (mMarketCapRank != -1) {
                TextView rank = findViewById(R.id.rank);
                rank.setText(String.valueOf(mMarketCapRank));
            }

            if (mVolume24Hour != -1) {
                TextView volume = findViewById(R.id.volume);
                volume.setText(mMarketModel.getFormattedUsdBillions(mVolume24Hour));
            }

            if (mMarketCap != -1) {
                TextView marketCap = findViewById(R.id.market_cap);
                marketCap.setText(mMarketModel.getFormattedUsdBillions(mMarketCap));
            }

            DisplayMetrics displayMetrics =
                    ContextUtils.getApplicationContext().getResources().getDisplayMetrics();
            final int sizePx = dpToPx(displayMetrics, ASSET_LOGO_SIZE_DP);
            ImageLoader.downloadImage(
                    mAssetLogo, Glide.with(this), true, 0, new CustomTarget<>(sizePx, sizePx) {
                        @Override
                        public void onResourceReady(@NonNull Drawable resource,
                                @Nullable Transition<? super Drawable> transition) {
                            assetTitleText.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                    resource, null, null, null);
                        }

                        @Override
                        public void onLoadCleared(@Nullable Drawable placeholder) {
                            /* No-op. */
                        }
                    }, null);
            TextView accountsLabel = findViewById(R.id.accounts);
            TextView transactionsLabel = findViewById(R.id.transactions);
            AndroidUtils.gone(accountsLabel, transactionsLabel, mBtnBuy, btnSend, mBtnSwap);
        } else if (!mAssetLogo.isEmpty()) {
            String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
            String iconPath =
                    mAssetLogo.isEmpty() ? null : ("file://" + tokensPath + "/" + mAssetLogo);
            Utils.setBitmapResource(mExecutor, mHandler, this, iconPath,
                    Utils.getCoinIcon(mCoinType), null, assetTitleText, false);
        } else {
            Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, null, mContractAddress,
                    mAssetSymbol, getResources().getDisplayMetrics().density, assetTitleText, this,
                    false, (float) 0.5, true);
        }

        TextView assetPriceText = findViewById(R.id.asset_price_text);
        assetPriceText.setText(String.format(
                getResources().getString(R.string.asset_price), mAssetName, mAssetSymbol));

        if (!mIsMarketCoin && AssetUtils.isAuroraAddress(mContractAddress, mChainId)) {
            mBtnSwap.setVisibility(View.GONE);
            mBtnBridgeToAurora = findViewById(R.id.btn_aurora_bridge);
            mBtnBridgeToAurora.setVisibility(View.VISIBLE);
            SpannableString rainBowLearnMore =
                    Utils.createSpanForSurroundedPhrase(
                            this,
                            R.string.brave_wallet_rainbow_bridge_learn_more,
                            (v) ->
                                    TabUtils.openLinkWithFocus(
                                            this, WalletConstants.URL_RAINBOW_BRIDGE_OVERVIEW));
            rainBowLearnMore.setSpan(
                    new BulletSpan(15, getColor(R.color.wallet_text_color)),
                    0,
                    rainBowLearnMore.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            SpannableString rainBowRisksLearnMore =
                    Utils.createSpanForSurroundedPhrase(
                            this,
                            R.string.brave_wallet_rainbow_bridge_risks_learn_more,
                            (v) -> {
                                TabUtils.openLinkWithFocus(
                                        this, WalletConstants.URL_RAINBOW_BRIDGE_RISKS);
                            });
            rainBowRisksLearnMore.setSpan(
                    new BulletSpan(15, getColor(R.color.wallet_text_color)),
                    0,
                    rainBowRisksLearnMore.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
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

            mShouldShowDialog =
                    ChromeSharedPreferences.getInstance()
                            .readBoolean(WalletConstants.PREF_SHOW_BRIDGE_INFO_DIALOG, true);

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
            checkBox.setOnCheckedChangeListener(
                    (buttonView, isChecked) -> {
                        ChromeSharedPreferences.getInstance()
                                .writeBoolean(
                                        WalletConstants.PREF_SHOW_BRIDGE_INFO_DIALOG, !isChecked);
                        mShouldShowDialog = !isChecked;
                    });

            message.setMovementMethod(LinkMovementMethod.getInstance());
            message.setText(getString(R.string.brave_wallet_aurora_modal_description,
                    getString(R.string.brave_wallet_rainbow_bridge)));
            btnOpenRainBow.setOnClickListener(
                    v -> TabUtils.openLinkWithFocus(this, WalletConstants.URL_RAINBOW_AURORA));

            mBtnBridgeToAurora.setOnClickListener(v -> {
                if (mShouldShowDialog) {
                    auroraDialog.show();
                } else {
                    TabUtils.openLinkWithFocus(this, WalletConstants.URL_RAINBOW_AURORA);
                }
            });
        }
        if (!mIsMarketCoin) {
            mBtnSwap.setOnClickListener(
                    v -> Utils.openBuySendSwapActivity(this, WebUiActivityType.SWAP));
            mBtnBuy.setOnClickListener(
                    v -> Utils.openBuySendSwapActivity(this, WebUiActivityType.BUY));
            btnSend.setOnClickListener(
                    v -> Utils.openBuySendSwapActivity(this, WebUiActivityType.SEND));
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
        chartES.setColors(new int[] {getColor(R.color.wallet_asset_graph_color)});
        chartES.drawLine(0, assetPrice);
        chartES.setNoDrawText(true);
        chartES.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                v.getParent().requestDisallowInterceptTouchEvent(true);
                SmoothLineChartEquallySpaced chartES = (SmoothLineChartEquallySpaced) v;
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
        if (!mIsMarketCoin && mHasNewTx) {
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
        String[] fromAssets = new String[] {asset.toLowerCase(Locale.ENGLISH)};
        String[] toAssets = new String[] {vsAsset.toLowerCase(Locale.ENGLISH)};
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
        if (JavaUtils.anyNull(mWalletModel, mAssetNetwork)) return;
        LiveDataUtil.observeOnce(
                mWalletModel.getKeyringModel().mAccountInfos, accounts -> {
                    mAccountInfos = AssetUtils.filterAccountsByNetwork(
                            accounts, mAssetNetwork.coin, mChainId);

                    WalletListItemModel thisAssetItemModel = new WalletListItemModel(
                            R.drawable.ic_eth, mAsset.name, mAsset.symbol, mAsset.tokenId, "", "");
                    LiveDataUtil.observeOnce(
                            mWalletModel.getNetworkModel().mCryptoNetworks, allNetworks -> {
                                Utils.getTxExtraInfo(new WeakReference<>(this),
                                        TokenUtils.TokenType.ALL, allNetworks, mAssetNetwork,
                                        mAccountInfos, new BlockchainToken[] {mAsset}, false,
                                        (assetPrices, fullTokenList, nativeAssetsBalances,
                                                blockchainTokensBalances) -> {
                                            thisAssetItemModel.setBlockchainToken(mAsset);
                                            Utils.setUpTransactionList(this, mAccountInfos,
                                                    allNetworks, thisAssetItemModel, assetPrices,
                                                    fullTokenList, nativeAssetsBalances,
                                                    blockchainTokensBalances, mAssetNetwork,
                                                    this::showTransactionList);

                                            double assetPrice = Utils.getOrDefault(assetPrices,
                                                    mAsset.symbol.toLowerCase(Locale.ENGLISH),
                                                    0.0d);

                                            List<WalletListItemModel> walletListItemModelList =
                                                    createAccountList(mAccountInfos, assetPrice,
                                                            nativeAssetsBalances,
                                                            blockchainTokensBalances);

                                            showAccounts(rvAccounts, walletCoinAdapter,
                                                    walletListItemModelList);
                                        });
                            });
                });
    }

    private void showTransactionList(List<WalletListItemModel> walletListItemModelList) {
        mWalletTxCoinAdapter.setWalletCoinAdapterType(
                WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);

        mWalletTxCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        mWalletTxCoinAdapter.setOnWalletListItemClick(AssetDetailActivity.this);
        mWalletTxCoinAdapter.setWalletListItemType(Utils.TRANSACTION_ITEM);
        RecyclerView rvTransactions = findViewById(R.id.rv_transactions);
        rvTransactions.setAdapter(mWalletTxCoinAdapter);
    }

    private Double getAccountBalance(HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
            String accountAddressLower) {
        return Utils.isNativeToken(mAssetNetwork, mAsset)
                ? Utils.getOrDefault(nativeAssetsBalances, accountAddressLower, 0.0d)
                : Utils.getOrDefault(Utils.getOrDefault(blockchainTokensBalances,
                                             accountAddressLower, new HashMap<>()),
                        Utils.tokenToString(mAsset), 0.0d);
    }

    // Get back token from native. If cannot find then something is wrong
    private void getBlockchainToken(Runnable callback) {
        if (mIsMarketCoin) {
            return;
        }
        if (mAsset != null || !mNativeInitialized || mAssetNetwork == null) {
            callback.run();
            return;
        }

        TokenUtils.getExactUserAsset(getBraveWalletService(), mAssetNetwork, mAssetNetwork.coin,
                mAssetSymbol, mAssetName, mAssetId, mContractAddress, mAssetDecimals, token -> {
                    assert token != null;
                    mAsset = token;
                    callback.run();
                });
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        mNativeInitialized = true;
        getPriceHistory(mAssetSymbol, "usd", AssetPriceTimeframe.ONE_DAY);
        getPrice(mAssetSymbol, "btc", AssetPriceTimeframe.LIVE);
        getBlockchainToken(this::setUpAccountList);
    }

    @Override
    public void onAccountClick(WalletListItemModel walletListItemModel) {
        if (walletListItemModel.getAccountInfo() == null) {
            return;
        }

        Intent intent =
                AccountDetailActivity.createIntent(this, walletListItemModel.getAccountInfo());
        startActivityForResult(intent, Utils.ACCOUNT_REQUEST_CODE);
    }

    @Override
    public void onTransactionClick(TransactionInfo txInfo) {
        if (mWalletModel == null) return;
        NetworkInfo txNetwork = mWalletModel.getNetworkModel().getNetwork(txInfo.chainId);
        Utils.openTransaction(txInfo, this, mCoinType, txNetwork);
    }

    @Override
    public void onTxApprovedRejected(boolean approved, String txId) {}

    @Override
    public void onTxPending(String txId) {}

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

    private List<WalletListItemModel> createAccountList(AccountInfo[] accountInfos,
            double assetPrice, HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances) {
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        for (AccountInfo accountInfo : accountInfos) {
            final String accountAddressLower = accountInfo.address.toLowerCase(Locale.ENGLISH);
            double thisAccountBalance = getAccountBalance(
                    nativeAssetsBalances, blockchainTokensBalances, accountAddressLower);
            final String fiatBalanceString =
                    String.format(Locale.ENGLISH, "$%,.2f", assetPrice * thisAccountBalance);
            final String cryptoBalanceString =
                    String.format(Locale.ENGLISH, "%.4f %s", thisAccountBalance, mAsset.symbol);

            walletListItemModelList.add(WalletListItemModel.makeForAccountInfoWithBalances(
                    accountInfo, fiatBalanceString, cryptoBalanceString));
        }
        return walletListItemModelList;
    }

    private void showAccounts(RecyclerView rvAccounts, WalletCoinAdapter walletCoinAdapter,
            List<WalletListItemModel> walletListItemModelList) {
        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(this);
        walletCoinAdapter.setWalletListItemType(Utils.ACCOUNT_ITEM);
        rvAccounts.setAdapter(walletCoinAdapter);
        rvAccounts.setLayoutManager(new LinearLayoutManager(this));
    }

    private void adjustButtonsVisibilities() {
        if (mIsMarketCoin) {
            return;
        }
        showHideBuyUi();
        if (Utils.allowSwap(mChainId)) {
            mBtnSwap.setVisibility(View.VISIBLE);
        } else {
            mBtnSwap.setVisibility(View.GONE);
        }
    }

    private void showHideBuyUi() {
        if (!Utils.allowBuy(mChainId)) {
            AndroidUtils.gone(mBtnBuy);
            return;
        }
        if (JavaUtils.anyNull(mWalletModel, mAssetNetwork)) return;

        TokenUtils.isBuySupported(mWalletModel.getBlockchainRegistry(), mAssetNetwork, mAssetSymbol,
                mContractAddress, mChainId, isBuyEnabled -> {
                    if (isBuyEnabled) {
                        AndroidUtils.show(mBtnBuy);
                    } else {
                        AndroidUtils.gone(mBtnBuy);
                    }
                });
    }
}

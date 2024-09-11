/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import static org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType.ADD_ETHEREUM_CHAIN;
import static org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType.SWITCH_ETHEREUM_CHAIN;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

import org.chromium.brave_wallet.mojom.AddChainRequest;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OriginInfo;
import org.chromium.brave_wallet.mojom.SwitchChainRequest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType;
import org.chromium.chrome.browser.crypto_wallet.adapters.FragmentNavigationItemAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItem;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItemText;
import org.chromium.chrome.browser.crypto_wallet.fragments.TwoLineItemFragment;
import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper.DefaultFaviconHelper;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper.FaviconImageCallback;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;

public class AddSwitchChainNetworkFragment extends BaseDAppsFragment {
    private final List<NavigationItem> mTabTitles;
    private final ActivityType mPanelType;
    private NetworkInfo mNetworkInfo;
    private BraveWalletBaseActivity mBraveWalletBaseActivity;
    private SwitchChainRequest mSwitchChainRequest;
    private AddChainRequest mAddChainRequest;
    private AddSwitchRequestProcessListener mAddSwitchRequestProcessListener;
    private boolean mHasMultipleAddSwitchChainRequest;
    private final List<TwoLineItem> mNetworks;
    private final List<TwoLineItem> mDetails;
    private ImageView mFavicon;
    private TextView mSiteTv;
    private FaviconHelper mFaviconHelper;
    private DefaultFaviconHelper mDefaultFaviconHelper;

    public AddSwitchChainNetworkFragment(ActivityType panelType) {
        mPanelType = panelType;
        mTabTitles = new ArrayList<>();
        mNetworks = new ArrayList<>();
        mDetails = new ArrayList<>();
    }

    public AddSwitchChainNetworkFragment(
            ActivityType panelType,
            AddSwitchRequestProcessListener addSwitchRequestProcessListener) {
        this(panelType);
        mAddSwitchRequestProcessListener = addSwitchRequestProcessListener;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mTabTitles.add(
                new NavigationItem(
                        getString(R.string.network_text), new TwoLineItemFragment(mNetworks)));
        mTabTitles.add(
                new NavigationItem(getString(R.string.details), new TwoLineItemFragment(mDetails)));
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_add_switch_ethereum_chain, container, false);
        ViewPager2 viewPager =
                view.findViewById(R.id.fragment_switch_eth_chain_tv_message_view_pager);
        TabLayout tabLayout = view.findViewById(R.id.fragment_switch_eth_chain_tv_message_tabs);
        viewPager.setUserInputEnabled(false);
        FragmentNavigationItemAdapter adapter = new FragmentNavigationItemAdapter(this, mTabTitles);
        viewPager.setAdapter(adapter);
        new TabLayoutMediator(
                        tabLayout,
                        viewPager,
                        (tab, position) -> tab.setText(mTabTitles.get(position).getTitle()))
                .attach();
        Button btnAddSwitchNetwork = view.findViewById(R.id.fragment_add_switch_eth_chain_btn_sign);
        if (mPanelType == ADD_ETHEREUM_CHAIN) {
            btnAddSwitchNetwork.setText(R.string.approve);
            TextView tvAddChainTitle = view.findViewById(R.id.fragment_add_switch_chain_tv_title);
            tvAddChainTitle.setText(R.string.brave_wallet_allow_add_network_heading);
            TextView addChainDesc = view.findViewById(R.id.fragment_add_switch_chain_tv_text);
            Spanned spannedDescriptionText =
                    Utils.createSpanForSurroundedPhrase(
                            requireContext(),
                            R.string.brave_wallet_allow_add_network_description,
                            (v) -> {
                                TabUtils.openUrlInNewTab(false, Utils.BRAVE_SUPPORT_URL);
                                TabUtils.bringChromeTabbedActivityToTheTop(getActivity());
                            });
            addChainDesc.setMovementMethod(LinkMovementMethod.getInstance());
            addChainDesc.setText(spannedDescriptionText);

        } else if (mPanelType == SWITCH_ETHEREUM_CHAIN) {
            btnAddSwitchNetwork.setText(R.string.brave_wallet_allow_change_network_button);
        }
        btnAddSwitchNetwork.setOnClickListener(
                v -> {
                    if (mPanelType == ADD_ETHEREUM_CHAIN) {
                        processAddChainRequest(mNetworkInfo, true);
                        processNextAddNetworkRequest();
                    } else if (mPanelType == SWITCH_ETHEREUM_CHAIN) {
                        processSwitchChainRequest(mSwitchChainRequest, true);
                        processNextSwitchChainRequest();
                    }
                });
        Button btnAddSwitchCancel =
                view.findViewById(R.id.fragment_add_switch_eth_chain_btn_cancel);
        btnAddSwitchCancel.setOnClickListener(
                v -> {
                    if (mPanelType == ADD_ETHEREUM_CHAIN) {
                        processAddChainRequest(mNetworkInfo, false);
                        processNextAddNetworkRequest();
                    } else if (mPanelType == SWITCH_ETHEREUM_CHAIN) {
                        processSwitchChainRequest(mSwitchChainRequest, false);
                        processNextSwitchChainRequest();
                    }
                });
        mFavicon = view.findViewById(R.id.fragment_add_token_iv_domain_icon);
        mSiteTv = view.findViewById(R.id.fragment_add_token_tv_site);

        return view;
    }

    private void showFavIcon(GURL url) {
        mFaviconHelper = new FaviconHelper();
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            FaviconImageCallback imageCallback =
                    (bitmap, iconUrl) -> setBitmapOnImageView(url, bitmap);
            // 0 is a max bitmap size for download
            mFaviconHelper.getLocalFaviconImageForURL(
                    activity.getCurrentProfile(), url, 0, imageCallback);

        } catch (Exception ignored) {
            /* Ignored. */
        }
    }

    private void setBitmapOnImageView(GURL pageUrl, Bitmap iconBitmap) {
        if (iconBitmap == null) {
            if (mDefaultFaviconHelper == null) mDefaultFaviconHelper = new DefaultFaviconHelper();
            iconBitmap =
                    mDefaultFaviconHelper.getDefaultFaviconBitmap(getActivity(), pageUrl, true);
        }
        mFavicon.setImageBitmap(iconBitmap);
        mFavicon.setVisibility(View.VISIBLE);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        fetchNetworkInfo();
    }

    private void fetchNetworkInfo() {
        assert getActivity() instanceof BraveWalletBaseActivity;
        mBraveWalletBaseActivity = (BraveWalletBaseActivity) getActivity();
        if (mPanelType == ADD_ETHEREUM_CHAIN) {
            mBraveWalletBaseActivity
                    .getJsonRpcService()
                    .getPendingAddChainRequests(
                            addChainRequests -> {
                                if (addChainRequests == null || addChainRequests.length == 0) {
                                    return;
                                }
                                mAddChainRequest = addChainRequests[0];
                                mNetworkInfo = mAddChainRequest.networkInfo;
                                mHasMultipleAddSwitchChainRequest = addChainRequests.length > 1;
                                updateState();
                                fillOriginInfo(mAddChainRequest.originInfo);
                            });
        } else if (mPanelType == SWITCH_ETHEREUM_CHAIN) {
            mBraveWalletBaseActivity
                    .getJsonRpcService()
                    .getPendingSwitchChainRequests(
                            switchChainRequests -> {
                                mSwitchChainRequest = switchChainRequests[0];
                                mHasMultipleAddSwitchChainRequest = switchChainRequests.length > 1;
                                mBraveWalletBaseActivity
                                        .getJsonRpcService()
                                        .getAllNetworks(
                                                chains -> {
                                                    for (NetworkInfo network : chains) {
                                                        if (network.coin != CoinType.ETH) {
                                                            continue;
                                                        }
                                                        if (mSwitchChainRequest.chainId.equals(
                                                                network.chainId)) {
                                                            mNetworkInfo = network;
                                                            updateState();
                                                            break;
                                                        }
                                                    }
                                                });
                                fillOriginInfo(mSwitchChainRequest.originInfo);
                            });
        }
    }

    private void fillOriginInfo(OriginInfo originInfo) {
        if (originInfo != null && URLUtil.isValidUrl(originInfo.originSpec)) {
            mSiteTv.setText(Utils.geteTldSpanned(originInfo));
            showFavIcon(new GURL(originInfo.originSpec));
        }
    }

    private void updateState() {
        if (getView() != null && mNetworkInfo != null) {
            addNetworkTabInfo(mNetworkInfo);
            addDetailsTabInfo(mNetworkInfo);
            updateNetworkInfo();
        }
    }

    private void processNextSwitchChainRequest() {
        if (mAddSwitchRequestProcessListener != null) {
            mAddSwitchRequestProcessListener.onSwitchRequestProcessed(
                    mHasMultipleAddSwitchChainRequest);
        }
    }

    private void processNextAddNetworkRequest() {
        if (mAddSwitchRequestProcessListener != null) {
            mAddSwitchRequestProcessListener.onAddRequestProcessed(
                    mHasMultipleAddSwitchChainRequest);
        }
    }

    private void processSwitchChainRequest(
            SwitchChainRequest switchChainRequest, boolean isApproved) {
        mBraveWalletBaseActivity
                .getJsonRpcService()
                .notifySwitchChainRequestProcessed(switchChainRequest.requestId, isApproved);
    }

    private void processAddChainRequest(NetworkInfo networkInfo, boolean isApproved) {
        mBraveWalletBaseActivity
                .getJsonRpcService()
                .addEthereumChainRequestCompleted(networkInfo.chainId, isApproved);
    }

    private void updateNetworkInfo() {
        for (NavigationItem navigationItem : mTabTitles) {
            ((TwoLineItemFragment) navigationItem.getFragment()).invalidateData();
        }
    }

    private String getActiveRpcEndpointUrl(NetworkInfo networkInfo) {
        if (networkInfo.activeRpcEndpointIndex >= 0
                && networkInfo.activeRpcEndpointIndex < networkInfo.rpcEndpoints.length) {
            return networkInfo.rpcEndpoints[networkInfo.activeRpcEndpointIndex].url;
        }
        return "";
    }

    private void addDetailsTabInfo(NetworkInfo networkInfo) {
        mNetworks.clear();
        mNetworks.add(
                new TwoLineItemText(
                        getString(R.string.brave_wallet_allow_add_network_name),
                        networkInfo.chainName));
        mNetworks.add(
                new TwoLineItemText(
                        getString(R.string.brave_wallet_allow_add_network_url),
                        getActiveRpcEndpointUrl(networkInfo)));
    }

    private void addNetworkTabInfo(NetworkInfo networkInfo) {
        mDetails.clear();
        mDetails.add(
                new TwoLineItemText(
                        getString(R.string.brave_wallet_allow_add_network_name),
                        networkInfo.chainName));
        mDetails.add(
                new TwoLineItemText(
                        getString(R.string.brave_wallet_allow_add_network_url),
                        getActiveRpcEndpointUrl(networkInfo)));
        mDetails.add(
                new TwoLineItemText(
                        getString(R.string.brave_wallet_chain_id), networkInfo.chainId));
        mDetails.add(
                new TwoLineItemText(
                        getString(R.string.brave_wallet_allow_add_network_currency_symbol),
                        networkInfo.symbol));
        mDetails.add(
                new TwoLineItemText(
                        getString(R.string.wallet_add_custom_asset_decimals),
                        String.valueOf(networkInfo.decimals)));
        mDetails.add(
                new TwoLineItemText(
                        getString(R.string.brave_wallet_add_network_block_explorer_urls),
                        networkInfo.blockExplorerUrls.length > 0
                                ? networkInfo.blockExplorerUrls[0]
                                : ""));
    }

    public interface AddSwitchRequestProcessListener {
        void onAddRequestProcessed(boolean hasMoreRequests);

        void onSwitchRequestProcessed(boolean hasMoreRequests);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mFaviconHelper != null) mFaviconHelper.destroy();
        if (mDefaultFaviconHelper != null) mDefaultFaviconHelper.clearCache();
    }
}

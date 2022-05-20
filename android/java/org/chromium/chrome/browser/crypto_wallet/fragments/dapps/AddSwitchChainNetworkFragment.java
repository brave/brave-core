package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import static org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType.ADD_ETHEREUM_CHAIN;
import static org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType.SWITCH_ETHEREUM_CHAIN;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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
import org.chromium.brave_wallet.mojom.SwitchChainRequest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType;
import org.chromium.chrome.browser.crypto_wallet.adapters.FragmentNavigationItemAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItemDataSource;
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
    private List<NavigationItem> mTabTitles;
    private ActivityType mPanelType;
    private NetworkInfo mNetworkInfo;
    private BraveWalletBaseActivity mBraveWalletBaseActivity;
    private SwitchChainRequest mSwitchChainRequest;
    private AddChainRequest mAddChainRequest;
    private AddSwitchRequestProcessListener mAddSwitchRequestProcessListener;
    private boolean hasMultipleAddSwitchChainRequest;
    private List<TwoLineItemDataSource> networks;
    private List<TwoLineItemDataSource> details;
    private ImageView mFavicon;
    private FaviconHelper mFaviconHelper;
    private DefaultFaviconHelper mDefaultFaviconHelper;

    public AddSwitchChainNetworkFragment(BraveWalletDAppsActivity.ActivityType panelType) {
        mPanelType = panelType;
        mTabTitles = new ArrayList<>();
        networks = new ArrayList<>();
        details = new ArrayList<>();
    }

    public AddSwitchChainNetworkFragment(BraveWalletDAppsActivity.ActivityType panelType,
            AddSwitchRequestProcessListener addSwitchRequestProcessListener) {
        this(panelType);
        mAddSwitchRequestProcessListener = addSwitchRequestProcessListener;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mTabTitles.add(new NavigationItem(getString(R.string.brave_wallet_allow_add_network_title),
                new TwoLineItemFragment(networks)));
        mTabTitles.add(
                new NavigationItem(getString(R.string.details), new TwoLineItemFragment(details)));
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_add_switch_ethereum_chain, container, false);
        ViewPager2 viewPager =
                view.findViewById(R.id.fragment_switch_eth_chain_tv_message_view_pager);
        TabLayout tabLayout = view.findViewById(R.id.fragment_switch_eth_chain_tv_message_tabs);
        viewPager.setUserInputEnabled(false);
        FragmentNavigationItemAdapter adapter = new FragmentNavigationItemAdapter(this, mTabTitles);
        viewPager.setAdapter(adapter);
        new TabLayoutMediator(tabLayout, viewPager,
                (tab, position) -> tab.setText(mTabTitles.get(position).getTitle()))
                .attach();
        Button btnAddSwitchNetwork = view.findViewById(R.id.fragment_add_switch_eth_chain_btn_sign);
        if (mPanelType == ADD_ETHEREUM_CHAIN) {
            btnAddSwitchNetwork.setText(R.string.approve);
            SpannableString addChainHeading = Utils.createSpannableString(
                    getString(R.string.brave_wallet_allow_add_network_description),
                    getString(R.string.brave_wallet), new ClickableSpan() {
                        @Override
                        public void onClick(@NonNull View widget) {
                            TabUtils.openUrlInNewTab(false, Utils.BRAVE_SUPPORT_URL);
                            TabUtils.bringChromeTabbedActivityToTheTop(getActivity());
                        }
                    }, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

            TextView tvAddChainTitle = view.findViewById(R.id.fragment_add_switch_chain_tv_title);
            tvAddChainTitle.setText(R.string.brave_wallet_allow_add_network_heading);

            TextView addChainDesc = view.findViewById(R.id.fragment_add_switch_chain_tv_text);
            addChainDesc.setMovementMethod(LinkMovementMethod.getInstance());
            addChainDesc.setText(addChainHeading);

        } else if (mPanelType == SWITCH_ETHEREUM_CHAIN) {
            btnAddSwitchNetwork.setText(R.string.brave_wallet_allow_change_network_button);
        }
        btnAddSwitchNetwork.setOnClickListener(v -> {
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
        btnAddSwitchCancel.setOnClickListener(v -> {
            if (mPanelType == ADD_ETHEREUM_CHAIN) {
                processAddChainRequest(mNetworkInfo, false);
                processNextAddNetworkRequest();
            } else if (mPanelType == SWITCH_ETHEREUM_CHAIN) {
                processSwitchChainRequest(mSwitchChainRequest, false);
                processNextSwitchChainRequest();
            }
        });
        mFavicon = view.findViewById(R.id.fragment_add_token_iv_domain_icon);
        TextView siteTv = view.findViewById(R.id.fragment_add_token_tv_site);
        GURL siteUrl = Utils.getCurentTabUrl();
        if (siteUrl != null) {
            getBraveWalletService().geteTldPlusOneFromOrigin(Utils.getCurrentMojomOrigin(),
                    origin -> { siteTv.setText(Utils.geteTLD(origin.eTldPlusOne)); });
            showFavIcon(siteUrl.getOrigin());
        }
        return view;
    }

    private void showFavIcon(GURL url) {
        mFaviconHelper = new FaviconHelper();
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            FaviconImageCallback imageCallback =
                    (bitmap, iconUrl) -> setBitmapOnImageView(url, bitmap);
            // 0 is a max bitmap size for download
            mFaviconHelper.getLocalFaviconImageForURL(
                    activity.getCurrentProfile(), url, 0, imageCallback);
        }
    }

    private void setBitmapOnImageView(GURL pageUrl, Bitmap iconBitmap) {
        if (iconBitmap == null) {
            if (mDefaultFaviconHelper == null) mDefaultFaviconHelper = new DefaultFaviconHelper();
            iconBitmap = mDefaultFaviconHelper.getDefaultFaviconBitmap(
                    getActivity().getResources(), pageUrl, true);
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
            mBraveWalletBaseActivity.getJsonRpcService().getPendingAddChainRequests(
                    addChainRequests -> {
                        mAddChainRequest = addChainRequests[0];
                        mNetworkInfo = mAddChainRequest.networkInfo;
                        hasMultipleAddSwitchChainRequest = addChainRequests.length > 1;
                        updateState();
                    });
        } else if (mPanelType == SWITCH_ETHEREUM_CHAIN) {
            mBraveWalletBaseActivity.getJsonRpcService().getPendingSwitchChainRequests(
                    switchChainRequests -> {
                        mSwitchChainRequest = switchChainRequests[0];
                        hasMultipleAddSwitchChainRequest = switchChainRequests.length > 1;
                        mBraveWalletBaseActivity.getJsonRpcService().getAllNetworks(
                                CoinType.ETH, chains -> {
                                    for (NetworkInfo network : chains) {
                                        if (mSwitchChainRequest.chainId.equals(network.chainId)) {
                                            mNetworkInfo = network;
                                            updateState();
                                            break;
                                        }
                                    }
                                });
                    });
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
                    hasMultipleAddSwitchChainRequest);
        }
    }

    private void processNextAddNetworkRequest() {
        if (mAddSwitchRequestProcessListener != null) {
            mAddSwitchRequestProcessListener.onAddRequestProcessed(
                    hasMultipleAddSwitchChainRequest);
        }
    }

    private void processSwitchChainRequest(
            SwitchChainRequest switchChainRequest, boolean isApproved) {
        mBraveWalletBaseActivity.getJsonRpcService().notifySwitchChainRequestProcessed(
                isApproved, switchChainRequest.originInfo.origin);
    }

    private void processAddChainRequest(NetworkInfo networkInfo, boolean isApproved) {
        mBraveWalletBaseActivity.getJsonRpcService().addEthereumChainRequestCompleted(
                networkInfo.chainId, isApproved);
    }

    private void updateNetworkInfo() {
        for (NavigationItem navigationItem : mTabTitles) {
            ((TwoLineItemFragment) navigationItem.getFragment()).invalidateData();
        }
    }

    private void addDetailsTabInfo(NetworkInfo networkInfo) {
        networks.clear();
        networks.add(new TwoLineItemDataSource(
                getString(R.string.brave_wallet_allow_add_network_name), networkInfo.chainName));
        networks.add(
                new TwoLineItemDataSource(getString(R.string.brave_wallet_allow_add_network_url),
                        networkInfo.rpcUrls.length > 0 ? networkInfo.rpcUrls[0] : ""));
    }

    private void addNetworkTabInfo(NetworkInfo networkInfo) {
        details.clear();
        details.add(new TwoLineItemDataSource(
                getString(R.string.brave_wallet_allow_add_network_name), networkInfo.chainName));
        details.add(
                new TwoLineItemDataSource(getString(R.string.brave_wallet_allow_add_network_url),
                        networkInfo.rpcUrls.length > 0 ? networkInfo.rpcUrls[0] : ""));
        details.add(new TwoLineItemDataSource(
                getString(R.string.brave_wallet_allow_add_network_chain_id), networkInfo.chainId));
        details.add(new TwoLineItemDataSource(
                getString(R.string.brave_wallet_allow_add_network_currency_symbol),
                networkInfo.symbol));
        details.add(new TwoLineItemDataSource(getString(R.string.wallet_add_custom_asset_decimals),
                String.valueOf(networkInfo.decimals)));
        details.add(new TwoLineItemDataSource(
                getString(R.string.brave_wallet_add_network_block_explorer_urls),
                networkInfo.blockExplorerUrls.length > 0 ? networkInfo.blockExplorerUrls[0] : ""));
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

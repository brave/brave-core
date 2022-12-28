/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;

import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.mojom.Url;

import java.net.MalformedURLException;
import java.net.URL;

public class BraveWalletAddNetworksFragment extends Fragment implements ConnectionErrorHandler {
    interface Refresher {
        void refreshNetworksList();
    }
    /**
     * A host to launch BraveWalletAddNetworksFragment and receive the result.
     */
    interface Launcher {
        /**
         * Launches BraveWalletAddNetworksFragment.
         */
        void launchAddNetwork(String chainId, boolean activeNetwork);
        void setRefresher(Refresher refresher);
    }

    private JsonRpcService mJsonRpcService;
    private String mChainId;
    private EditText mChainIdEditText;
    private boolean mIsActiveNetwork;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.brave_wallet_networks_title);
        mIsActiveNetwork =
                getArguments().getBoolean(ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK, false);
        InitJsonRpcService();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mJsonRpcService.close();
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.brave_wallet_add_network, container, false);

        Button btAdd = view.findViewById(R.id.add);

        if (!mIsActiveNetwork) {
            btAdd.setOnClickListener(v -> {
                if (!validateInputsAddChain(view)) {
                    // We have some errors in inputs
                    return;
                }
            });
        } else {
            btAdd.setVisibility(View.GONE);
        }
        mChainId = getArguments().getString(ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID);
        mChainIdEditText = view.findViewById(R.id.chain_id);
        if (!mChainId.isEmpty()) {
            btAdd.setText(R.string.brave_wallet_add_network_submit);
            assert mJsonRpcService != null;
            mJsonRpcService.getAllNetworks(CoinType.ETH, networks -> {
                for (NetworkInfo chain : networks) {
                    if (chain.chainId.equals(mChainId)) {
                        fillControls(chain, view);
                        break;
                    }
                }
            });
        }

        return view;
    }

    @Override
    public void onConnectionError(MojoException e) {
        mJsonRpcService.close();
        mJsonRpcService = null;
        InitJsonRpcService();
    }

    private void InitJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = JsonRpcServiceFactory.getInstance().getJsonRpcService(this);
    }

    private void fillControls(NetworkInfo chain, View view) {
        EditText chainCurrencyDecimals = view.findViewById(R.id.chain_currency_decimals);
        String strChainId = chain.chainId;
        if (strChainId.startsWith("0x")) {
            strChainId = strChainId.substring(2);
        }
        try {
            int chainId = Integer.parseInt(strChainId, 16);
            mChainIdEditText.setText(String.valueOf(chainId));
            chainCurrencyDecimals.setText(String.valueOf(chain.decimals));
        } catch (NumberFormatException exc) {
        }

        EditText chainName = view.findViewById(R.id.chain_name);
        chainName.setText(chain.chainName);

        EditText chainCurrencyName = view.findViewById(R.id.chain_currency_name);
        chainCurrencyName.setText(chain.symbolName);

        EditText chainCurrencySymbol = view.findViewById(R.id.chain_currency_symbol);
        chainCurrencySymbol.setText(chain.symbol);

        if (chain.activeRpcEndpointIndex >= 0
                && chain.activeRpcEndpointIndex < chain.rpcEndpoints.length) {
            EditText rpcUrls = view.findViewById(R.id.rpc_urls);
            rpcUrls.setText(chain.rpcEndpoints[chain.activeRpcEndpointIndex].toString());
        }

        if (chain.iconUrls.length > 0) {
            EditText iconUrls = view.findViewById(R.id.icon_urls);
            iconUrls.setText(chain.iconUrls[0]);
        }

        if (chain.blockExplorerUrls.length > 0) {
            EditText blockExplorerUrls = view.findViewById(R.id.block_explorer_urls);
            blockExplorerUrls.setText(chain.blockExplorerUrls[0]);
        }
        if (mIsActiveNetwork) {
            AndroidUtils.disableViewsByIds(view, R.id.chain_id, R.id.chain_currency_decimals,
                    R.id.chain_name, R.id.chain_currency_name, R.id.chain_currency_symbol,
                    R.id.rpc_urls, R.id.icon_urls, R.id.block_explorer_urls);
        }
    }

    private boolean validateInputsAddChain(View view) {
        NetworkInfo chain = new NetworkInfo();
        String strChainId = mChainIdEditText.getText().toString().trim();
        try {
            int iChainId = Integer.valueOf(strChainId);
            if (iChainId <= 0) {
                mChainIdEditText.setError(
                        getString(R.string.brave_wallet_add_network_chain_id_error));

                return false;
            }
            strChainId = "0x" + Integer.toHexString(iChainId);
        } catch (NumberFormatException exc) {
            mChainIdEditText.setError(getString(R.string.brave_wallet_add_network_chain_id_error));

            return false;
        }
        chain.chainId = strChainId;

        EditText chainName = view.findViewById(R.id.chain_name);
        String strChainName = chainName.getText().toString().trim();
        if (strChainName.isEmpty()) {
            chainName.setError(getString(R.string.brave_wallet_add_network_chain_empty_error));

            return false;
        }
        chain.chainName = strChainName;

        EditText chainCurrencyName = view.findViewById(R.id.chain_currency_name);
        String strChainCurrencyName = chainCurrencyName.getText().toString().trim();
        if (strChainCurrencyName.isEmpty()) {
            chainCurrencyName.setError(
                    getString(R.string.brave_wallet_add_network_chain_empty_error));

            return false;
        }
        chain.symbolName = strChainCurrencyName;

        EditText chainCurrencySymbol = view.findViewById(R.id.chain_currency_symbol);
        String strChainCurrencySymbol = chainCurrencySymbol.getText().toString().trim();
        if (strChainCurrencySymbol.isEmpty()) {
            chainCurrencySymbol.setError(
                    getString(R.string.brave_wallet_add_network_chain_empty_error));

            return false;
        }
        chain.symbol = strChainCurrencySymbol;

        EditText chainCurrencyDecimals = view.findViewById(R.id.chain_currency_decimals);
        String strChainCurrencyDecimals = chainCurrencyDecimals.getText().toString().trim();
        try {
            int iChainCurrencyDecimals = Integer.valueOf(strChainCurrencyDecimals);
            if (iChainCurrencyDecimals <= 0) {
                chainCurrencyDecimals.setError(
                        getString(R.string.brave_wallet_add_network_chain_currency_decimals_error));

                return false;
            }
            chain.decimals = iChainCurrencyDecimals;
        } catch (NumberFormatException exc) {
            chainCurrencyDecimals.setError(
                    getString(R.string.brave_wallet_add_network_chain_currency_decimals_error));

            return false;
        }

        EditText rpcUrls = view.findViewById(R.id.rpc_urls);
        String strRpcUrls = rpcUrls.getText().toString().trim();
        try {
            URL url = new URL(strRpcUrls);
            if (!url.getProtocol().equals("http") && !url.getProtocol().equals("https")) {
                rpcUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));

                return false;
            }
            org.chromium.url.mojom.Url mojourl = new Url();
            mojourl.url = strRpcUrls;
            chain.rpcEndpoints = new Url[] {mojourl};
            chain.activeRpcEndpointIndex = 0;

        } catch (MalformedURLException exc) {
            rpcUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));

            return false;
        }

        EditText iconUrls = view.findViewById(R.id.icon_urls);
        String strIconUrls = iconUrls.getText().toString().trim();
        if (!strIconUrls.isEmpty()) {
            try {
                URL url = new URL(strIconUrls);
                if (!url.getProtocol().equals("http") && !url.getProtocol().equals("https")) {
                    iconUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));

                    return false;
                }
                chain.iconUrls = new String[] {strIconUrls};
            } catch (MalformedURLException exc) {
                iconUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));

                return false;
            }
        } else {
            chain.iconUrls = new String[0];
        }

        EditText blockExplorerUrls = view.findViewById(R.id.block_explorer_urls);
        String strBlockExplorerUrls = blockExplorerUrls.getText().toString().trim();
        if (!strBlockExplorerUrls.isEmpty()) {
            try {
                URL url = new URL(strBlockExplorerUrls);
                if (!url.getProtocol().equals("http") && !url.getProtocol().equals("https")) {
                    blockExplorerUrls.setError(
                            getString(R.string.brave_wallet_add_network_urls_error));

                    return false;
                }
                chain.blockExplorerUrls = new String[] {strBlockExplorerUrls};
            } catch (MalformedURLException exc) {
                blockExplorerUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));

                return false;
            }
        } else {
            chain.blockExplorerUrls = new String[0];
        }
        chain.coin = CoinType.ETH;

        assert mJsonRpcService != null;
        if (!mChainId.isEmpty()) {
            if (mChainId.equals(chain.chainId)) {
                mJsonRpcService.removeChain(mChainId, CoinType.ETH, success -> {
                    if (!success) {
                        return;
                    }
                    addChain(chain, false);
                });
            } else {
                addChain(chain, true);
            }
        } else {
            addChain(chain, false);
        }

        return true;
    }

    private void addChain(NetworkInfo chain, boolean remove) {
        assert mJsonRpcService != null;
        mJsonRpcService.addChain(chain, (chainId, error, errorMessage) -> {
            if (error != ProviderError.SUCCESS) {
                // (sergz): Perhaps we will need to add more errors in the future.
                // We support only that one for now from backend
                if (errorMessage.contains("eth_chainId")) {
                    mChainIdEditText.setError(errorMessage);
                }
                return;
            }
            if (remove) {
                mJsonRpcService.removeChain(mChainId, CoinType.ETH, success -> {
                    // We just do nothing here as we added a chain with a diff
                    // chainId already
                    refreshNetworksFinishFragment();
                });
            }
            refreshNetworksFinishFragment();

            return;
        });
    }

    private void refreshNetworksFinishFragment() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.getWalletModel().getCryptoModel().getNetworkModel().refreshNetworks();
        }
        getActivity().finish();
    }
}

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.app.Activity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;
import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.url.mojom.Url;

import java.net.MalformedURLException;
import java.net.URL;

public class BraveWalletAddNetworksFragment extends Fragment implements ConnectionErrorHandler {

    /**
     * Listener implemented by {@link BraveWalletNetworksPreferenceFragment} used to notify the
     * fragment hosting network preference screen that a network is being added or modified.
     */
    public interface Listener {
        /** Adds a new network. */
        void addNewNetwork();

        /** Modifies an existing network. */
        void modifyNetwork(String chainId, boolean activeNetwork);
    }

    private JsonRpcService mJsonRpcService;
    private boolean mIsActiveNetwork;

    @Nullable private String mChainId;
    private EditText mChainIdEditText;
    private EditText mChainName;
    private EditText mChainCurrencyName;
    private EditText mChainCurrencySymbol;
    private EditText mChainCurrencyDecimals;
    private EditText mRpcUrls;
    private EditText mIconUrls;
    private EditText mBlockExplorerUrls;
    private TextView mSubmitError;
    private AppCompatButton mButtonSubmit;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requireActivity().setTitle(R.string.brave_wallet_networks_title);
        Bundle arguments = getArguments();
        if (arguments != null) {
            mIsActiveNetwork = arguments.getBoolean(ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK, false);
            mChainId = getArguments().getString(ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID);
        }
        initJsonRpcService();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mJsonRpcService.close();
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_wallet_add_network, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mChainIdEditText = view.findViewById(R.id.chain_id);
        mChainName = view.findViewById(R.id.chain_name);
        mChainCurrencyName = view.findViewById(R.id.chain_currency_name);
        mChainCurrencySymbol = view.findViewById(R.id.chain_currency_symbol);
        mChainCurrencyDecimals = view.findViewById(R.id.chain_currency_decimals);
        mRpcUrls = view.findViewById(R.id.rpc_urls);
        mIconUrls = view.findViewById(R.id.icon_urls);
        mBlockExplorerUrls = view.findViewById(R.id.block_explorer_urls);
        mButtonSubmit = view.findViewById(R.id.submit);
        mSubmitError = view.findViewById(R.id.submit_error);

        if (!mIsActiveNetwork) {
            mButtonSubmit.setOnClickListener(v -> validateInputsAddChain());
        } else {
            mButtonSubmit.setVisibility(View.GONE);
        }
        if (!TextUtils.isEmpty(mChainId)) {
            mButtonSubmit.setText(R.string.brave_wallet_add_network_submit);
            assert mJsonRpcService != null;
            mJsonRpcService.getAllNetworks(
                    networks -> {
                        for (NetworkInfo chain : networks) {
                            if (chain.coin != CoinType.ETH) {
                                continue;
                            }
                            if (chain.chainId.equals(mChainId)) {
                                fillControls(chain);
                                break;
                            }
                        }
                    });
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
        mJsonRpcService.close();
        mJsonRpcService = null;
        initJsonRpcService();
    }

    private void initJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = BraveWalletServiceFactory.getInstance().getJsonRpcService(this);
    }

    private void fillControls(@NonNull final NetworkInfo chain) {
        String strChainId = chain.chainId;
        if (strChainId.startsWith("0x")) {
            strChainId = strChainId.substring(2);
        }
        try {
            int chainId = Integer.parseInt(strChainId, 16);
            mChainIdEditText.setText(String.valueOf(chainId));
            mChainCurrencyDecimals.setText(String.valueOf(chain.decimals));
        } catch (NumberFormatException ignored) {
            /* Ignored. */
        }

        mChainName.setText(chain.chainName);
        mChainCurrencyName.setText(chain.symbolName);
        mChainCurrencySymbol.setText(chain.symbol);

        if (chain.activeRpcEndpointIndex >= 0
                && chain.activeRpcEndpointIndex < chain.rpcEndpoints.length) {
            mRpcUrls.setText(chain.rpcEndpoints[chain.activeRpcEndpointIndex].url);
        }

        if (chain.iconUrls.length > 0) {
            mIconUrls.setText(chain.iconUrls[0]);
        }

        if (chain.blockExplorerUrls.length > 0) {
            mBlockExplorerUrls.setText(chain.blockExplorerUrls[0]);
        }
        if (mIsActiveNetwork) {
            AndroidUtils.disable(
                    mChainIdEditText,
                    mChainCurrencyDecimals,
                    mChainName,
                    mChainCurrencyName,
                    mChainCurrencySymbol,
                    mRpcUrls,
                    mIconUrls,
                    mBlockExplorerUrls);
        }
    }

    /**
     * Validates input fields and adds a new chain. Based on Desktop implementation the following
     * logic applies:
     *
     * <ul>
     *   <li>Chain ID, chain name, currency name, currency symbol, currency decimals and RPC URL are
     *       mandatory fields and cannot be empty.
     *   <li>Icon URL and block explorer URL can be empty.
     *   <li>Chain's currency decimals accept only numbers greater than 0.
     *   <li>RPC URL, icon URL and block explorer URL support only HTTP and HTTPS protocols.
     *   <li>Chain ID and RPC URL are cross validated: if the chain ID returned by RPC endpoint
     *       doesn't match the chain ID provided an error message will be displayed.
     * </ul>
     */
    private void validateInputsAddChain() {
        boolean error = false;
        mSubmitError.setVisibility(View.INVISIBLE);
        mButtonSubmit.setEnabled(false);

        NetworkInfo chain = new NetworkInfo();
        String strChainId = mChainIdEditText.getText().toString().trim();
        try {
            int parsedChainId = Integer.parseInt(strChainId);
            if (parsedChainId <= 0) {
                mChainIdEditText.setError(
                        getString(R.string.brave_wallet_add_network_chain_id_error));
                error = true;
            } else {
                strChainId = "0x" + Integer.toHexString(parsedChainId);
                chain.chainId = strChainId;
                chain.supportedKeyrings = new int[0];
            }
        } catch (NumberFormatException exc) {
            error = true;
            mChainIdEditText.setError(getString(R.string.brave_wallet_add_network_chain_id_error));
        }

        String strChainName = mChainName.getText().toString().trim();
        if (strChainName.isEmpty()) {
            mChainName.setError(getString(R.string.brave_wallet_add_network_chain_empty_error));
            error = true;
        } else {
            chain.chainName = strChainName;
        }

        String strChainCurrencyName = mChainCurrencyName.getText().toString().trim();
        if (strChainCurrencyName.isEmpty()) {
            mChainCurrencyName.setError(
                    getString(R.string.brave_wallet_add_network_chain_empty_error));
            error = true;
        } else {
            chain.symbolName = strChainCurrencyName;
        }

        String strChainCurrencySymbol = mChainCurrencySymbol.getText().toString().trim();
        if (strChainCurrencySymbol.isEmpty()) {
            mChainCurrencySymbol.setError(
                    getString(R.string.brave_wallet_add_network_chain_empty_error));
            error = true;
        } else {
            chain.symbol = strChainCurrencySymbol;
        }

        String strChainCurrencyDecimals = mChainCurrencyDecimals.getText().toString().trim();
        try {
            int iChainCurrencyDecimals = Integer.parseInt(strChainCurrencyDecimals);
            if (iChainCurrencyDecimals > 0) {
                chain.decimals = iChainCurrencyDecimals;
            } else {
                mChainCurrencyDecimals.setError(
                        getString(R.string.brave_wallet_add_network_chain_currency_decimals_error));
                error = true;
            }
        } catch (NumberFormatException numberFormatException) {
            error = true;
            mChainCurrencyDecimals.setError(
                    getString(R.string.brave_wallet_add_network_chain_currency_decimals_error));
        }

        String strRpcUrls = mRpcUrls.getText().toString().trim();
        try {
            URL url = new URL(strRpcUrls);
            if (!isProtocolSupported(url)) {
                mRpcUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));
                error = true;
            } else {
                Url mojourl = new Url();
                mojourl.url = strRpcUrls;
                chain.rpcEndpoints = new Url[] {mojourl};
                chain.activeRpcEndpointIndex = 0;
            }
        } catch (MalformedURLException exc) {
            error = true;
            mRpcUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));
        }

        String strIconUrls = mIconUrls.getText().toString().trim();
        if (!strIconUrls.isEmpty()) {
            try {
                URL url = new URL(strIconUrls);
                if (!isProtocolSupported(url)) {
                    mIconUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));
                    error = true;
                } else {
                    chain.iconUrls = new String[] {strIconUrls};
                }
            } catch (MalformedURLException exc) {
                error = true;
                mIconUrls.setError(getString(R.string.brave_wallet_add_network_urls_error));
            }
        } else {
            chain.iconUrls = new String[0];
        }

        String strBlockExplorerUrls = mBlockExplorerUrls.getText().toString().trim();
        if (!strBlockExplorerUrls.isEmpty()) {
            try {
                URL url = new URL(strBlockExplorerUrls);
                if (!isProtocolSupported(url)) {
                    mBlockExplorerUrls.setError(
                            getString(R.string.brave_wallet_add_network_urls_error));
                    error = true;
                } else {
                    chain.blockExplorerUrls = new String[] {strBlockExplorerUrls};
                }
            } catch (MalformedURLException exc) {
                error = true;
                mBlockExplorerUrls.setError(
                        getString(R.string.brave_wallet_add_network_urls_error));
            }
        } else {
            chain.blockExplorerUrls = new String[0];
        }
        chain.coin = CoinType.ETH;

        if (error) {
            mButtonSubmit.setEnabled(true);
            return;
        }

        assert mJsonRpcService != null;
        if (!TextUtils.isEmpty(mChainId)) {
            if (mChainId.equals(chain.chainId)) {
                mJsonRpcService.removeChain(
                        mChainId,
                        CoinType.ETH,
                        success -> {
                            if (!success) {
                                mButtonSubmit.setEnabled(true);
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
    }

    private void addChain(@NonNull final NetworkInfo chain, final boolean remove) {
        assert mJsonRpcService != null;
        mJsonRpcService.addChain(
                chain,
                (chainId, error, errorMessage) -> {
                    if (error != ProviderError.SUCCESS) {
                        // (sergz): Perhaps we will need to add more errors in the future.
                        // We support only that one for now from backend
                        if (errorMessage.contains("eth_chainId")) {
                            mSubmitError.setText(errorMessage);
                            mSubmitError.setVisibility(View.VISIBLE);
                        }
                        mButtonSubmit.setEnabled(true);
                        return;
                    }
                    if (remove) {
                        mJsonRpcService.removeChain(
                                mChainId,
                                CoinType.ETH,
                                success -> {
                                    // We just do nothing here as we added a chain with a diff
                                    // chainId already
                                    setActivityResult();
                                });
                    } else {
                        setActivityResult();
                    }
                });
    }

    private void setActivityResult() {
        requireActivity().setResult(Activity.RESULT_OK);
        requireActivity().finish();
    }

    /**
     * Returns <code>true</code> if URL protocol is HTTPS, <code>false</code> otherwise.
     *
     * @param url Url whose protocol will be verified.
     * @return <code>true</code> if URL protocol is HTTPS, <code>false</code> otherwise.
     * @noinspection BooleanMethodIsAlwaysInverted
     */
    private boolean isProtocolSupported(@NonNull final URL url) {
        return url.getProtocol().equals("https");
    }
}

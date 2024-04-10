/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding;

import static org.chromium.chrome.browser.crypto_wallet.adapters.OnboardingNetworkSelectorGridAdapter.NETWORK_ITEM_VIEW_TYPE;

import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;

import androidx.annotation.IntegerRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatButton;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.checkbox.MaterialCheckBox;
import com.google.android.material.textfield.TextInputEditText;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.adapters.AutoGridLayoutManager;
import org.chromium.chrome.browser.crypto_wallet.adapters.OnboardingNetworkSelectorGridAdapter;

/** Onboarding fragment showing networks to include before using Brave Wallet. */
public class OnboardingNetworkSelectionFragment extends BaseOnboardingWalletFragment
        implements CompoundButton.OnCheckedChangeListener,
                OnboardingNetworkSelectorGridAdapter.OnNetworkSelectionListener {

    private MaterialCheckBox mShowTestnets;
    private AppCompatButton mContinueButton;
    private RecyclerView mNetworks;
    private TextInputEditText mEditText;
    private OnboardingNetworkSelectorGridAdapter mOnboardingNetworkSelectorGridAdapter;

    private boolean mContinueButtonClicked;
    private int mSelectedNetworks;

    @NonNull
    public static OnboardingNetworkSelectionFragment newInstance() {
        return new OnboardingNetworkSelectionFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContinueButtonClicked = false;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
    }

    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_onboarding_network_selection, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mEditText = view.findViewById(R.id.text_search_networks_edit_text);

        mShowTestnets = view.findViewById(R.id.show_testnets);
        mShowTestnets.setOnCheckedChangeListener(this);

        mNetworks = view.findViewById(R.id.recycler_view_networks);

        final NetworkModel.NetworkLists networkLists = getNetworkLists();
        if (networkLists != null) {
            mOnboardingNetworkSelectorGridAdapter =
                    new OnboardingNetworkSelectorGridAdapter(
                            requireContext(), mShowTestnets.isChecked(), networkLists, this);
            mNetworks.setAdapter(mOnboardingNetworkSelectorGridAdapter);

            @IntegerRes
            int itemWidth =
                    (int)
                            getResources()
                                    .getDimension(R.dimen.onboarding_network_selection_item_width);
            AutoGridLayoutManager layoutManager =
                    new AutoGridLayoutManager(requireContext(), 3, itemWidth);
            layoutManager.setSpanSizeLookup(
                    new GridLayoutManager.SpanSizeLookup() {
                        @Override
                        public int getSpanSize(int position) {
                            int type =
                                    mOnboardingNetworkSelectorGridAdapter.getItemViewType(position);
                            if (type == NETWORK_ITEM_VIEW_TYPE) {
                                return 1;
                            } else {
                                return layoutManager.calculateSpanCount();
                            }
                        }
                    });
            mNetworks.setLayoutManager(layoutManager);

            mEditText.addTextChangedListener(
                    new TextWatcher() {
                        @Override
                        public void beforeTextChanged(
                                CharSequence s, int start, int count, int after) {
                            /* Not used. */
                        }

                        @Override
                        public void onTextChanged(
                                CharSequence s, int start, int before, int count) {
                            mOnboardingNetworkSelectorGridAdapter.filter(s.toString());
                        }

                        @Override
                        public void afterTextChanged(Editable s) {
                            /* Not used. */
                        }
                    });
        }

        mContinueButton = view.findViewById(R.id.continue_button);
        updateSelectedNetworksButton();
        mContinueButton.setOnClickListener(
                v -> {
                    if (mContinueButtonClicked) {
                        return;
                    }
                    mContinueButtonClicked = true;

                    mOnboardingViewModel.setSelectedNetworks(
                            mOnboardingNetworkSelectorGridAdapter.getSelectedNetworks(),
                            mOnboardingNetworkSelectorGridAdapter.getAvailableNetworks());

                    if (mOnNextPage != null) {
                        mOnNextPage.gotoNextPage();
                    }
                });
    }

    private void updateSelectedNetworksButton() {
        String continueWithSelectedNetworks =
                String.format(
                        getResources().getString(R.string.continue_with_networks),
                        mSelectedNetworks);
        mContinueButton.setText(continueWithSelectedNetworks);
    }

    @Nullable
    private NetworkModel.NetworkLists getNetworkLists() {
        final NetworkModel networkModel = getNetworkModel();
        if (networkModel == null) {
            return null;
        }
        return networkModel.mNetworkLists.getValue();
    }

    @Override
    public void onResume() {
        super.onResume();
        mContinueButtonClicked = false;
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (mOnboardingNetworkSelectorGridAdapter != null) {
            mOnboardingNetworkSelectorGridAdapter.showTestNetworks(isChecked);
        }
    }

    @Override
    public void selectedNetworks(int selectedNetworks) {
        mSelectedNetworks = selectedNetworks;
        updateSelectedNetworksButton();
    }
}

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.ONBOARDING_ACTION;
import static org.chromium.chrome.browser.crypto_wallet.util.Utils.RESTORE_WALLET_ACTION;
import static org.chromium.chrome.browser.crypto_wallet.util.Utils.UNLOCK_WALLET_ACTION;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletNativeWorker;
import org.chromium.chrome.browser.crypto_wallet.CryptoWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.CryptoFragmentPageAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.CryptoWalletOnboardingPagerAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.BackupWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.RecoveryPhraseFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.RestoreWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.SecurePasswordFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.SetupWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.UnlockWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.VerifyRecoveryPhraseFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnFinishOnboarding;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;
import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;

import java.util.ArrayList;
import java.util.List;

public class CryptoFragment extends Fragment {
    private OnFinishOnboarding onFinishOnboarding;

    private View rootView;

    private View cryptoLayout;
    private ViewPager cryptoWalletOnboardingViewPager;
    private CryptoWalletOnboardingPagerAdapter cryptoWalletOnboardingPagerAdapter;

    public void setOnLastPageClick(OnFinishOnboarding onFinishOnboarding) {
        this.onFinishOnboarding = onFinishOnboarding;
    }

    public static CryptoFragment newInstance() {
        return new CryptoFragment();
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
        rootView = inflater.inflate(R.layout.fragment_crypto, container, false);
        return rootView;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        cryptoLayout = view.findViewById(R.id.crypto_layout);
        if (CryptoWalletActivity.isOnboardingDone) {
            setCryptoLayout();
        } else {
            cryptoLayout.setVisibility(View.GONE);
            cryptoWalletOnboardingViewPager =
                    view.findViewById(R.id.crypto_wallet_onboarding_viewpager);
            assert getActivity() != null;
            cryptoWalletOnboardingPagerAdapter = new CryptoWalletOnboardingPagerAdapter(
                    ((FragmentActivity) getActivity()).getSupportFragmentManager());
            cryptoWalletOnboardingViewPager.setAdapter(cryptoWalletOnboardingPagerAdapter);
            cryptoWalletOnboardingViewPager.setOffscreenPageLimit(
                    cryptoWalletOnboardingPagerAdapter.getCount() - 1);

            ImageView onboardingBackButton = view.findViewById(R.id.onboarding_back_button);
            onboardingBackButton.setVisibility(View.GONE);
            onboardingBackButton.setOnClickListener(v -> {
                if (cryptoWalletOnboardingViewPager != null) {
                    cryptoWalletOnboardingViewPager.setCurrentItem(
                            cryptoWalletOnboardingViewPager.getCurrentItem() - 1);
                }
            });

            cryptoWalletOnboardingViewPager.addOnPageChangeListener(
                    new ViewPager.OnPageChangeListener() {
                        @Override
                        public void onPageScrolled(
                                int position, float positionOffset, int positionOffsetPixels) {}

                        @Override
                        public void onPageSelected(int position) {
                            if (position == 0 || position == 2) {
                                onboardingBackButton.setVisibility(View.GONE);
                            } else {
                                onboardingBackButton.setVisibility(View.VISIBLE);
                            }
                        }

                        @Override
                        public void onPageScrollStateChanged(int state) {}
                    });
            // TODO below code commented out for logic, it will get updated in follow up issue.

            // if (BraveWalletNativeWorker.getInstance().isWalletLocked()) {
            //     setNavigationFragments(UNLOCK_WALLET_ACTION);
            // } else {
            setNavigationFragments(ONBOARDING_ACTION);
            // }
        }
    }

    private void setNavigationFragments(int type) {
        List<NavigationItem> navigationItems = new ArrayList<>();
        if (type == ONBOARDING_ACTION) {
            SetupWalletFragment setupWalletFragment = new SetupWalletFragment();
            setupWalletFragment.setOnNextPageListener(onNextPage);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.setup_crypto), setupWalletFragment));
            SecurePasswordFragment securePasswordFragment = new SecurePasswordFragment();
            securePasswordFragment.setOnNextPageListener(onNextPage);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.secure_your_crypto), securePasswordFragment));
            BackupWalletFragment backupWalletFragment = new BackupWalletFragment();
            backupWalletFragment.setOnNextPageListener(onNextPage);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.backup_your_wallet), backupWalletFragment));
            RecoveryPhraseFragment recoveryPhraseFragment = new RecoveryPhraseFragment();
            recoveryPhraseFragment.setOnNextPageListener(onNextPage);
            navigationItems.add(
                    new NavigationItem(getResources().getString(R.string.your_recovery_phrase),
                            recoveryPhraseFragment));
            VerifyRecoveryPhraseFragment verifyRecoveryPhraseFragment =
                    new VerifyRecoveryPhraseFragment();
            verifyRecoveryPhraseFragment.setOnNextPageListener(onNextPage);
            navigationItems.add(
                    new NavigationItem(getResources().getString(R.string.verify_recovery_phrase),
                            verifyRecoveryPhraseFragment));
        } else if (type == UNLOCK_WALLET_ACTION) {
            UnlockWalletFragment unlockWalletFragment = new UnlockWalletFragment();
            unlockWalletFragment.setOnNextPageListener(onNextPage);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.unlock_wallet_title), unlockWalletFragment));
        } else if (type == RESTORE_WALLET_ACTION) {
            RestoreWalletFragment restoreWalletFragment = new RestoreWalletFragment();
            restoreWalletFragment.setOnNextPageListener(onNextPage);
            navigationItems.add(
                    new NavigationItem(getResources().getString(R.string.restore_crypto_account),
                            restoreWalletFragment));
        }

        if (cryptoWalletOnboardingPagerAdapter != null) {
            cryptoWalletOnboardingPagerAdapter.setNavigationItems(navigationItems);
            cryptoWalletOnboardingPagerAdapter.notifyDataSetChanged();
        }
    }

    private void setCryptoLayout() {
        View cryptoOnboardingLayout = rootView.findViewById(R.id.crypto_onboarding_layout);
        cryptoOnboardingLayout.setVisibility(View.GONE);
        cryptoLayout.setVisibility(View.VISIBLE);

        ViewPager viewPager = rootView.findViewById(R.id.navigation_view_pager);
        CryptoFragmentPageAdapter adapter =
                new CryptoFragmentPageAdapter(getChildFragmentManager());
        viewPager.setAdapter(adapter);
        viewPager.setOffscreenPageLimit(adapter.getCount() - 1);
        TabLayout tabLayout = rootView.findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(viewPager);
    }

    private final OnNextPage onNextPage = new OnNextPage() {
        @Override
        public void gotoNextPage(boolean finishOnboarding) {
            if (finishOnboarding) {
                onFinishOnboarding.onFinish();
            } else {
                if (cryptoWalletOnboardingViewPager != null)
                    cryptoWalletOnboardingViewPager.setCurrentItem(
                            cryptoWalletOnboardingViewPager.getCurrentItem() + 1);
            }
        }

        @Override
        public void gotoRestorePage() {
            setNavigationFragments(RESTORE_WALLET_ACTION);
        }
    };
}

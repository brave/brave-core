/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.ONBOARDING_ACTION;
import static org.chromium.chrome.browser.crypto_wallet.util.Utils.RESTORE_WALLET_ACTION;
import static org.chromium.chrome.browser.crypto_wallet.util.Utils.UNLOCK_WALLET_ACTION;

import android.app.SearchManager;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;

import androidx.appcompat.widget.SearchView;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.ErcTokenRegistry;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.ERCTokenRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.EthJsonRpcControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.CryptoFragmentPageAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.CryptoWalletOnboardingPagerAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.SwapBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.BackupWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.RecoveryPhraseFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.RestoreWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.SecurePasswordFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.SetupWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.UnlockWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments.VerifyRecoveryPhraseFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;
import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.List;

public class BraveWalletActivity
        extends AsyncInitializationActivity implements OnNextPage, ConnectionErrorHandler {
    private Toolbar toolbar;

    private View cryptoLayout;
    private ImageView swapButton;
    private ViewPager cryptoWalletOnboardingViewPager;
    private CryptoWalletOnboardingPagerAdapter cryptoWalletOnboardingPagerAdapter;
    private KeyringController mKeyringController;
    private ErcTokenRegistry mErcTokenRegistry;
    private EthJsonRpcController mEthJsonRpcController;

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.wallet_search, menu);
        final MenuItem searchItem = menu.findItem(R.id.search);
        SearchView searchView = (SearchView) searchItem.getActionView();

        EditText searchEditText = searchView.findViewById(R.id.search_src_text);
        searchEditText.setTextColor(getResources().getColor(android.R.color.black));
        searchEditText.setHintTextColor(getResources().getColor(android.R.color.darker_gray));
        ImageView closeButtonImage = searchView.findViewById(R.id.search_close_btn);
        closeButtonImage.setImageResource(R.drawable.ic_baseline_close_24);
        SearchManager searchManager = (SearchManager) getSystemService(SEARCH_SERVICE);
        searchView.setSearchableInfo(searchManager.getSearchableInfo(getComponentName()));
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_brave_wallet);
        toolbar = findViewById(R.id.toolbar);
        toolbar.setTitleTextColor(getResources().getColor(android.R.color.black));
        toolbar.setTitle("");
        toolbar.setOverflowIcon(
                ContextCompat.getDrawable(this, R.drawable.ic_baseline_more_vert_24));
        setSupportActionBar(toolbar);

        swapButton = findViewById(R.id.swap_button);
        swapButton.setOnClickListener(v -> {
            SwapBottomSheetDialogFragment swapBottomSheetDialogFragment =
                    SwapBottomSheetDialogFragment.newInstance();
            swapBottomSheetDialogFragment.show(
                    getSupportFragmentManager(), SwapBottomSheetDialogFragment.TAG_FRAGMENT);
        });

        cryptoLayout = findViewById(R.id.crypto_layout);
        cryptoWalletOnboardingViewPager = findViewById(R.id.crypto_wallet_onboarding_viewpager);
        cryptoWalletOnboardingPagerAdapter =
                new CryptoWalletOnboardingPagerAdapter(getSupportFragmentManager());
        cryptoWalletOnboardingViewPager.setAdapter(cryptoWalletOnboardingPagerAdapter);
        cryptoWalletOnboardingViewPager.setOffscreenPageLimit(
                cryptoWalletOnboardingPagerAdapter.getCount() - 1);

        ImageView onboardingBackButton = findViewById(R.id.onboarding_back_button);
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

        onInitialLayoutInflationComplete();
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringController = null;
        mErcTokenRegistry = null;
        InitKeyringController();
        InitErcTokenRegistry();
        InitEthJsonRpcController();
    }

    private void InitKeyringController() {
        if (mKeyringController != null) {
            return;
        }

        mKeyringController = KeyringControllerFactory.getInstance().getKeyringController(this);
    }

    private void InitErcTokenRegistry() {
        if (mErcTokenRegistry != null) {
            return;
        }

        mErcTokenRegistry = ERCTokenRegistryFactory.getInstance().getERCTokenRegistry(this);
    }

    private void InitEthJsonRpcController() {
        if (mEthJsonRpcController != null) {
            return;
        }

        mEthJsonRpcController =
                EthJsonRpcControllerFactory.getInstance().getEthJsonRpcController(this);
    }

    public KeyringController getKeyringController() {
        return mKeyringController;
    }

    public ErcTokenRegistry getErcTokenRegistry() {
        return mErcTokenRegistry;
    }

    public EthJsonRpcController getEthJsonRpcController() {
        return mEthJsonRpcController;
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        InitKeyringController();
        InitErcTokenRegistry();
        InitEthJsonRpcController();
        if (Utils.shouldShowCryptoOnboarding()) {
            setNavigationFragments(ONBOARDING_ACTION);
        } else if (mKeyringController != null) {
            mKeyringController.isLocked(isLocked -> {
                if (isLocked) {
                    setNavigationFragments(UNLOCK_WALLET_ACTION);
                } else {
                    setCryptoLayout();
                }
            });
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mKeyringController != null) {
            mKeyringController.lock();
        }
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    private void setNavigationFragments(int type) {
        List<NavigationItem> navigationItems = new ArrayList<>();
        cryptoLayout.setVisibility(View.GONE);
        if (type == ONBOARDING_ACTION) {
            SetupWalletFragment setupWalletFragment = new SetupWalletFragment();
            setupWalletFragment.setOnNextPageListener(this);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.setup_crypto), setupWalletFragment));
            SecurePasswordFragment securePasswordFragment = new SecurePasswordFragment();
            securePasswordFragment.setOnNextPageListener(this);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.secure_your_crypto), securePasswordFragment));
            BackupWalletFragment backupWalletFragment = new BackupWalletFragment();
            backupWalletFragment.setOnNextPageListener(this);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.backup_your_wallet), backupWalletFragment));
            RecoveryPhraseFragment recoveryPhraseFragment = new RecoveryPhraseFragment();
            recoveryPhraseFragment.setOnNextPageListener(this);
            navigationItems.add(
                    new NavigationItem(getResources().getString(R.string.your_recovery_phrase),
                            recoveryPhraseFragment));
            VerifyRecoveryPhraseFragment verifyRecoveryPhraseFragment =
                    new VerifyRecoveryPhraseFragment();
            verifyRecoveryPhraseFragment.setOnNextPageListener(this);
            navigationItems.add(
                    new NavigationItem(getResources().getString(R.string.verify_recovery_phrase),
                            verifyRecoveryPhraseFragment));
        } else if (type == UNLOCK_WALLET_ACTION) {
            UnlockWalletFragment unlockWalletFragment = new UnlockWalletFragment();
            unlockWalletFragment.setOnNextPageListener(this);
            navigationItems.add(new NavigationItem(
                    getResources().getString(R.string.unlock_wallet_title), unlockWalletFragment));
        } else if (type == RESTORE_WALLET_ACTION) {
            RestoreWalletFragment restoreWalletFragment = new RestoreWalletFragment();
            restoreWalletFragment.setOnNextPageListener(this);
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
        View cryptoOnboardingLayout = findViewById(R.id.crypto_onboarding_layout);
        cryptoOnboardingLayout.setVisibility(View.GONE);
        cryptoLayout.setVisibility(View.VISIBLE);

        ViewPager viewPager = findViewById(R.id.navigation_view_pager);
        CryptoFragmentPageAdapter adapter =
                new CryptoFragmentPageAdapter(getSupportFragmentManager());
        viewPager.setAdapter(adapter);
        viewPager.setOffscreenPageLimit(adapter.getCount() - 1);
        TabLayout tabLayout = findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(viewPager);

        if (swapButton != null) swapButton.setVisibility(View.VISIBLE);
    }

    @Override
    public void gotoNextPage(boolean finishOnboarding) {
        if (finishOnboarding) {
            setCryptoLayout();
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
}

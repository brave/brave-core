/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.appcompat.view.menu.MenuBuilder;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.appbar.MaterialToolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.OnboardingAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.CryptoWalletOnboardingPagerAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.BaseWalletNextPageFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.UnlockWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingBackupWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingCreatingWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingInitWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingNetworkSelectionFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingRecoveryPhraseFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingRestoreWalletFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingSecurePasswordFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingTermsOfUseFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingVerifyRecoveryPhraseFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.settings.BraveWalletPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.modaldialog.AppModalPresenter;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.ArrayList;
import java.util.List;

/**
 * Main Brave Wallet activity
 */
public class BraveWalletActivity extends BraveWalletBaseActivity implements OnNextPage {

    public static final String IS_FROM_DAPPS = "isFromDapps";
    public static final String RESTART_WALLET_ACTIVITY = "restartWalletActivity";
    public static final String RESTART_WALLET_ACTIVITY_SETUP = "restartWalletActivitySetup";
    public static final String RESTART_WALLET_ACTIVITY_RESTORE = "restartWalletActivityRestore";
    public static final String SHOW_WALLET_ACTIVITY_BACKUP = "showWalletActivityBackup";

    private static final String TAG = "BWalletBaseActivity";

    /**
     * Wallet action types used by {@link BraveWalletActivity#setNavigationFragments(WalletAction)},
     * and {@link #replaceNavigationFragments(WalletAction)}.
     */
    public enum WalletAction {
        // Initial onboarding action triggered to create a new Wallet or restore an existing one.
        ONBOARDING,
        // Password creation action part of the onboarding flow, triggered when creating a new
        // Wallet.
        PASSWORD_CREATION,
        // Unlock action type triggered when accessing the locked Wallet.
        UNLOCK,
        // Restore action part of the onboarding flow, triggered when restoring a Wallet.
        ONBOARDING_RESTORE,
        // Restore action triggered outside onboarding flow on a pre-existing wallet.
        RESTORE
    }

    private MaterialToolbar mToolbar;

    private View mCryptoOnboardingLayout;
    private ImageView mOnboardingCloseButton;
    private ImageView mOnboardingBackButton;
    private ViewPager2 mCryptoWalletOnboardingViewPager;
    private ModalDialogManager mModalDialogManager;
    private CryptoWalletOnboardingPagerAdapter mCryptoWalletOnboardingPagerAdapter;
    private boolean mShowBiometricPrompt;
    private boolean mIsFromDapps;
    private WalletModel mWalletModel;
    private boolean mRestartSetupAction;
    private boolean mRestartRestoreAction;
    private boolean mBackupWallet;

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.wallet_search, menu);

        if (menu instanceof MenuBuilder) {
            ((MenuBuilder) menu).setOptionalIconsVisible(true);
        }
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.settings) {
            SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
            settingsLauncher.launchSettingsActivity(this, BraveWalletPreferences.class);
            return true;
        } else if (item.getItemId() == R.id.lock) {
            if (mKeyringService != null) {
                mKeyringService.lock();
            }
        } else if (item.getItemId() == R.id.help_center) {
            WalletUtils.openWalletHelpCenter(this);
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_brave_wallet);
        mIsFromDapps = false;
        final Intent intent = getIntent();
        if (intent != null) {
            mIsFromDapps = intent.getBooleanExtra(IS_FROM_DAPPS, false);
            mRestartSetupAction = intent.getBooleanExtra(RESTART_WALLET_ACTIVITY_SETUP, false);
            mRestartRestoreAction = intent.getBooleanExtra(RESTART_WALLET_ACTIVITY_RESTORE, false);
            mBackupWallet = intent.getBooleanExtra(SHOW_WALLET_ACTIVITY_BACKUP, false);
        }
        mShowBiometricPrompt = true;

        try {
            mWalletModel = BraveActivity.getBraveActivity().getWalletModel();

            // Update network model to use default network.
            getNetworkModel().updateMode(NetworkModel.Mode.WALLET_MODE);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "triggerLayoutInflation", e);
        }

        mCryptoOnboardingLayout = findViewById(R.id.crypto_onboarding_layout);
        mCryptoWalletOnboardingViewPager = findViewById(R.id.crypto_wallet_onboarding_viewpager);
        mCryptoWalletOnboardingViewPager.setUserInputEnabled(false);
        mCryptoWalletOnboardingPagerAdapter = new CryptoWalletOnboardingPagerAdapter(this);
        mCryptoWalletOnboardingViewPager.setAdapter(mCryptoWalletOnboardingPagerAdapter);

        mOnboardingCloseButton = findViewById(R.id.onboarding_close_button);
        mOnboardingCloseButton.setOnClickListener(v -> finish());

        mOnboardingBackButton = findViewById(R.id.onboarding_back_button);
        mOnboardingBackButton.setOnClickListener(
                v -> {
                    if (mCryptoWalletOnboardingViewPager != null
                            && mCryptoWalletOnboardingViewPager.getCurrentItem() > 0) {
                        mCryptoWalletOnboardingViewPager.setCurrentItem(
                                mCryptoWalletOnboardingViewPager.getCurrentItem() - 1);
                    }
                });

        mModalDialogManager = new ModalDialogManager(
                new AppModalPresenter(this), ModalDialogManager.ModalDialogType.APP);

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        if (Utils.shouldShowCryptoOnboarding()) {
            setNavigationFragments(WalletAction.ONBOARDING);
        } else if (mKeyringService != null) {
            mKeyringService.isLocked(
                    isLocked -> {
                        if (isLocked) {
                            setNavigationFragments(WalletAction.UNLOCK);
                        } else if (mBackupWallet) {
                            showBackupSequence();
                        } else {
                            showMainLayout();
                        }
                    });
        }
    }

    @Override
    public void onDestroy() {
        mModalDialogManager.destroy();
        super.onDestroy();
    }

    @Override
    protected ActivityWindowAndroid createWindowAndroid() {
        return new ActivityWindowAndroid(this, true, getIntentRequestTracker()) {
            @Override
            public ModalDialogManager getModalDialogManager() {
                return mModalDialogManager;
            }
        };
    }

    private void setNavigationFragments(@NonNull final WalletAction walletAction) {
        List<BaseWalletNextPageFragment> navigationFragments = new ArrayList<>();
        mShowBiometricPrompt = true;
        mCryptoOnboardingLayout.setVisibility(View.VISIBLE);
        if (walletAction == WalletAction.ONBOARDING) {
            OnboardingInitWalletFragment onboardingInitWalletFragment =
                    new OnboardingInitWalletFragment(mRestartSetupAction, mRestartRestoreAction);
            navigationFragments.add(onboardingInitWalletFragment);
            mBraveWalletP3A.reportOnboardingAction(OnboardingAction.SHOWN);
        } else if (walletAction == WalletAction.UNLOCK) {
            UnlockWalletFragment unlockWalletFragment = new UnlockWalletFragment();
            navigationFragments.add(unlockWalletFragment);
        } else if (walletAction == WalletAction.ONBOARDING_RESTORE) {
            mShowBiometricPrompt = false;
            OnboardingRestoreWalletFragment onboardingRestoreWalletFragment =
                    OnboardingRestoreWalletFragment.newInstance();
            navigationFragments.add(onboardingRestoreWalletFragment);
        }

        if (mCryptoWalletOnboardingPagerAdapter != null) {
            mCryptoWalletOnboardingPagerAdapter.setNavigationItems(navigationFragments);
        }
        addRemoveSecureFlag(true);
    }

    @SuppressLint("NotifyDataSetChanged")
    private void replaceNavigationFragments(@NonNull final WalletAction walletAction) {
        if (mCryptoWalletOnboardingViewPager == null) return;
        if (mCryptoWalletOnboardingPagerAdapter == null) return;

        final List<BaseWalletNextPageFragment> navigationFragments = new ArrayList<>();
        // Terms of use screen is shown only during onboarding actions.
        if (walletAction != WalletAction.RESTORE) {
            final OnboardingTermsOfUseFragment onboardingTermsOfUseFragment =
                    OnboardingTermsOfUseFragment.newInstance();
            navigationFragments.add(onboardingTermsOfUseFragment);
            final OnboardingNetworkSelectionFragment onboardingNetworkSelectionFragment =
                    OnboardingNetworkSelectionFragment.newInstance();
            navigationFragments.add(onboardingNetworkSelectionFragment);
        }

        if (walletAction == WalletAction.ONBOARDING_RESTORE
                || walletAction == WalletAction.RESTORE) {
            mShowBiometricPrompt = false;

            final OnboardingRestoreWalletFragment onboardingRestoreWalletFragment =
                    OnboardingRestoreWalletFragment.newInstance();
            navigationFragments.add(onboardingRestoreWalletFragment);
            addWalletCreationPage(navigationFragments);

        } else if (walletAction == WalletAction.PASSWORD_CREATION) {
            mShowBiometricPrompt = true;

            final OnboardingSecurePasswordFragment onboardingSecurePasswordFragment =
                    new OnboardingSecurePasswordFragment();
            navigationFragments.add(onboardingSecurePasswordFragment);
            addWalletCreationPage(navigationFragments);
            addBackupWalletSequence(navigationFragments, true);
        }
        mCryptoWalletOnboardingPagerAdapter.replaceWithNavigationItems(
                navigationFragments, mCryptoWalletOnboardingViewPager.getCurrentItem() + 1);
        mCryptoWalletOnboardingPagerAdapter.notifyDataSetChanged();

        mCryptoWalletOnboardingViewPager.setCurrentItem(
                mCryptoWalletOnboardingViewPager.getCurrentItem() + 1);
    }

    private void showMainLayout() {
        addRemoveSecureFlag(false);

        mCryptoOnboardingLayout.setVisibility(View.GONE);
        WalletUtils.openWebWallet();
    }

    private void addRemoveSecureFlag(final boolean add) {
        if (add) {
            getWindow().setFlags(
                    WindowManager.LayoutParams.FLAG_SECURE, WindowManager.LayoutParams.FLAG_SECURE);
        } else {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
        }
        try {
            getWindowManager().removeViewImmediate(getWindow().getDecorView());
            getWindowManager().addView(getWindow().getDecorView(), getWindow().getAttributes());
        } catch (IllegalArgumentException exc) {
            // The activity isn't active right now
        }
    }

    private void addBackupWalletSequence(
            @NonNull final List<BaseWalletNextPageFragment> navigationFragments,
            final boolean isOnboarding) {
        OnboardingBackupWalletFragment onboardingBackupWalletFragment =
                OnboardingBackupWalletFragment.newInstance(isOnboarding);
        navigationFragments.add(onboardingBackupWalletFragment);
        OnboardingRecoveryPhraseFragment onboardingRecoveryPhraseFragment =
                OnboardingRecoveryPhraseFragment.newInstance(isOnboarding);
        navigationFragments.add(onboardingRecoveryPhraseFragment);
        OnboardingVerifyRecoveryPhraseFragment onboardingVerifyRecoveryPhraseFragment =
                OnboardingVerifyRecoveryPhraseFragment.newInstance(isOnboarding);
        navigationFragments.add(onboardingVerifyRecoveryPhraseFragment);
    }

    private void addWalletCreationPage(
            @NonNull final List<BaseWalletNextPageFragment> navigationFragments) {
        OnboardingCreatingWalletFragment onboardingCreatingWalletFragment =
                new OnboardingCreatingWalletFragment();
        navigationFragments.add(onboardingCreatingWalletFragment);
    }

    public void showBackupSequence() {
        addRemoveSecureFlag(true);
        mCryptoOnboardingLayout.setVisibility(View.VISIBLE);

        List<BaseWalletNextPageFragment> navigationFragments = new ArrayList<>();
        // We don't need addWalletCreatingPage here, as showOnboardingLayout
        // is invoked only when we didn't back up wallet initially and doing
        // it later from `Backup your crypto wallet` bubble.
        addBackupWalletSequence(navigationFragments, false);

        if (mCryptoWalletOnboardingPagerAdapter != null
                && mCryptoWalletOnboardingViewPager != null) {
            mCryptoWalletOnboardingPagerAdapter.setNavigationItems(navigationFragments);
            mCryptoWalletOnboardingViewPager.setCurrentItem(0);
        }
    }

    @Override
    public boolean showBiometricPrompt() {
        return mShowBiometricPrompt;
    }

    @Override
    public void enableBiometricPrompt() {
        mShowBiometricPrompt = true;
    }

    @Override
    public void gotoNextPage() {
        if (mCryptoWalletOnboardingViewPager != null
                && mCryptoWalletOnboardingViewPager.getAdapter() != null
                && mCryptoWalletOnboardingViewPager.getCurrentItem()
                        < mCryptoWalletOnboardingViewPager.getAdapter().getItemCount() - 1) {
            mCryptoWalletOnboardingViewPager.setCurrentItem(
                    mCryptoWalletOnboardingViewPager.getCurrentItem() + 1);
        }
    }

    @Override
    public void onboardingCompleted() {
        if (mIsFromDapps) {
            finish();
            try {
                BraveActivity activity = BraveActivity.getBraveActivity();
                activity.showWalletPanel(true);
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "gotoNextPage", e);
            }
        } else {
            showMainLayout();
        }
    }

    @Override
    public void gotoCreationPage() {
        replaceNavigationFragments(WalletAction.PASSWORD_CREATION);
        mBraveWalletP3A.reportOnboardingAction(OnboardingAction.LEGAL_AND_PASSWORD);
    }

    @Override
    public void gotoRestorePage(boolean isOnboarding) {
        replaceNavigationFragments(
                isOnboarding ? WalletAction.ONBOARDING_RESTORE : WalletAction.RESTORE);
        if (isOnboarding) {
            mBraveWalletP3A.reportOnboardingAction(OnboardingAction.START_RESTORE);
        }
    }

    @Override
    public void showCloseButton(final boolean show) {
        mOnboardingCloseButton.setVisibility(show ? View.VISIBLE : View.GONE);
    }

    @Override
    public void showBackButton(final boolean show) {
        mOnboardingBackButton.setVisibility(show ? View.VISIBLE : View.GONE);
    }

    @Override
    public void locked() {
        setNavigationFragments(WalletAction.UNLOCK);
    }

    public NetworkModel getNetworkModel() {
        return mWalletModel.getNetworkModel();
    }

    public KeyringModel getKeyringModel() {
        return mWalletModel.getKeyringModel();
    }
}

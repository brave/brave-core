/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletOnboardingPagerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletOnboardingPagerAdapter.WalletAction;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.settings.BraveWalletPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.modaldialog.AppModalPresenter;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.ui.base.ActivityWindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

/** Main Brave Wallet activity */
public class BraveWalletActivity extends BraveWalletBaseActivity implements OnNextPage {

    public static final String IS_FROM_DAPPS = "isFromDapps";
    public static final String RESTART_WALLET_ACTIVITY = "restartWalletActivity";
    public static final String RESTART_WALLET_ACTIVITY_SETUP = "restartWalletActivitySetup";
    public static final String RESTART_WALLET_ACTIVITY_RESTORE = "restartWalletActivityRestore";
    public static final String SHOW_WALLET_ACTIVITY_BACKUP = "showWalletActivityBackup";

    private static final String TAG = "BWalletBaseActivity";

    private MaterialToolbar mToolbar;

    private View mCryptoOnboardingLayout;
    private ImageView mOnboardingCloseButton;
    private ImageView mOnboardingBackButton;
    private ViewPager2 mCryptoWalletOnboardingViewPager;
    private ModalDialogManager mModalDialogManager;
    private WalletOnboardingPagerAdapter mWalletOnboardingPagerAdapter;
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
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
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
        mCryptoWalletOnboardingViewPager.setOffscreenPageLimit(1);
        mCryptoWalletOnboardingViewPager.registerOnPageChangeCallback(
                new ViewPager2.OnPageChangeCallback() {
                    @Override
                    public void onPageSelected(int position) {
                        super.onPageSelected(position);
                        Utils.hideKeyboard(
                                BraveWalletActivity.this,
                                mCryptoWalletOnboardingViewPager.getWindowToken());
                    }
                });

        mOnboardingCloseButton = findViewById(R.id.onboarding_close_button);
        mOnboardingCloseButton.setOnClickListener(v -> finish());

        mOnboardingBackButton = findViewById(R.id.onboarding_back_button);
        mOnboardingBackButton.setOnClickListener(
                v -> {
                    if (mCryptoWalletOnboardingViewPager.getCurrentItem() > 0) {
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
        mWalletOnboardingPagerAdapter =
                new WalletOnboardingPagerAdapter(
                        this, mBraveWalletP3A, mRestartSetupAction, mRestartRestoreAction);
        mCryptoWalletOnboardingViewPager.setAdapter(mWalletOnboardingPagerAdapter);

        if (Utils.shouldShowCryptoOnboarding()) {
            mCryptoOnboardingLayout.setVisibility(View.VISIBLE);
            mWalletOnboardingPagerAdapter.setWalletAction(WalletAction.ONBOARDING);
            mCryptoWalletOnboardingViewPager.setCurrentItem(0);
            addRemoveSecureFlag(true);
        } else if (mKeyringService != null) {
            mKeyringService.isLocked(
                    isLocked -> {
                        if (isLocked) {
                            mCryptoOnboardingLayout.setVisibility(View.VISIBLE);
                            mWalletOnboardingPagerAdapter.setWalletAction(WalletAction.UNLOCK);
                            mCryptoWalletOnboardingViewPager.setCurrentItem(0);
                            addRemoveSecureFlag(true);
                        } else if (mBackupWallet) {
                            showBackupSequence();
                        } else {
                            showMainLayout(false);
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

    private void showMainLayout(final boolean forceNewTab) {
        addRemoveSecureFlag(false);

        mCryptoOnboardingLayout.setVisibility(View.GONE);
        WalletUtils.openWebWallet(forceNewTab);
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

    public void showBackupSequence() {
        addRemoveSecureFlag(true);
        mCryptoOnboardingLayout.setVisibility(View.VISIBLE);
        mWalletOnboardingPagerAdapter.setWalletAction(WalletAction.BACKUP);
        mCryptoWalletOnboardingViewPager.setCurrentItem(0);
    }

    @Override
    public void incrementPages(int pages) {
        if (mCryptoWalletOnboardingViewPager.getAdapter() != null
                && mCryptoWalletOnboardingViewPager.getCurrentItem()
                        < mCryptoWalletOnboardingViewPager.getAdapter().getItemCount() - pages) {
            final boolean smoothScroll = pages == 1;
            mCryptoWalletOnboardingViewPager.setCurrentItem(
                    mCryptoWalletOnboardingViewPager.getCurrentItem() + pages, smoothScroll);
        }
    }

    @Override
    public void showWallet(final boolean forceNewTab) {
        if (mIsFromDapps) {
            finish();
            try {
                BraveActivity activity = BraveActivity.getBraveActivity();
                activity.showWalletPanel(true);
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "onboardingCompleted", e);
            }
        } else {
            showMainLayout(forceNewTab);
        }
    }

    @Override
    public void gotoCreationPage() {
        mWalletOnboardingPagerAdapter.setWalletAction(WalletAction.PASSWORD_CREATION);
        mCryptoWalletOnboardingViewPager.setCurrentItem(
                mCryptoWalletOnboardingViewPager.getCurrentItem() + 1);
    }

    @Override
    public void gotoRestorePage(boolean isOnboarding) {
        mWalletOnboardingPagerAdapter.setWalletAction(
                isOnboarding ? WalletAction.ONBOARDING_RESTORE : WalletAction.RESTORE);
        mCryptoWalletOnboardingViewPager.setCurrentItem(
                mCryptoWalletOnboardingViewPager.getCurrentItem() + 1);
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
        mCryptoOnboardingLayout.setVisibility(View.VISIBLE);
        mWalletOnboardingPagerAdapter.setWalletAction(WalletAction.UNLOCK);
        mCryptoWalletOnboardingViewPager.setCurrentItem(0);
        addRemoveSecureFlag(true);
    }

    public NetworkModel getNetworkModel() {
        return mWalletModel.getNetworkModel();
    }

    public KeyringModel getKeyringModel() {
        return mWalletModel.getKeyringModel();
    }
}

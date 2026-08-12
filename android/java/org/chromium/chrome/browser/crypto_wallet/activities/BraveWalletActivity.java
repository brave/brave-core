/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.lifecycle.ViewModelProvider;
import androidx.viewpager2.widget.ViewPager2;

import org.chromium.base.Log;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletOnboardingPagerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletOnboardingPagerAdapter.WalletAction;
import org.chromium.chrome.browser.crypto_wallet.fragments.onboarding.OnboardingTermsOfUseFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;
import org.chromium.chrome.browser.crypto_wallet.model.OnboardingViewModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.components.browser_ui.modaldialog.AppModalPresenter;
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
    private static final String KEY_WALLET_ACTION = "wallet_action";
    private static final String KEY_PAGER_INDEX = "pager_index";

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
    protected @Nullable Bundle transformSavedInstanceStateForOnCreate(
            @Nullable Bundle savedInstanceState) {
        // Do not let Activity.onCreate() restore the FragmentManager state: the onboarding pager's
        // FragmentStateAdapter uses fragments that depend on native and is set only in
        // finishNativeInitialization, and restoring the fragments before that (and before an
        // adapter
        // exists) crashes the pager.
        // The onboarding flow is instead rebuilt from the action and page persisted in
        // onSaveInstanceState. Kept consistent with onRestoreInstanceState, which also drops the
        // (view) pager state.
        return null;
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        // Drop the restored view state (the pager's adapter state) so it stays consistent with the
        // dropped FragmentManager state above; restoring one without the other crashes the pager's
        // FragmentStateAdapter. Activity does not check the argument for null, so pass an empty
        // bundle.
        super.onRestoreInstanceState(new Bundle());
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
                        // Reset the shared model and uncheck the terms fragment if it's still alive
                        // off screen so it won't load stale selections when shown again.
                        if (mWalletOnboardingPagerAdapter != null
                                && mWalletOnboardingPagerAdapter.isInitWalletFragmentAt(position)) {
                            new ViewModelProvider(BraveWalletActivity.this)
                                    .get(OnboardingViewModel.class)
                                    .reset();
                            final OnboardingTermsOfUseFragment termsOfUseFragment =
                                    mWalletOnboardingPagerAdapter.getTermsOfUseFragment();
                            if (termsOfUseFragment != null) {
                                termsOfUseFragment.uncheckSelections();
                            }
                        }
                        // Keep the keyboard on the unlock screen; hide it when navigating other
                        // onboarding pages.
                        if (mWalletOnboardingPagerAdapter != null
                                && mWalletOnboardingPagerAdapter.getWalletAction()
                                        == WalletAction.UNLOCK) {
                            return;
                        }
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

        mModalDialogManager =
                new ModalDialogManager(
                        new AppModalPresenter(this), ModalDialogManager.ModalDialogType.APP);

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        mWalletOnboardingPagerAdapter =
                new WalletOnboardingPagerAdapter(this, mRestartSetupAction, mRestartRestoreAction);
        mCryptoWalletOnboardingViewPager.setAdapter(mWalletOnboardingPagerAdapter);

        // Rebuild an onboarding flow that was in progress before a recreation (rotation/theme
        // switch). The action and page are persisted in onSaveInstanceState; the fragments
        // themselves are recreated fresh.
        final WalletAction restoredAction = getRestoredWalletAction();
        if (restoredAction != null) {
            mCryptoOnboardingLayout.setVisibility(View.VISIBLE);
            mWalletOnboardingPagerAdapter.setWalletAction(restoredAction);
            mCryptoWalletOnboardingViewPager.setCurrentItem(getRestoredPagerIndex(), false);
            addRemoveSecureFlag(true);
            return;
        }

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
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        // Persist the onboarding action and page so finishNativeInitialization can rebuild the flow
        // after a recreation. Only while the onboarding UI is shown; otherwise leave it out, so a
        // completed or unlocked state is not restored as onboarding. The FragmentManager and pager
        // view state are dropped (see transformSavedInstanceStateForOnCreate and
        // onRestoreInstanceState), so the fragments are recreated fresh.
        if (mCryptoOnboardingLayout != null
                && mCryptoOnboardingLayout.getVisibility() == View.VISIBLE
                && mWalletOnboardingPagerAdapter != null) {
            outState.putSerializable(
                    KEY_WALLET_ACTION, mWalletOnboardingPagerAdapter.getWalletAction());
            outState.putInt(KEY_PAGER_INDEX, mCryptoWalletOnboardingViewPager.getCurrentItem());
        }
    }

    /**
     * Returns the onboarding action that was in progress before a recreation, or {@code null} if
     * none was.
     */
    private @Nullable WalletAction getRestoredWalletAction() {
        final Bundle savedState = getSavedInstanceState();
        if (savedState == null) {
            return null;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return savedState.getSerializable(KEY_WALLET_ACTION, WalletAction.class);
        }
        return (WalletAction) savedState.getSerializable(KEY_WALLET_ACTION);
    }

    /**
     * Returns the onboarding page to restore after a recreation, or {@code 0} if none was saved.
     */
    private int getRestoredPagerIndex() {
        final Bundle savedState = getSavedInstanceState();
        return savedState == null ? 0 : savedState.getInt(KEY_PAGER_INDEX, 0);
    }

    @Override
    public void onDestroy() {
        mModalDialogManager.destroy();
        super.onDestroy();
    }

    @Override
    protected ActivityWindowAndroid createWindowAndroid() {
        return new ActivityWindowAndroid(
                this,
                true,
                getIntentRequestTracker(),
                null,
                /* occlusionTrackingAllowed= */ false) {
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
            getWindow()
                    .setFlags(
                            WindowManager.LayoutParams.FLAG_SECURE,
                            WindowManager.LayoutParams.FLAG_SECURE);
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

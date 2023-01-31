/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app;

import static org.chromium.chrome.browser.app.domain.NetworkSelectorModel.Mode.DEFAULT_WALLET_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity.NETWORK_SELECTOR_MODE;
import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.android.billingclient.api.Purchase;
import com.bumptech.glide.Glide;
import com.bumptech.glide.Priority;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.wireguard.android.backend.GoBackend;
import com.wireguard.android.backend.Tunnel;
import com.wireguard.crypto.KeyPair;

import org.json.JSONException;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.CollectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.UnownedUserDataSupplier;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.SwapService;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ApplicationLifetime;
import org.chromium.chrome.browser.BraveAdFreeCalloutDialogFragment;
import org.chromium.chrome.browser.BraveAdaptiveCaptchaUtils;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveFeatureUtil;
import org.chromium.chrome.browser.BraveHelper;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveSyncInformers;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.CrossPromotionalModalDialogFragment;
import org.chromium.chrome.browser.DormantUsersEngagementDialogFragment;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.app.domain.NetworkSelectorModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.brave_news.BraveNewsUtils;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.brave_stats.BraveStatsBottomSheetDialogFragment;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.browsing_data.BrowsingDataBridge;
import org.chromium.chrome.browser.browsing_data.BrowsingDataType;
import org.chromium.chrome.browser.browsing_data.TimePeriod;
import org.chromium.chrome.browser.compositor.CompositorViewHolder;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.SwapServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.TxServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.custom_layout.popup_window_tooltip.PopupWindowTooltip;
import org.chromium.chrome.browser.dependency_injection.ChromeActivityComponent;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.informers.BraveAndroidSyncDisabledInformer;
import org.chromium.chrome.browser.informers.BraveSyncAccountDeletedInformer;
import org.chromium.chrome.browser.notifications.BraveNotificationWarningDialog;
import org.chromium.chrome.browser.notifications.BravePermissionUtils;
import org.chromium.chrome.browser.notifications.permissions.NotificationPermissionController;
import org.chromium.chrome.browser.notifications.permissions.NotificationPermissionRationaleDialogController;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.ntp.NewTabPageManager;
import org.chromium.chrome.browser.ntp_background_images.util.NewTabPageListener;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.onboarding.v2.HighlightDialogFragment;
import org.chromium.chrome.browser.onboarding.v2.HighlightItem;
import org.chromium.chrome.browser.onboarding.v2.HighlightView;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefChangeRegistrar;
import org.chromium.chrome.browser.preferences.PrefChangeRegistrar.PrefObserver;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.prefetch.settings.PreloadPagesSettingsBridge;
import org.chromium.chrome.browser.prefetch.settings.PreloadPagesState;
import org.chromium.chrome.browser.privacy.settings.BravePrivacySettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.rate.BraveRateDialogFragment;
import org.chromium.chrome.browser.rate.RateUtils;
import org.chromium.chrome.browser.safe_browsing.SafeBrowsingBridge;
import org.chromium.chrome.browser.safe_browsing.SafeBrowsingState;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.set_default_browser.OnBraveSetDefaultBrowserListener;
import org.chromium.chrome.browser.settings.BraveNewsPreferences;
import org.chromium.chrome.browser.settings.BraveNewsPreferencesV2;
import org.chromium.chrome.browser.settings.BraveRewardsPreferences;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.settings.BraveWalletPreferences;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.share.ShareDelegate.ShareOrigin;
import org.chromium.chrome.browser.site_settings.BraveWalletEthereumConnectedSites;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.BraveDbUtil;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.activities.BraveVpnProfileActivity;
import org.chromium.chrome.browser.vpn.fragments.BraveVpnCalloutDialogFragment;
import org.chromium.chrome.browser.vpn.fragments.LinkVpnSubscriptionDialogFragment;
import org.chromium.chrome.browser.vpn.utils.BraveVpnApiResponseUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.utils.InAppPurchaseWrapper;
import org.chromium.chrome.browser.vpn.wireguard.WireguardConfigUtils;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.safe_browsing.BraveSafeBrowsingApiHandler;
import org.chromium.components.safe_browsing.SafeBrowsingApiBridge;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.UiThreadTaskTraits;
import org.chromium.content_public.browser.WebContents;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.permissions.PermissionConstants;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Brave's extension for ChromeActivity
 */
@JNINamespace("chrome::android")
public abstract class BraveActivity<C extends ChromeActivityComponent> extends ChromeActivity
        implements BrowsingDataBridge.OnClearBrowsingDataListener, BraveVpnObserver,
                   OnBraveSetDefaultBrowserListener, ConnectionErrorHandler, PrefObserver,
                   BraveSafeBrowsingApiHandler.BraveSafeBrowsingApiHandlerDelegate {
    public static final String BRAVE_REWARDS_SETTINGS_URL = "brave://rewards/";
    public static final String BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL =
            "brave://rewards/#verify";
    public static final String REWARDS_AC_SETTINGS_URL = "brave://rewards/contribute";
    public static final String REWARDS_LEARN_MORE_URL = "https://brave.com/faq-rewards/#unclaimed-funds";
    public static final String BRAVE_TERMS_PAGE =
            "https://basicattentiontoken.org/user-terms-of-service/";
    public static final String BRAVE_PRIVACY_POLICY = "https://brave.com/privacy/#rewards";
    private static final String PREF_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    private static final String PREF_CLEAR_ON_EXIT = "clear_on_exit";
    public static final String OPEN_URL = "open_url";

    private static final int DAYS_1 = 1;
    private static final int DAYS_4 = 4;
    private static final int DAYS_5 = 5;
    private static final int DAYS_12 = 12;

    /**
     * Settings for sending local notification reminders.
     */
    public static final String CHANNEL_ID = "com.brave.browser";

    // Explicitly declare this variable to avoid build errors.
    // It will be removed in asm and parent variable will be used instead.
    private UnownedUserDataSupplier<BrowserControlsManager> mBrowserControlsManagerSupplier;

    private static final List<String> yandexRegions =
            Arrays.asList("AM", "AZ", "BY", "KG", "KZ", "MD", "RU", "TJ", "TM", "UZ");

    private String mPurchaseToken = "";
    private String mProductId = "";
    private boolean mIsVerification;
    private boolean isDefaultCheckOnResume;
    private boolean isSetDefaultBrowserNotification;
    private BraveWalletService mBraveWalletService;
    private KeyringService mKeyringService;
    private JsonRpcService mJsonRpcService;
    private SwapService mSwapService;
    private WalletModel mWalletModel;
    private BlockchainRegistry mBlockchainRegistry;
    private TxService mTxService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private AssetRatioService mAssetRatioService;
    public CompositorViewHolder compositorView;
    public boolean mLoadedFeed;
    public boolean mComesFromNewTab;
    public CopyOnWriteArrayList<FeedItemsCard> mNewsItemsFeedCards;
    private boolean isProcessingPendingDappsTxRequest;
    private int mLastTabId;
    private boolean mNativeInitialized;
    private boolean mSafeBrowsingFlagEnabled;
    private NewTabPageManager mNewTabPageManager;
    private NotificationPermissionController mNotificationPermissionController;
    private BraveNewsController mBraveNewsController;

    @SuppressLint("VisibleForTests")
    public BraveActivity() {
        // Disable key checker to avoid asserts on Brave keys in debug
        SharedPreferencesManager.getInstance().disableKeyCheckerForTesting();
    }

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        BraveActivityJni.get().restartStatsUpdater();
        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            InAppPurchaseWrapper.getInstance().startBillingServiceConnection(BraveActivity.this);
            BraveVpnNativeWorker.getInstance().addObserver(this);

            BraveVpnUtils.reportBackgroundUsageP3A();
        }
        Profile profile = getCurrentTabModel().getProfile();
        if (profile != null) {
            // Set proper active DSE whenever brave returns to foreground.
            // If active tab is private, set private DSE as an active DSE.
            BraveSearchEngineUtils.updateActiveDSE(profile);
        }

        // The check on mNativeInitialized is mostly to ensure that mojo
        // services for wallet are initialized.
        // TODO(sergz): verify do we need it in that phase or not.
        if (mNativeInitialized) {
            BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
            if (layout == null || !layout.isWalletIconVisible()) {
                return;
            }
            updateWalletBadgeVisibility();
        }

        // We can store a state of that flag as a browser has to be restarted
        // when the flag state is changed in any case
        mSafeBrowsingFlagEnabled =
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ANDROID_SAFE_BROWSING);

        executeInitSafeBrowsing(0);
    }

    @Override
    public void onPauseWithNative() {
        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            BraveVpnNativeWorker.getInstance().removeObserver(this);
        }
        Profile profile = getCurrentTabModel().getProfile();
        if (profile != null && profile.isOffTheRecord()) {
            // Set normal DSE as an active DSE when brave goes in background
            // because currently set DSE is used by outside of brave(ex, brave search widget).
            BraveSearchEngineUtils.updateActiveDSE(profile);
        }
        super.onPauseWithNative();
    }

    private NewTabPageListener newTabPageListener;

    @Override
    public boolean onMenuOrKeyboardAction(int id, boolean fromMenu) {
        final TabImpl currentTab = (TabImpl) getActivityTab();
        // Handle items replaced by Brave.
        if (id == R.id.info_menu_id && currentTab != null) {
            ShareDelegate shareDelegate = (ShareDelegate) getShareDelegateSupplier().get();
            shareDelegate.share(currentTab, false, ShareOrigin.OVERFLOW_MENU);
            return true;
        } else if (id == R.id.reload_menu_id) {
            setComesFromNewTab(true);
        }

        if (super.onMenuOrKeyboardAction(id, fromMenu)) {
            return true;
        }

        // Handle items added by Brave.
        if (currentTab == null) {
            return false;
        } else if (id == R.id.exit_id) {
            ApplicationLifetime.terminate(false);
        } else if (id == R.id.set_default_browser) {
            BraveSetDefaultBrowserUtils.showBraveSetDefaultBrowserDialog(BraveActivity.this, true);
        } else if (id == R.id.brave_rewards_id) {
            openNewOrSelectExistingTab(BRAVE_REWARDS_SETTINGS_URL);
        } else if (id == R.id.brave_wallet_id) {
            openBraveWallet(false, false, false);
        } else if (id == R.id.brave_news_id) {
            openBraveNewsSettings();
        } else if (id == R.id.request_brave_vpn_id || id == R.id.request_brave_vpn_check_id) {
            if (!InternetConnection.isNetworkAvailable(BraveActivity.this)) {
                Toast.makeText(BraveActivity.this, R.string.no_internet, Toast.LENGTH_SHORT).show();
            } else {
                if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(BraveActivity.this)) {
                    BraveVpnUtils.showProgressDialog(BraveActivity.this,
                            getResources().getString(R.string.vpn_disconnect_text));
                    BraveVpnProfileUtils.getInstance().stopVpn(BraveActivity.this);
                    BraveVpnUtils.dismissProgressDialog();
                } else {
                    if (BraveVpnNativeWorker.getInstance().isPurchasedUser()) {
                        BraveVpnPrefUtils.setSubscriptionPurchase(true);
                        if (WireguardConfigUtils.isConfigExist(BraveActivity.this)) {
                            BraveVpnProfileUtils.getInstance().startVpn(BraveActivity.this);
                        } else {
                            BraveVpnUtils.openBraveVpnProfileActivity(BraveActivity.this);
                        }
                    } else {
                        BraveVpnUtils.showProgressDialog(BraveActivity.this,
                                getResources().getString(R.string.vpn_connect_text));
                        if (BraveVpnPrefUtils.isSubscriptionPurchase()) {
                            verifySubscription();
                        } else {
                            BraveVpnUtils.dismissProgressDialog();
                            BraveVpnUtils.openBraveVpnPlansActivity(BraveActivity.this);
                        }
                    }
                }
            }
        } else {
            return false;
        }
        return true;
    }

    private void cleanUpBraveNewsController() {
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
        mBraveNewsController = null;
    }

    @Override
    public void onConnectionError(MojoException e) {
        cleanUpNativeServices();
        initNativeServices();

        cleanUpBraveNewsController();
        initBraveNewsController();
    }

    @Override
    protected void onDestroyInternal() {
        if (mNotificationPermissionController != null) {
            NotificationPermissionController.detach(mNotificationPermissionController);
            mNotificationPermissionController = null;
        }
        BraveSafeBrowsingApiHandler.getInstance().shutdownSafeBrowsing();
        super.onDestroyInternal();
        cleanUpBraveNewsController();
        cleanUpNativeServices();
    }

    public WalletModel getWalletModel() {
        return mWalletModel;
    }

    private void maybeHasPendingUnlockRequest() {
        assert mKeyringService != null;
        mKeyringService.hasPendingUnlockRequest(pending -> {
            if (pending) {
                BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
                if (layout != null) {
                    layout.showWalletPanel();
                }

                return;
            }
            maybeShowPendingTransactions();
            maybeShowSignTxRequestLayout();
        });
    }

    private void setWalletBadgeVisibility(boolean visibile) {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.updateWalletBadgeVisibility(visibile);
        }
    }

    private void maybeShowPendingTransactions() {
        assert mWalletModel != null;
        // trigger to observer to refresh data to process the pending request
        mWalletModel.getCryptoModel().refreshTransactions();
    }

    private void maybeShowSignTxRequestLayout() {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingSignTransactionRequests(requests -> {
            if (requests != null && requests.length != 0) {
                openBraveWalletDAppsActivity(
                        BraveWalletDAppsActivity.ActivityType.SIGN_TRANSACTION);
                return;
            }
            maybeShowSignAllTxRequestLayout();
        });
    }

    private void maybeShowSignAllTxRequestLayout() {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingSignAllTransactionsRequests(requests -> {
            if (requests != null && requests.length != 0) {
                openBraveWalletDAppsActivity(
                        BraveWalletDAppsActivity.ActivityType.SIGN_ALL_TRANSACTIONS);
                return;
            }
            maybeShowSignMessageRequestLayout();
        });
    }

    private void maybeShowSignMessageRequestLayout() {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingSignMessageRequests(requests -> {
            if (requests != null && requests.length != 0) {
                openBraveWalletDAppsActivity(BraveWalletDAppsActivity.ActivityType.SIGN_MESSAGE);

                return;
            }
            maybeShowChainRequestLayout();
        });
    }

    private void maybeShowChainRequestLayout() {
        assert mJsonRpcService != null;
        mJsonRpcService.getPendingAddChainRequests(networks -> {
            if (networks != null && networks.length != 0) {
                openBraveWalletDAppsActivity(
                        BraveWalletDAppsActivity.ActivityType.ADD_ETHEREUM_CHAIN);

                return;
            }
            maybeShowSwitchChainRequestLayout();
        });
    }

    private void maybeShowSwitchChainRequestLayout() {
        assert mJsonRpcService != null;
        mJsonRpcService.getPendingSwitchChainRequests(requests -> {
            if (requests != null && requests.length != 0) {
                openBraveWalletDAppsActivity(
                        BraveWalletDAppsActivity.ActivityType.SWITCH_ETHEREUM_CHAIN);

                return;
            }
            maybeShowAddSuggestTokenRequestLayout();
        });
    }

    private void maybeShowAddSuggestTokenRequestLayout() {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingAddSuggestTokenRequests(requests -> {
            if (requests != null && requests.length != 0) {
                openBraveWalletDAppsActivity(BraveWalletDAppsActivity.ActivityType.ADD_TOKEN);

                return;
            }
            maybeShowGetEncryptionPublicKeyRequestLayout();
        });
    }

    private void maybeShowGetEncryptionPublicKeyRequestLayout() {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(requests -> {
            if (requests != null && requests.length != 0) {
                openBraveWalletDAppsActivity(
                        BraveWalletDAppsActivity.ActivityType.GET_ENCRYPTION_PUBLIC_KEY_REQUEST);

                return;
            }
            maybeShowDecryptRequestLayout();
        });
    }

    private void maybeShowDecryptRequestLayout() {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingDecryptRequests(requests -> {
            if (requests != null && requests.length != 0) {
                openBraveWalletDAppsActivity(BraveWalletDAppsActivity.ActivityType.DECRYPT_REQUEST);

                return;
            }
            BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
            if (layout != null) {
                layout.showWalletPanel();
            }
        });
    }

    public void dismissWalletPanelOrDialog() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.dismissWalletPanelOrDialog();
        }
    }

    public void showWalletPanel(boolean ignoreWeb3NotificationPreference) {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.showWalletIcon(true);
        }
        if (!ignoreWeb3NotificationPreference
                && !BraveWalletPreferences.getPrefWeb3NotificationsEnabled()) {
            return;
        }
        assert mKeyringService != null;
        mKeyringService.isLocked(locked -> {
            if (locked) {
                layout.showWalletPanel();
                return;
            }
            maybeHasPendingUnlockRequest();
        });
    }

    public void showWalletOnboarding() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.showWalletIcon(true);
            if (!BraveWalletPreferences.getPrefWeb3NotificationsEnabled()) {
                return;
            }
            layout.showWalletPanel();
        }
    }

    public void walletInteractionDetected(WebContents webContents) {
        Tab tab = getActivityTab();
        if (tab == null
                || !webContents.getLastCommittedUrl().equals(
                        tab.getWebContents().getLastCommittedUrl())) {
            return;
        }
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.showWalletIcon(true);
            updateWalletBadgeVisibility();
        }
    }

    public void showAccountCreation(String keyringId) {
        assert mWalletModel != null : " mWalletModel is null ";
        mWalletModel.getDappsModel().addAccountCreationRequest(keyringId);
    }

    private void updateWalletBadgeVisibility() {
        assert mWalletModel != null;
        mWalletModel.getDappsModel().updateWalletBadgeVisibility();
    }

    private void verifySubscription() {
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases != null && purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            mPurchaseToken = purchase.getPurchaseToken();
            mProductId = purchase.getSkus().get(0).toString();
            BraveVpnNativeWorker.getInstance().verifyPurchaseToken(mPurchaseToken, mProductId,
                    BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
        } else {
            BraveVpnApiResponseUtils.queryPurchaseFailed(BraveActivity.this);
            if (!mIsVerification) {
                BraveVpnUtils.openBraveVpnPlansActivity(BraveActivity.this);
            }
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess) {
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            int paymentState = BraveVpnUtils.getPaymentState(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setPurchaseToken(mPurchaseToken);
                BraveVpnPrefUtils.setProductId(mProductId);
                BraveVpnPrefUtils.setPurchaseExpiry(purchaseExpiry);
                BraveVpnPrefUtils.setSubscriptionPurchase(true);
                BraveVpnPrefUtils.setPaymentState(paymentState);
                if (BraveVpnPrefUtils.isResetConfiguration()) {
                    BraveVpnUtils.dismissProgressDialog();
                    BraveVpnUtils.openBraveVpnProfileActivity(BraveActivity.this);
                } else {
                    if (!mIsVerification) {
                        checkForVpn();
                    } else {
                        mIsVerification = false;
                        if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(
                                    BraveActivity.this)
                                && !TextUtils.isEmpty(BraveVpnPrefUtils.getHostname())
                                && !TextUtils.isEmpty(BraveVpnPrefUtils.getClientId())
                                && !TextUtils.isEmpty(BraveVpnPrefUtils.getSubscriberCredential())
                                && !TextUtils.isEmpty(BraveVpnPrefUtils.getApiAuthToken())) {
                            BraveVpnNativeWorker.getInstance().verifyCredentials(
                                    BraveVpnPrefUtils.getHostname(),
                                    BraveVpnPrefUtils.getClientId(),
                                    BraveVpnPrefUtils.getSubscriberCredential(),
                                    BraveVpnPrefUtils.getApiAuthToken());
                        }
                    }
                    BraveVpnUtils.dismissProgressDialog();
                }
            } else {
                BraveVpnApiResponseUtils.queryPurchaseFailed(BraveActivity.this);
                if (!mIsVerification) {
                    BraveVpnUtils.openBraveVpnPlansActivity(BraveActivity.this);
                }
                mIsVerification = false;
            }
            mPurchaseToken = "";
            mProductId = "";
        } else {
            BraveVpnApiResponseUtils.queryPurchaseFailed(BraveActivity.this);
            if (!mIsVerification) {
                BraveVpnUtils.openBraveVpnPlansActivity(BraveActivity.this);
            }
            mIsVerification = false;
        }
    };

    private void checkForVpn() {
        BraveVpnNativeWorker.getInstance().reportForegroundP3A();
        new Thread() {
            @Override
            public void run() {
                Intent intent = GoBackend.VpnService.prepare(BraveActivity.this);
                if (intent != null
                        || !WireguardConfigUtils.isConfigExist(getApplicationContext())) {
                    BraveVpnUtils.dismissProgressDialog();
                    BraveVpnUtils.openBraveVpnProfileActivity(BraveActivity.this);
                    return;
                }
                BraveVpnProfileUtils.getInstance().startVpn(BraveActivity.this);
            }
        }.start();
    }

    @Override
    public void onVerifyCredentials(String jsonVerifyCredentials, boolean isSuccess) {
        if (!isSuccess) {
            if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(BraveActivity.this)) {
                BraveVpnProfileUtils.getInstance().stopVpn(BraveActivity.this);
            }
            Intent braveVpnProfileIntent =
                    new Intent(BraveActivity.this, BraveVpnProfileActivity.class);
            braveVpnProfileIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            braveVpnProfileIntent.putExtra(BraveVpnUtils.VERIFY_CREDENTIALS_FAILED, true);
            startActivity(braveVpnProfileIntent);
        }
    }

    @Override
    public void initializeState() {
        super.initializeState();
        if (isNoRestoreState()) {
            CommandLine.getInstance().appendSwitch(ChromeSwitches.NO_RESTORE_STATE);
        }

        if (isClearBrowsingDataOnExit()) {
            List<Integer> dataTypes = Arrays.asList(
                    BrowsingDataType.HISTORY, BrowsingDataType.COOKIES, BrowsingDataType.CACHE);

            int[] dataTypesArray = CollectionUtil.integerListToIntArray(new ArrayList<>(dataTypes));

            // has onBrowsingDataCleared() as an @Override callback from implementing
            // BrowsingDataBridge.OnClearBrowsingDataListener
            BrowsingDataBridge.getInstance().clearBrowsingData(
                    this, dataTypesArray, TimePeriod.ALL_TIME);
        }

        setLoadedFeed(false);
        setComesFromNewTab(false);
        setNewsItemsFeedCards(null);
        BraveSearchEngineUtils.initializeBraveSearchEngineStates(getTabModelSelector());
        Intent intent = getIntent();
        if (intent != null && intent.getBooleanExtra(Utils.RESTART_WALLET_ACTIVITY, false)) {
            openBraveWallet(false,
                    intent.getBooleanExtra(Utils.RESTART_WALLET_ACTIVITY_SETUP, false),
                    intent.getBooleanExtra(Utils.RESTART_WALLET_ACTIVITY_RESTORE, false));
        }
    }

    public int getLastTabId() {
        return mLastTabId;
    }

    public void setLastTabId(int lastTabId) {
        this.mLastTabId = lastTabId;
    }

    public boolean isLoadedFeed() {
        return mLoadedFeed;
    }

    public void setLoadedFeed(boolean loadedFeed) {
        this.mLoadedFeed = loadedFeed;
    }

    public CopyOnWriteArrayList<FeedItemsCard> getNewsItemsFeedCards() {
        return mNewsItemsFeedCards;
    }

    public void setNewsItemsFeedCards(CopyOnWriteArrayList<FeedItemsCard> newsItemsFeedCards) {
        this.mNewsItemsFeedCards = newsItemsFeedCards;
    }

    public void setComesFromNewTab(boolean comesFromNewTab) {
        this.mComesFromNewTab = comesFromNewTab;
    }

    public boolean isComesFromNewTab() {
        return mComesFromNewTab;
    }

    @Override
    public void onBrowsingDataCleared() {}

    @Override
    public void OnCheckDefaultResume() {
        isDefaultCheckOnResume = true;
    }

    @Override
    public void onResume() {
        super.onResume();
        isProcessingPendingDappsTxRequest = false;
        if (isDefaultCheckOnResume) {
            isDefaultCheckOnResume = false;

            if (BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(this)) {
                BraveSetDefaultBrowserUtils.setBraveDefaultSuccess();
            }
        }

        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> { BraveStatsUtil.removeShareStatsFile(); });
        BraveSafeBrowsingApiHandler.getInstance().setDelegate(
                BraveConfig.SAFEBROWSING_API_KEY, this);
    }

    @Override
    public void performPostInflationStartup() {
        super.performPostInflationStartup();

        createNotificationChannel();
    }

    @Override
    protected void initializeStartupMetrics() {
        super.initializeStartupMetrics();

        // Disable FRE for arm64 builds where ChromeActivity is the one that
        // triggers FRE instead of ChromeLauncherActivity on arm32 build.
        BraveHelper.DisableFREDRP();
    }

    @Override
    public void onPreferenceChange() {
        maybeSolveAdaptiveCaptcha();
    }

    @Override
    public void turnSafeBrowsingOff() {
        SafeBrowsingBridge.setSafeBrowsingState(SafeBrowsingState.NO_SAFE_BROWSING);
    }

    @Override
    public boolean isSafeBrowsingEnabled() {
        return mSafeBrowsingFlagEnabled;
    }

    @Override
    public Activity getActivity() {
        return this;
    }

    public void maybeSolveAdaptiveCaptcha() {
        String captchaID = UserPrefs.get(Profile.getLastUsedRegularProfile())
                                   .getString(BravePref.SCHEDULED_CAPTCHA_ID);
        String paymentID = UserPrefs.get(Profile.getLastUsedRegularProfile())
                                   .getString(BravePref.SCHEDULED_CAPTCHA_PAYMENT_ID);
        if (!TextUtils.isEmpty(captchaID) && !TextUtils.isEmpty(paymentID)
                && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()) {
            BraveAdaptiveCaptchaUtils.solveCaptcha(captchaID, paymentID);
        }
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        BraveVpnNativeWorker.getInstance().reloadPurchasedState();

        BraveHelper.maybeMigrateSettings();
        BraveHelper.cacheNativeFeatures();

        PrefChangeRegistrar mPrefChangeRegistrar = new PrefChangeRegistrar();
        mPrefChangeRegistrar.addObserver(BravePref.SCHEDULED_CAPTCHA_ID, this);

        if (UserPrefs.get(Profile.getLastUsedRegularProfile())
                        .getInteger(BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS)
                >= 10) {
            UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .setBoolean(BravePref.SCHEDULED_CAPTCHA_PAUSED, true);
        }

        Log.e("adaptive captcha",
                "Failed attempts : "
                        + UserPrefs.get(Profile.getLastUsedRegularProfile())
                                  .getInteger(BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS));
        if (!UserPrefs.get(Profile.getLastUsedRegularProfile())
                        .getBoolean(BravePref.SCHEDULED_CAPTCHA_PAUSED)) {
            maybeSolveAdaptiveCaptcha();
        }

        if (SharedPreferencesManager.getInstance().readBoolean(
                    BravePreferenceKeys.BRAVE_DOUBLE_RESTART, false)) {
            SharedPreferencesManager.getInstance().writeBoolean(
                    BravePreferenceKeys.BRAVE_DOUBLE_RESTART, false);
            BraveRelaunchUtils.restart();
            return;
        }

        // Make sure this option is disabled
        if (PreloadPagesSettingsBridge.getState() != PreloadPagesState.NO_PRELOADING) {
            PreloadPagesSettingsBridge.setState(PreloadPagesState.NO_PRELOADING);
        }

        if (BraveRewardsHelper.hasRewardsEnvChange()) {
            BravePrefServiceBridge.getInstance().resetPromotionLastFetchStamp();
            BraveRewardsHelper.setRewardsEnvChange(false);
        }

        int appOpenCount = SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT);
        SharedPreferencesManager.getInstance().writeInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT, appOpenCount + 1);

        if (PackageUtils.isFirstInstall(this) && appOpenCount == 0) {
            checkForYandexSE();
        }

        //set bg ads to off for existing and new installations
        setBgBraveAdsDefaultOff();
        migrateBgPlaybackToFeature();

        Context app = ContextUtils.getApplicationContext();
        if (null != app
                && BraveReflectionUtil.EqualTypes(this.getClass(), ChromeTabbedActivity.class)) {
            // Trigger BraveSyncWorker CTOR to make migration from sync v1 if sync is enabled
            BraveSyncWorker.get();
        }

        checkAndshowNotificationWarningDialog();

        if (RateUtils.getInstance().isLastSessionShown()) {
            RateUtils.getInstance().setPrefNextRateDate();
            RateUtils.getInstance().setLastSessionShown(false);
        }

        if (!RateUtils.getInstance().getPrefRateEnabled()) {
            RateUtils.getInstance().setPrefRateEnabled(true);
            RateUtils.getInstance().setPrefNextRateDate();
        }
        RateUtils.getInstance().setTodayDate();

        if (RateUtils.getInstance().shouldShowRateDialog(this)) {
            showBraveRateDialog();
            RateUtils.getInstance().setLastSessionShown(true);
        }

        // TODO commenting out below code as we may use it in next release

        // if (PackageUtils.isFirstInstall(this)
        //         &&
        //         SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
        //         == 1) {
        //     Calendar calender = Calendar.getInstance();
        //     calender.setTime(new Date());
        //     calender.add(Calendar.DATE, DAYS_4);
        //     OnboardingPrefManager.getInstance().setNextOnboardingDate(
        //         calender.getTimeInMillis());
        // }

        // OnboardingActivity onboardingActivity = null;
        // for (Activity ref : ApplicationStatus.getRunningActivities()) {
        //     if (!(ref instanceof OnboardingActivity)) continue;

        //     onboardingActivity = (OnboardingActivity) ref;
        // }

        // if (onboardingActivity == null
        //         && OnboardingPrefManager.getInstance().showOnboardingForSkip(this)) {
        //     OnboardingPrefManager.getInstance().showOnboarding(this);
        //     OnboardingPrefManager.getInstance().setOnboardingShownForSkip(true);
        // }

        if (SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT) == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_12);
            OnboardingPrefManager.getInstance().setNextCrossPromoModalDate(
                    calender.getTimeInMillis());
        }

        if (OnboardingPrefManager.getInstance().showCrossPromoModal()) {
            showCrossPromotionalDialog();
            OnboardingPrefManager.getInstance().setCrossPromoModalShown(true);
        }
        BraveSyncInformers.show();
        BraveAndroidSyncDisabledInformer.showInformers();
        BraveSyncAccountDeletedInformer.show();

        if (!OnboardingPrefManager.getInstance().isOneTimeNotificationStarted()
                && PackageUtils.isFirstInstall(this)) {
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.HOUR_3);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.HOUR_24);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_6);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_10);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_30);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DAY_35);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_1);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_2);
            RetentionNotificationUtil.scheduleNotification(this, RetentionNotificationUtil.DEFAULT_BROWSER_3);
            OnboardingPrefManager.getInstance().setOneTimeNotificationStarted(true);
        }

        if (PackageUtils.isFirstInstall(this)
                && SharedPreferencesManager.getInstance().readInt(
                           BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                        == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_4);
            BraveRewardsHelper.setNextRewardsOnboardingModalDate(calender.getTimeInMillis());
        }

        if (!isSetDefaultBrowserNotification) {
            BraveSetDefaultBrowserUtils.checkSetDefaultBrowserModal(this);
        }

        checkFingerPrintingOnUpgrade();
        compositorView = null;

        String countryCode = Locale.getDefault().getCountry();

        if (!countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)
                && BraveVpnUtils.isBraveVpnFeatureEnable()
                && InAppPurchaseWrapper.getInstance().isSubscriptionSupported()) {
            if (BraveVpnPrefUtils.shouldShowCallout() && !BraveVpnPrefUtils.isSubscriptionPurchase()
                            && (SharedPreferencesManager.getInstance().readInt(
                                        BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                                            == 1
                                    && !PackageUtils.isFirstInstall(this))
                    || (SharedPreferencesManager.getInstance().readInt(
                                BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                                    == 7
                            && PackageUtils.isFirstInstall(this))) {
                showVpnCalloutDialog();
            }

            if (!TextUtils.isEmpty(BraveVpnPrefUtils.getPurchaseToken())
                    && !TextUtils.isEmpty(BraveVpnPrefUtils.getProductId())) {
                mIsVerification = true;
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                        BraveVpnPrefUtils.getPurchaseToken(), BraveVpnPrefUtils.getProductId(),
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
            }
        }
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_VPN_LINK_SUBSCRIPTION_ANDROID_UI)
                && BraveVpnPrefUtils.isSubscriptionPurchase()
                && !BraveVpnPrefUtils.isLinkSubscriptionDialogShown()) {
            showLinkVpnSubscriptionDialog();
        }
        if (PackageUtils.isFirstInstall(this)
                && (OnboardingPrefManager.getInstance().isDormantUsersEngagementEnabled()
                        || getPackageName().equals(BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME))) {
            OnboardingPrefManager.getInstance().setDormantUsersPrefs();
            if (!OnboardingPrefManager.getInstance().isDormantUsersNotificationsStarted()) {
                RetentionNotificationUtil.scheduleDormantUsersNotifications(this);
                OnboardingPrefManager.getInstance().setDormantUsersNotificationsStarted(true);
            }
        }
        initNativeServices();

        if (OnboardingPrefManager.getInstance().isOnboardingSearchBoxTooltip()) {
            showSearchBoxTooltip();
        }
        mNativeInitialized = true;

        if (countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)
                && SharedPreferencesManager.getInstance().readBoolean(
                        BravePreferenceKeys.BRAVE_AD_FREE_CALLOUT_DIALOG, true)
                && getActivityTab() != null && getActivityTab().getUrl().getSpec() != null
                && UrlUtilities.isNTPUrl(getActivityTab().getUrl().getSpec())
                && (SharedPreferencesManager.getInstance().readBoolean(
                            BravePreferenceKeys.BRAVE_OPENED_YOUTUBE, false)
                        || SharedPreferencesManager.getInstance().readInt(
                                   BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                                >= 7)) {
            showAdFreeCalloutDialog();
        }

        initBraveNewsController();
    }

    public void initBraveNewsController() {
        if (mBraveNewsController != null) {
            return;
        }

        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS_V2)
                && BravePrefServiceBridge.getInstance().getShowNews()
                && BravePrefServiceBridge.getInstance().getNewsOptIn()) {
            mBraveNewsController =
                    BraveNewsControllerFactory.getInstance().getBraveNewsController(this);

            BraveNewsUtils.getBraveNewsSettingsData(mBraveNewsController, null);
        }
    }

    private void migrateBgPlaybackToFeature() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        if (sharedPreferences.getBoolean(
                    BravePreferenceKeys.BRAVE_BACKGROUND_VIDEO_PLAYBACK_CONVERTED_TO_FEATURE,
                    false)) {
            if (BravePrefServiceBridge.getInstance().getBackgroundVideoPlaybackEnabled()
                    && ChromeFeatureList.isEnabled(
                            BraveFeatureList.BRAVE_BACKGROUND_VIDEO_PLAYBACK)) {
                BravePrefServiceBridge.getInstance().setBackgroundVideoPlaybackEnabled(false);
            }
            return;
        }
        if (BravePrefServiceBridge.getInstance().getBackgroundVideoPlaybackEnabled()) {
            BraveFeatureUtil.enableFeature(
                    BraveFeatureList.BRAVE_BACKGROUND_VIDEO_PLAYBACK_INTERNAL, true, true);
        }
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(
                BravePreferenceKeys.BRAVE_BACKGROUND_VIDEO_PLAYBACK_CONVERTED_TO_FEATURE, true);
        sharedPreferencesEditor.apply();
    }

    private void showSearchBoxTooltip() {
        OnboardingPrefManager.getInstance().setOnboardingSearchBoxTooltip(false);

        HighlightView highlightView = new HighlightView(this, null);
        highlightView.setColor(
                ContextCompat.getColor(this, R.color.onboarding_search_highlight_color));
        ViewGroup viewGroup = findViewById(android.R.id.content);
        View anchorView = (View) findViewById(R.id.toolbar);
        float padding = (float) dpToPx(this, 20);
        boolean isTablet = ConfigurationUtils.isTablet(this);
        new Handler().postDelayed(() -> {
            PopupWindowTooltip popupWindowTooltip =
                    new PopupWindowTooltip.Builder(this)
                            .anchorView(anchorView)
                            .arrowColor(getResources().getColor(R.color.onboarding_arrow_color))
                            .gravity(Gravity.BOTTOM)
                            .dismissOnOutsideTouch(true)
                            .dismissOnInsideTouch(false)
                            .backgroundDimDisabled(true)
                            .contentArrowAtStart(!isTablet)
                            .padding(padding)
                            .parentPaddingHorizontal(dpToPx(this, 10))
                            .onDismissListener(tooltip -> {
                                if (viewGroup != null && highlightView != null) {
                                    viewGroup.removeView(highlightView);
                                }
                                maybeShowNotificationPermissionRetionale();
                            })
                            .modal(true)
                            .contentView(R.layout.brave_onboarding_searchbox)
                            .build();

            String countryCode = Locale.getDefault().getCountry();
            if (countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)) {
                TextView toolTipBody = popupWindowTooltip.findViewById(R.id.tv_tooltip_title);
                toolTipBody.setText(getResources().getString(R.string.searchbox_onboarding_india));
            }
            viewGroup.addView(highlightView);
            HighlightItem item = new HighlightItem(anchorView);
            highlightView.setHighlightTransparent(true);
            highlightView.setHighlightItem(item);
            popupWindowTooltip.show();
        }, 500);
    }

    private void maybeShowNotificationPermissionRetionale() {
        NotificationPermissionController mNotificationPermissionController =
                new NotificationPermissionController(getWindowAndroid(),
                        new NotificationPermissionRationaleDialogController(
                                this, getModalDialogManager()));
        NotificationPermissionController.attach(
                getWindowAndroid(), mNotificationPermissionController);
        mNotificationPermissionController.requestPermissionIfNeeded(false /* contextual */);
    }

    public void setDormantUsersPrefs() {
        OnboardingPrefManager.getInstance().setDormantUsersPrefs();
        RetentionNotificationUtil.scheduleDormantUsersNotifications(this);
    }

    private void showVpnCalloutDialog() {
        BraveVpnCalloutDialogFragment braveVpnCalloutDialogFragment =
                new BraveVpnCalloutDialogFragment();
        braveVpnCalloutDialogFragment.show(
                getSupportFragmentManager(), "BraveVpnCalloutDialogFragment");
    }

    private void showLinkVpnSubscriptionDialog() {
        LinkVpnSubscriptionDialogFragment linkVpnSubscriptionDialogFragment =
                new LinkVpnSubscriptionDialogFragment();
        linkVpnSubscriptionDialogFragment.setCancelable(false);
        linkVpnSubscriptionDialogFragment.show(
                getSupportFragmentManager(), "LinkVpnSubscriptionDialogFragment");
    }

    private void showAdFreeCalloutDialog() {
        SharedPreferencesManager.getInstance().writeBoolean(
                BravePreferenceKeys.BRAVE_AD_FREE_CALLOUT_DIALOG, false);

        BraveAdFreeCalloutDialogFragment braveAdFreeCalloutDialogFragment =
                new BraveAdFreeCalloutDialogFragment();
        braveAdFreeCalloutDialogFragment.show(
                getSupportFragmentManager(), "BraveAdFreeCalloutDialogFragment");
    }

    public void setNewTabPageManager(NewTabPageManager manager) {
        mNewTabPageManager = manager;
    }

    public void focusSearchBox() {
        if (mNewTabPageManager != null) {
            mNewTabPageManager.focusSearchBox(false, null);
        }
    }

    // Sets NTP background
    public void setBackground(Bitmap bgWallpaper) {
        CompositorViewHolder compositorView = findViewById(R.id.compositor_view_holder);
        if (compositorView != null) {
            ViewGroup root = (ViewGroup) compositorView.getChildAt(1);
            if (root != null && root.getChildAt(0) instanceof FrameLayout) {
                FrameLayout frameLayout = (FrameLayout) root.getChildAt(0);
                DisplayMetrics displayMetrics = new DisplayMetrics();
                getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
                int mDeviceHeight = displayMetrics.heightPixels;
                int mDeviceWidth = displayMetrics.widthPixels;
                Glide.with(this)
                        .asBitmap()
                        .load(bgWallpaper)
                        .centerCrop()
                        .override(mDeviceWidth, mDeviceHeight)
                        .priority(Priority.IMMEDIATE)
                        .diskCacheStrategy(DiskCacheStrategy.ALL)
                        .into(new CustomTarget<Bitmap>() {
                            @Override
                            public void onResourceReady(@NonNull Bitmap resource,
                                    @Nullable Transition<? super Bitmap> transition) {
                                Drawable drawable = new BitmapDrawable(getResources(), resource);
                                frameLayout.setBackground(drawable);
                            }
                            @Override
                            public void onLoadCleared(@Nullable Drawable placeholder) {}
                        });
            }
        }
    }

    private void checkFingerPrintingOnUpgrade() {
        if (!PackageUtils.isFirstInstall(this)
                && SharedPreferencesManager.getInstance().readInt(
                           BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                        == 0) {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            boolean value = sharedPreferences.getBoolean(
                    BravePrivacySettings.PREF_FINGERPRINTING_PROTECTION, true);
            if (value) {
                BraveShieldsContentSettings.setShieldsValue(Profile.getLastUsedRegularProfile(), "",
                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                        BraveShieldsContentSettings.DEFAULT, false);
            } else {
                BraveShieldsContentSettings.setShieldsValue(Profile.getLastUsedRegularProfile(), "",
                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                        BraveShieldsContentSettings.ALLOW_RESOURCE, false);
            }
        }
    }

    private void openBraveNewsSettings() {
        SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_NEWS_V2)) {
            settingsLauncher.launchSettingsActivity(this, BraveNewsPreferencesV2.class);
        } else {
            settingsLauncher.launchSettingsActivity(this, BraveNewsPreferences.class);
        }
    }

    public void openBraveWalletSettings() {
        SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
        settingsLauncher.launchSettingsActivity(this, BraveWalletPreferences.class);
    }

    public void openBraveConnectedSitesSettings() {
        SettingsLauncher settingsLauncher = new SettingsLauncherImpl();
        settingsLauncher.launchSettingsActivity(this, BraveWalletEthereumConnectedSites.class);
    }

    public void openBraveWallet(boolean fromDapp, boolean setupAction, boolean restoreAction) {
        Intent braveWalletIntent = new Intent(this, BraveWalletActivity.class);
        braveWalletIntent.putExtra(Utils.IS_FROM_DAPPS, fromDapp);
        braveWalletIntent.putExtra(Utils.RESTART_WALLET_ACTIVITY_SETUP, setupAction);
        braveWalletIntent.putExtra(Utils.RESTART_WALLET_ACTIVITY_RESTORE, restoreAction);
        braveWalletIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(braveWalletIntent);
    }

    public void viewOnBlockExplorer(String address, @CoinType.EnumType int coinType) {
        Utils.openAddress("/address/" + address, mJsonRpcService, this, coinType);
    }

    // should only be called if the wallet is setup and unlocked
    public void openNetworkSelection() {
        openNetworkSelection(DEFAULT_WALLET_NETWORK);
    }

    // should only be called if the wallet is setup and unlocked
    public void openNetworkSelection(NetworkSelectorModel.Mode mode) {
        Intent braveNetworkSelectionIntent = new Intent(this, NetworkSelectorActivity.class);
        braveNetworkSelectionIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        // Either in global or local network selection mode
        braveNetworkSelectionIntent.putExtra(NETWORK_SELECTOR_MODE, mode);
        startActivity(braveNetworkSelectionIntent);
    }

    public void openBraveWalletDAppsActivity(BraveWalletDAppsActivity.ActivityType activityType) {
        Intent braveWalletIntent = new Intent(this, BraveWalletDAppsActivity.class);
        braveWalletIntent.putExtra("activityType", activityType.getValue());
        braveWalletIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(braveWalletIntent);
    }

    private void checkForYandexSE() {
        String countryCode = Locale.getDefault().getCountry();
        if (yandexRegions.contains(countryCode)) {
            Profile lastUsedRegularProfile = Profile.getLastUsedRegularProfile();
            TemplateUrl yandexTemplateUrl = BraveSearchEngineUtils.getTemplateUrlByShortName(
                    lastUsedRegularProfile, OnboardingPrefManager.YANDEX);
            if (yandexTemplateUrl != null) {
                BraveSearchEngineUtils.setDSEPrefs(yandexTemplateUrl, lastUsedRegularProfile);
                BraveSearchEngineUtils.setDSEPrefs(yandexTemplateUrl,
                        lastUsedRegularProfile.getPrimaryOTRProfile(/* createIfNeeded= */ true));
            }
        }
    }

    private void showNotificationWarningDialog() {
        BraveNotificationWarningDialog notificationWarningDialog =
                BraveNotificationWarningDialog.newInstance(
                        BraveNotificationWarningDialog.FROM_LAUNCHED_BRAVE_ACTIVITY);
        notificationWarningDialog.setCancelable(false);
        notificationWarningDialog.setDismissListener(closeDialogListener);
        notificationWarningDialog.show(getSupportFragmentManager(),
                BraveNotificationWarningDialog.NOTIFICATION_WARNING_DIALOG_TAG);
    }

    private BraveNotificationWarningDialog.DismissListener closeDialogListener =
            new BraveNotificationWarningDialog.DismissListener() {
                @Override
                public void onDismiss() {
                    checkForNotificationData();
                }
            };

    private void checkAndshowNotificationWarningDialog() {
        OnboardingPrefManager.getInstance().updateLaunchCount();
        if (OnboardingPrefManager.getInstance().launchCount() >= 3
                && BraveNotificationWarningDialog.shouldShowNotificationWarningDialog(this)
                && !OnboardingPrefManager.getInstance()
                            .isNotificationPermissionEnablingDialogShown()) {
            if (BravePermissionUtils.hasNotificationPermission(this)) {
                showNotificationWarningDialog();
            } else {
                maybeShowNotificationPermissionRetionale();
            }
            OnboardingPrefManager.getInstance().setNotificationPermissionEnablingDialogShown(true);
        } else {
            checkForNotificationData();
        }
    }

    private void checkForNotificationData() {
        Intent notifIntent = getIntent();
        if (notifIntent != null && notifIntent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE) != null) {
            String notificationType = notifIntent.getStringExtra(RetentionNotificationUtil.NOTIFICATION_TYPE);
            switch (notificationType) {
                case RetentionNotificationUtil.HOUR_3:
                case RetentionNotificationUtil.HOUR_24:
                case RetentionNotificationUtil.EVERY_SUNDAY:
                    checkForBraveStats();
                    break;
                case RetentionNotificationUtil.DAY_6:
                    if (getActivityTab() != null && getActivityTab().getUrl().getSpec() != null
                            && !UrlUtilities.isNTPUrl(getActivityTab().getUrl().getSpec())) {
                        getTabCreator(false).launchUrl(
                                UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
                    }
                    break;
                case RetentionNotificationUtil.DAY_10:
                case RetentionNotificationUtil.DAY_30:
                case RetentionNotificationUtil.DAY_35:
                    openRewardsPanel();
                    break;
                case RetentionNotificationUtil.DORMANT_USERS_DAY_14:
                case RetentionNotificationUtil.DORMANT_USERS_DAY_25:
                case RetentionNotificationUtil.DORMANT_USERS_DAY_40:
                    showDormantUsersEngagementDialog(notificationType);
                    break;
                case RetentionNotificationUtil.DEFAULT_BROWSER_1:
                case RetentionNotificationUtil.DEFAULT_BROWSER_2:
                case RetentionNotificationUtil.DEFAULT_BROWSER_3:
                    if (!BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(BraveActivity.this)
                            && !BraveSetDefaultBrowserUtils.isBraveDefaultDontAsk()) {
                        isSetDefaultBrowserNotification = true;
                        BraveSetDefaultBrowserUtils.showBraveSetDefaultBrowserDialog(
                                BraveActivity.this, false);
                    }
                    break;
            }
        }
    }

    public void checkForBraveStats() {
        if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
            BraveStatsUtil.showBraveStats();
        } else {
            if (getActivityTab() != null && getActivityTab().getUrl().getSpec() != null
                    && !UrlUtilities.isNTPUrl(getActivityTab().getUrl().getSpec())) {
                OnboardingPrefManager.getInstance().setFromNotification(true);
                if (getTabCreator(false) != null) {
                    getTabCreator(false).launchUrl(
                            UrlConstants.NTP_URL, TabLaunchType.FROM_CHROME_UI);
                }
            } else {
                showOnboardingV2(false);
            }
        }
    }

    public void showOnboardingV2(boolean fromStats) {
        try {
            OnboardingPrefManager.getInstance().setNewOnboardingShown(true);
            FragmentManager fm = getSupportFragmentManager();
            HighlightDialogFragment fragment = (HighlightDialogFragment) fm.findFragmentByTag(
                    HighlightDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = fm.beginTransaction();

            if (fragment != null) {
                transaction.remove(fragment);
            }

            fragment = new HighlightDialogFragment();
            Bundle fragmentBundle = new Bundle();
            fragmentBundle.putBoolean(OnboardingPrefManager.FROM_STATS, fromStats);
            fragment.setArguments(fragmentBundle);
            transaction.add(fragment, HighlightDialogFragment.TAG_FRAGMENT);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("HighlightDialogFragment", e.getMessage());
        }
    }

    public void hideRewardsOnboardingIcon() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.hideRewardsOnboardingIcon();
        }
    }

    private void createNotificationChannel() {
        // Create the NotificationChannel, but only on API 26+ because
        // the NotificationChannel class is new and not in the support library
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID, getString(R.string.brave_browser), importance);
            channel.setDescription(
                    getString(R.string.brave_browser_notification_channel_description));
            // Register the channel with the system; you can't change the importance
            // or other notification behaviors after this
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }
    }

    private boolean isNoRestoreState() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_CLOSE_TABS_ON_EXIT, false);
    }

    private boolean isClearBrowsingDataOnExit() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_CLEAR_ON_EXIT, false);
    }

    public void OnRewardsPanelDismiss() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.onRewardsPanelDismiss();
        }
    }

    public void dismissRewardsPanel() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.dismissRewardsPanel();
        }
    }

    public void dismissShieldsTooltip() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.dismissShieldsTooltip();
        }
    }

    public void dismissCookieConsent() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.dismissCookieConsent();
        }
    }

    public void openRewardsPanel() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        if (layout != null) {
            layout.openRewardsPanel();
        }
    }

    public Profile getCurrentProfile() {
        Tab tab = getActivityTab();
        if (tab == null) {
            return Profile.getLastUsedRegularProfile();
        }

        return Profile.fromWebContents(tab.getWebContents());
    }

    public Tab selectExistingTab(String url) {
        Tab tab = getActivityTab();
        if (tab != null && tab.getUrl().getSpec().equals(url)) {
            return tab;
        }

        TabModel tabModel = getCurrentTabModel();
        int tabIndex = TabModelUtils.getTabIndexByUrl(tabModel, url);

        // Find if tab exists
        if (tabIndex != TabModel.INVALID_TAB_INDEX) {
            tab = tabModel.getTabAt(tabIndex);
            // Set active tab
            tabModel.setIndex(tabIndex, TabSelectionType.FROM_USER, false);
            return tab;
        } else {
            return null;
        }
    }

    public Tab openNewOrSelectExistingTab(String url) {
        TabModel tabModel = getCurrentTabModel();
        int tabRewardsIndex = TabModelUtils.getTabIndexByUrl(tabModel, url);
        Tab tab = selectExistingTab(url);
        if (tab != null) {
            return tab;
        } else { // Open a new tab
            return getTabCreator(false).launchUrl(url, TabLaunchType.FROM_CHROME_UI);
        }
    }

    private void clearWalletModelServices() {
        if (mWalletModel == null) {
            return;
        }

        mWalletModel.resetServices(
                getApplicationContext(), null, null, null, null, null, null, null, null, null);
    }

    private void setupWalletModel() {
        if (mWalletModel == null) {
            mWalletModel = new WalletModel(getApplicationContext(), mKeyringService,
                    mBlockchainRegistry, mJsonRpcService, mTxService, mEthTxManagerProxy,
                    mSolanaTxManagerProxy, mAssetRatioService, mBraveWalletService, mSwapService);
        } else {
            mWalletModel.resetServices(getApplicationContext(), mKeyringService,
                    mBlockchainRegistry, mJsonRpcService, mTxService, mEthTxManagerProxy,
                    mSolanaTxManagerProxy, mAssetRatioService, mBraveWalletService, mSwapService);
        }
        setupObservers();
    }

    private void setupObservers() {
        clearObservers();
        mWalletModel.getCryptoModel().getPendingTxHelper().mSelectedPendingRequest.observe(
                this, transactionInfo -> {
                    // don't show dapps panel if the wallet is locked and requests are being
                    // processed by the approve dialog already
                    KeyringInfo keyringInfo = mWalletModel.getKeyringModel().getKeyringInfo();
                    if (transactionInfo != null && keyringInfo != null && !keyringInfo.isLocked
                            && !isProcessingPendingDappsTxRequest) {
                        isProcessingPendingDappsTxRequest = true;
                        openBraveWalletDAppsActivity(
                                BraveWalletDAppsActivity.ActivityType.CONFIRM_TRANSACTION);
                    }
                    // update badge if there's a pending tx
                    if (transactionInfo != null && keyringInfo != null && !keyringInfo.isLocked) {
                        updateWalletBadgeVisibility();
                    }
                });

        mWalletModel.getDappsModel().mWalletIconNotificationVisible.observe(
                this, visible -> { setWalletBadgeVisibility(visible); });

        mWalletModel.getDappsModel().mPendingWalletAccountCreationRequest.observe(this, request -> {
            if (request == null) return;
            mWalletModel.getKeyringModel().isWalletLocked(isLocked -> {
                // Cannot use mWalletModel.getKeyringModel().getKeyringInfo().isLocked as account
                // creation request can be triggered when the wallet is locked and keyringInfo will
                // be null
                if (!BraveWalletPreferences.getPrefWeb3NotificationsEnabled()) return;
                if (isLocked) {
                    Tab tab = getActivityTab();
                    if (tab != null) {
                        walletInteractionDetected(tab.getWebContents());
                    }
                    showWalletPanel(false);
                    return;
                }
                for (CryptoAccountTypeInfo info :
                        mWalletModel.getCryptoModel().getSupportedCryptoAccountTypes()) {
                    if (info.getCoinType() == request.getCoinType()) {
                        Intent addAccountActivityIntent =
                                new Intent(this, AddAccountActivity.class);
                        addAccountActivityIntent.putExtra(AddAccountActivity.ACCOUNT, info);
                        startActivity(addAccountActivityIntent);
                        mWalletModel.getDappsModel().removeProcessedAccountCreationRequest(request);
                        break;
                    }
                }
            });
        });

        mWalletModel.getCryptoModel().getNetworkModel().mNeedToCreateAccountForNetwork.observe(
                this, networkInfo -> {
                    if (networkInfo == null) return;
                    MaterialAlertDialogBuilder builder =
                            new MaterialAlertDialogBuilder(
                                    this, R.style.BraveWalletAlertDialogTheme)
                                    .setMessage(getString(
                                            R.string.brave_wallet_create_account_description,
                                            networkInfo.symbolName))
                                    .setPositiveButton(R.string.wallet_action_yes,
                                            (dialog, which) -> {
                                                mWalletModel.getCryptoModel()
                                                        .getNetworkModel()
                                                        .setNetwork(networkInfo, success -> {
                                                            if (success) {
                                                                mWalletModel.getKeyringModel().addAccount(
                                                                        WalletUtils.getUniqueNextAccountName(
                                                                                this,
                                                                                mWalletModel
                                                                                        .getKeyringModel()
                                                                                        .mAccountInfos
                                                                                        .getValue()
                                                                                        .toArray(
                                                                                                new AccountInfo
                                                                                                        [0]),
                                                                                networkInfo
                                                                                        .symbolName,
                                                                                networkInfo.coin),
                                                                        networkInfo.coin,
                                                                        isAccountAdded -> {});
                                                            }
                                                            mWalletModel.getCryptoModel()
                                                                    .getNetworkModel()
                                                                    .clearCreateAccountState();
                                                        });
                                            })
                                    .setNegativeButton(
                                            R.string.wallet_action_no, (dialog, which) -> {
                                                mWalletModel.getCryptoModel()
                                                        .getNetworkModel()
                                                        .clearCreateAccountState();
                                                dialog.dismiss();
                                            });
                    builder.show();
                });
    }

    private void clearObservers() {
        mWalletModel.getCryptoModel().getPendingTxHelper().mSelectedPendingRequest.removeObservers(
                this);
        mWalletModel.getDappsModel().mWalletIconNotificationVisible.removeObservers(this);
        mWalletModel.getCryptoModel()
                .getNetworkModel()
                .mNeedToCreateAccountForNetwork.removeObservers(this);
    }

    private void showBraveRateDialog() {
        BraveRateDialogFragment rateDialogFragment = BraveRateDialogFragment.newInstance(false);
        rateDialogFragment.show(getSupportFragmentManager(), BraveRateDialogFragment.TAG_FRAGMENT);
    }

    private void showCrossPromotionalDialog() {
        CrossPromotionalModalDialogFragment mCrossPromotionalModalDialogFragment =
                new CrossPromotionalModalDialogFragment();
        mCrossPromotionalModalDialogFragment.show(getSupportFragmentManager(), "CrossPromotionalModalDialogFragment");
    }

    public void showDormantUsersEngagementDialog(String notificationType) {
        if (!BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(BraveActivity.this)
                && !BraveSetDefaultBrowserUtils.isBraveDefaultDontAsk()) {
            DormantUsersEngagementDialogFragment dormantUsersEngagementDialogFragment =
                    new DormantUsersEngagementDialogFragment();
            dormantUsersEngagementDialogFragment.setNotificationType(notificationType);
            dormantUsersEngagementDialogFragment.show(
                    getSupportFragmentManager(), "DormantUsersEngagementDialogFragment");
            setDormantUsersPrefs();
        }
    }

    static public ChromeTabbedActivity getChromeTabbedActivity() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof ChromeTabbedActivity)) continue;

            return (ChromeTabbedActivity)ref;
        }

        return null;
    }

    static public BraveActivity getBraveActivity() {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!(ref instanceof BraveActivity)) continue;

            return (BraveActivity)ref;
        }

        return null;
    }

    @Override
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (intent != null) {
            String openUrl = intent.getStringExtra(BraveActivity.OPEN_URL);
            if (!TextUtils.isEmpty(openUrl)) {
                try {
                    openNewOrSelectExistingTab(openUrl);
                } catch (NullPointerException e) {
                    Log.e("BraveActivity", "opening new tab " + e.getMessage());
                }
            }
        }
        checkForNotificationData();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK
                && (requestCode == BraveConstants.VERIFY_WALLET_ACTIVITY_REQUEST_CODE
                        || requestCode == BraveConstants.USER_WALLET_ACTIVITY_REQUEST_CODE
                        || requestCode == BraveConstants.SITE_BANNER_REQUEST_CODE)) {
            dismissRewardsPanel();
            if (data != null) {
                String open_url = data.getStringExtra(BraveActivity.OPEN_URL);
                if (!TextUtils.isEmpty(open_url)) {
                    openNewOrSelectExistingTab(open_url);
                }
            }
        } else if (resultCode == RESULT_OK
                && requestCode == BraveConstants.MONTHLY_CONTRIBUTION_REQUEST_CODE) {
            dismissRewardsPanel();

        } else if (resultCode == RESULT_OK
                && requestCode == BraveConstants.DEFAULT_BROWSER_ROLE_REQUEST_CODE) {
            BraveSetDefaultBrowserUtils.setBraveDefaultSuccess();
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        FragmentManager fm = getSupportFragmentManager();
        BraveStatsBottomSheetDialogFragment fragment =
                (BraveStatsBottomSheetDialogFragment) fm.findFragmentByTag(
                        BraveStatsUtil.STATS_FRAGMENT_TAG);
        if (fragment != null) {
            fragment.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }

        if (requestCode == BraveStatsUtil.SHARE_STATS_WRITE_EXTERNAL_STORAGE_PERM
                && grantResults.length != 0
                && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            BraveStatsUtil.shareStats(R.layout.brave_stats_share_layout);
        }
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    /**
     * Disable background ads on Android. Issue #8641.
     */
    private void setBgBraveAdsDefaultOff() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        boolean exists = sharedPreferences.contains(
                BraveRewardsPreferences.PREF_ADS_SWITCH_DEFAULT_HAS_BEEN_SET);
        if (!exists) {
            SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
            sharedPreferencesEditor.putBoolean(BraveRewardsPreferences.PREF_ADS_SWITCH, false);
            sharedPreferencesEditor.putBoolean(
                    BraveRewardsPreferences.PREF_ADS_SWITCH_DEFAULT_HAS_BEEN_SET, true);
            sharedPreferencesEditor.apply();
        }
    }

    @Override
    public void performPreInflationStartup() {
        BraveDbUtil dbUtil = BraveDbUtil.getInstance();
        if (dbUtil.dbOperationRequested()) {
            AlertDialog dialog =
                    new AlertDialog.Builder(this)
                            .setMessage(dbUtil.performDbExportOnStart()
                                            ? getString(
                                                    R.string.brave_db_processing_export_alert_info)
                                            : getString(
                                                    R.string.brave_db_processing_import_alert_info))
                            .setCancelable(false)
                            .create();
            dialog.setCanceledOnTouchOutside(false);
            if (dbUtil.performDbExportOnStart()) {
                dbUtil.setPerformDbExportOnStart(false);
                dbUtil.ExportRewardsDb(dialog);
            } else if (dbUtil.performDbImportOnStart() && !dbUtil.dbImportFile().isEmpty()) {
                dbUtil.setPerformDbImportOnStart(false);
                dbUtil.ImportRewardsDb(dialog, dbUtil.dbImportFile());
            }
            dbUtil.cleanUpDbOperationRequest();
        }
        super.performPreInflationStartup();
    }

    @Override
    protected @LaunchIntentDispatcher.Action int maybeDispatchLaunchIntent(
            Intent intent, Bundle savedInstanceState) {
        boolean notificationUpdate = IntentUtils.safeGetBooleanExtra(
                intent, BravePreferenceKeys.BRAVE_UPDATE_EXTRA_PARAM, false);
        if (notificationUpdate) {
            SetUpdatePreferences();
        }

        return super.maybeDispatchLaunchIntent(intent, savedInstanceState);
    }

    private void SetUpdatePreferences() {
        Calendar currentTime = Calendar.getInstance();
        long milliSeconds = currentTime.getTimeInMillis();

        SharedPreferences sharedPref = getApplicationContext().getSharedPreferences(
                BravePreferenceKeys.BRAVE_NOTIFICATION_PREF_NAME, 0);
        SharedPreferences.Editor editor = sharedPref.edit();

        editor.putLong(BravePreferenceKeys.BRAVE_MILLISECONDS_NAME, milliSeconds);
        editor.apply();
    }

    public ObservableSupplier<BrowserControlsManager> getBrowserControlsManagerSupplier() {
        return mBrowserControlsManagerSupplier;
    }

    public int getToolbarShadowHeight() {
        View toolbarShadow = findViewById(R.id.toolbar_hairline);
        assert toolbarShadow != null;
        if (toolbarShadow != null) {
            return toolbarShadow.getHeight();
        }
        return 0;
    }

    public float getToolbarBottom() {
        View toolbarShadow = findViewById(R.id.toolbar_hairline);
        assert toolbarShadow != null;
        if (toolbarShadow != null) {
            return toolbarShadow.getY();
        }
        return 0;
    }

    public boolean isViewBelowToolbar(View view) {
        View toolbarShadow = findViewById(R.id.toolbar_hairline);
        assert toolbarShadow != null;
        assert view != null;
        if (toolbarShadow != null && view != null) {
            int[] coordinatesToolbar = new int[2];
            toolbarShadow.getLocationInWindow(coordinatesToolbar);
            int[] coordinatesView = new int[2];
            view.getLocationInWindow(coordinatesView);
            return coordinatesView[1] >= coordinatesToolbar[1];
        }

        return false;
    }

    @NativeMethods
    interface Natives {
        void restartStatsUpdater();
    }

    @Override
    public void initializeCompositor() {
        super.initializeCompositor();

        BraveTabUiFeatureUtilities.maybeOverrideEnableTabGroupAutoCreationPreference(
                ContextUtils.getApplicationContext());
    }

    private void InitBraveWalletService() {
        if (mBraveWalletService != null) {
            return;
        }

        mBraveWalletService = BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
    }

    private void InitKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
    }

    private void InitJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = JsonRpcServiceFactory.getInstance().getJsonRpcService(this);
    }

    private void InitTxService() {
        if (mTxService != null) {
            return;
        }

        mTxService = TxServiceFactory.getInstance().getTxService(this);
    }

    private void InitEthTxManagerProxy() {
        if (mEthTxManagerProxy != null) {
            return;
        }

        mEthTxManagerProxy = TxServiceFactory.getInstance().getEthTxManagerProxy(this);
    }

    private void InitSolanaTxManagerProxy() {
        if (mSolanaTxManagerProxy != null) {
            return;
        }

        mSolanaTxManagerProxy = TxServiceFactory.getInstance().getSolanaTxManagerProxy(this);
    }

    private void InitBlockchainRegistry() {
        if (mBlockchainRegistry != null) {
            return;
        }

        mBlockchainRegistry = BlockchainRegistryFactory.getInstance().getBlockchainRegistry(this);
    }

    private void InitAssetRatioService() {
        if (mAssetRatioService != null) {
            return;
        }

        mAssetRatioService = AssetRatioServiceFactory.getInstance().getAssetRatioService(this);
    }

    private void initSwapService() {
        if (mSwapService != null) {
            return;
        }
        mSwapService = SwapServiceFactory.getInstance().getSwapService(this);
    }

    private void initNativeServices() {
        InitBlockchainRegistry();
        InitTxService();
        InitEthTxManagerProxy();
        InitSolanaTxManagerProxy();
        InitAssetRatioService();
        InitBraveWalletService();
        InitKeyringService();
        InitJsonRpcService();
        initSwapService();
        setupWalletModel();
    }

    private void cleanUpNativeServices() {
        clearWalletModelServices();
        if (mKeyringService != null) mKeyringService.close();
        if (mAssetRatioService != null) mAssetRatioService.close();
        if (mBlockchainRegistry != null) mBlockchainRegistry.close();
        if (mJsonRpcService != null) mJsonRpcService.close();
        if (mTxService != null) mTxService.close();
        if (mEthTxManagerProxy != null) mEthTxManagerProxy.close();
        if (mSolanaTxManagerProxy != null) mSolanaTxManagerProxy.close();
        if (mBraveWalletService != null) mBraveWalletService.close();
        mKeyringService = null;
        mBlockchainRegistry = null;
        mJsonRpcService = null;
        mTxService = null;
        mEthTxManagerProxy = null;
        mSolanaTxManagerProxy = null;
        mAssetRatioService = null;
        mBraveWalletService = null;
    }

    @NonNull
    private BraveToolbarLayoutImpl getBraveToolbarLayout() {
        BraveToolbarLayoutImpl layout = findViewById(R.id.toolbar);
        assert layout != null;
        return layout;
    }

    public void addOrEditBookmark(final Tab tabToBookmark) {
        RateUtils.getInstance().setPrefAddedBookmarkCount();
        ((TabBookmarker) mTabBookmarkerSupplier.get()).addOrEditBookmark(tabToBookmark);
    }

    // We call that method with an interval
    // BraveSafeBrowsingApiHandler.SAFE_BROWSING_INIT_INTERVAL_MS,
    // as upstream does, to keep the GmsCore process alive.
    private void executeInitSafeBrowsing(long delay) {
        // SafeBrowsingBridge.getSafeBrowsingState() has to be executed on a main thread
        PostTask.postDelayedTask(UiThreadTaskTraits.DEFAULT, () -> {
            if (SafeBrowsingBridge.getSafeBrowsingState() != SafeBrowsingState.NO_SAFE_BROWSING) {
                // initSafeBrowsing could be executed on a background thread
                PostTask.postTask(TaskTraits.USER_VISIBLE_MAY_BLOCK,
                        () -> { BraveSafeBrowsingApiHandler.getInstance().initSafeBrowsing(); });
            }
            executeInitSafeBrowsing(BraveSafeBrowsingApiHandler.SAFE_BROWSING_INIT_INTERVAL_MS);
        }, delay);
    }
}

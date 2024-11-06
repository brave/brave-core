/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.app.Activity;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentSender;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.coordinatorlayout.widget.CoordinatorLayout;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.brave.playlist.util.ConstantUtils;
import com.brave.playlist.util.PlaylistPreferenceUtils;
import com.brave.playlist.util.PlaylistUtils;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.play.core.appupdate.AppUpdateInfo;
import com.google.android.play.core.appupdate.AppUpdateManager;
import com.google.android.play.core.appupdate.AppUpdateManagerFactory;
import com.google.android.play.core.install.InstallStateUpdatedListener;
import com.google.android.play.core.install.model.AppUpdateType;
import com.google.android.play.core.install.model.InstallStatus;
import com.google.android.play.core.install.model.UpdateAvailability;
import com.google.android.play.core.tasks.Task;
import com.wireguard.android.backend.GoBackend;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.CollectionUtil;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.IntentUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.UnownedUserDataSupplier;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SolanaTxManagerProxy;
import org.chromium.brave_wallet.mojom.SwapService;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdFreeCalloutDialogFragment;
import org.chromium.chrome.browser.BraveFeatureUtil;
import org.chromium.chrome.browser.BraveHelper;
import org.chromium.chrome.browser.BraveIntentHandler;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveSyncWorker;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.CrossPromotionalModalDialogFragment;
import org.chromium.chrome.browser.DormantUsersEngagementDialogFragment;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.LaunchIntentDispatcher;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.PurchaseModel;
import org.chromium.chrome.browser.bookmarks.TabBookmarker;
import org.chromium.chrome.browser.brave_leo.BraveLeoActivity;
import org.chromium.chrome.browser.brave_leo.BraveLeoUtils;
import org.chromium.chrome.browser.brave_news.BraveNewsConnectionErrorHandler;
import org.chromium.chrome.browser.brave_news.BraveNewsControllerFactory;
import org.chromium.chrome.browser.brave_news.BraveNewsUtils;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.brave_stats.BraveStatsBottomSheetDialogFragment;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.browsing_data.BrowsingDataBridge;
import org.chromium.chrome.browser.browsing_data.BrowsingDataType;
import org.chromium.chrome.browser.browsing_data.TimePeriod;
import org.chromium.chrome.browser.compositor.layouts.LayoutManagerChrome;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.SwapServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.model.CryptoAccountTypeInfo;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.custom_layout.popup_window_tooltip.PopupWindowTooltip;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.flags.ChromeSwitches;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.informers.BraveSyncAccountDeletedInformer;
import org.chromium.chrome.browser.lifetime.ApplicationLifetime;
import org.chromium.chrome.browser.misc_metrics.MiscAndroidMetricsConnectionErrorHandler;
import org.chromium.chrome.browser.misc_metrics.MiscAndroidMetricsFactory;
import org.chromium.chrome.browser.multiwindow.BraveMultiWindowUtils;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.multiwindow.MultiWindowUtils;
import org.chromium.chrome.browser.notifications.BraveNotificationWarningDialog;
import org.chromium.chrome.browser.notifications.permissions.NotificationPermissionController;
import org.chromium.chrome.browser.notifications.retention.RetentionNotificationUtil;
import org.chromium.chrome.browser.ntp.NewTabPageManager;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.onboarding.v2.HighlightDialogFragment;
import org.chromium.chrome.browser.onboarding.v2.HighlightItem;
import org.chromium.chrome.browser.onboarding.v2.HighlightView;
import org.chromium.chrome.browser.playlist.PlaylistHostActivity;
import org.chromium.chrome.browser.playlist.settings.BravePlaylistPreferences;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.preferences.PrefChangeRegistrar;
import org.chromium.chrome.browser.preferences.PrefChangeRegistrar.PrefObserver;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.prefetch.settings.PreloadPagesSettingsBridge;
import org.chromium.chrome.browser.prefetch.settings.PreloadPagesState;
import org.chromium.chrome.browser.privacy.settings.BravePrivacySettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.quick_search_engines.settings.QuickSearchEnginesCallback;
import org.chromium.chrome.browser.quick_search_engines.settings.QuickSearchEnginesFragment;
import org.chromium.chrome.browser.quick_search_engines.settings.QuickSearchEnginesModel;
import org.chromium.chrome.browser.quick_search_engines.settings.QuickSearchEnginesUtil;
import org.chromium.chrome.browser.quick_search_engines.views.QuickSearchEnginesViewAdapter;
import org.chromium.chrome.browser.rate.BraveRateDialogFragment;
import org.chromium.chrome.browser.rate.RateUtils;
import org.chromium.chrome.browser.rewards.adaptive_captcha.AdaptiveCaptchaHelper;
import org.chromium.chrome.browser.safe_browsing.SafeBrowsingBridge;
import org.chromium.chrome.browser.safe_browsing.SafeBrowsingState;
import org.chromium.chrome.browser.set_default_browser.BraveSetDefaultBrowserUtils;
import org.chromium.chrome.browser.set_default_browser.OnBraveSetDefaultBrowserListener;
import org.chromium.chrome.browser.settings.BraveNewsPreferencesV2;
import org.chromium.chrome.browser.settings.BraveSearchEngineUtils;
import org.chromium.chrome.browser.settings.BraveWalletPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.settings.developer.BraveQAPreferences;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.share.ShareDelegate.ShareOrigin;
import org.chromium.chrome.browser.shields.ContentFilteringFragment;
import org.chromium.chrome.browser.site_settings.BraveWalletEthereumConnectedSites;
import org.chromium.chrome.browser.speedreader.BraveSpeedReaderUtils;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabClosureParams;
import org.chromium.chrome.browser.tabmodel.TabList;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl;
import org.chromium.chrome.browser.ui.RootUiCoordinator;
import org.chromium.chrome.browser.ui.messages.snackbar.Snackbar;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager.SnackbarController;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManagerProvider;
import org.chromium.chrome.browser.util.BraveConstants;
import org.chromium.chrome.browser.util.BraveDbUtil;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.util.ImageUtils;
import org.chromium.chrome.browser.util.KeyboardVisibilityHelper;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.browser.util.UsageMonitor;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.activities.BraveVpnProfileActivity;
import org.chromium.chrome.browser.vpn.fragments.LinkVpnSubscriptionDialogFragment;
import org.chromium.chrome.browser.vpn.timer.TimerDialogFragment;
import org.chromium.chrome.browser.vpn.utils.BraveVpnApiResponseUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.wireguard.WireguardConfigUtils;
import org.chromium.components.browser_ui.settings.SettingsNavigation;
import org.chromium.components.embedder_support.util.UrlConstants;
import org.chromium.components.embedder_support.util.UrlUtilities;
import org.chromium.components.safe_browsing.BraveSafeBrowsingApiHandler;
import org.chromium.components.search_engines.TemplateUrl;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.misc_metrics.mojom.MiscAndroidMetrics;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.widget.Toast;

import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;

/** Brave's extension for ChromeActivity */
@JNINamespace("chrome::android")
public abstract class BraveActivity extends ChromeActivity
        implements BrowsingDataBridge.OnClearBrowsingDataListener,
                BraveVpnObserver,
                OnBraveSetDefaultBrowserListener,
                ConnectionErrorHandler,
                PrefObserver,
                BraveSafeBrowsingApiHandler.BraveSafeBrowsingApiHandlerDelegate,
                BraveNewsConnectionErrorHandler.BraveNewsConnectionErrorHandlerDelegate,
                MiscAndroidMetricsConnectionErrorHandler
                        .MiscAndroidMetricsConnectionErrorHandlerDelegate,
                QuickSearchEnginesCallback,
                KeyboardVisibilityHelper.KeyboardVisibilityListener {
    public static final String BRAVE_WALLET_HOST = "wallet";
    public static final String BRAVE_WALLET_ORIGIN = "brave://wallet/";
    public static final String BRAVE_WALLET_URL = "brave://wallet/crypto/portfolio/assets";
    public static final String BRAVE_BUY_URL = "brave://wallet/crypto/fund-wallet";
    public static final String BRAVE_SEND_URL = "brave://wallet/send";
    public static final String BRAVE_SWAP_URL = "brave://wallet/swap";
    public static final String BRAVE_DEPOSIT_URL = "brave://wallet/crypto/deposit-funds";
    public static final String BRAVE_REWARDS_SETTINGS_URL = "brave://rewards/";
    public static final String BRAVE_REWARDS_SETTINGS_WALLET_VERIFICATION_URL =
            "brave://rewards/#verify";
    public static final String BRAVE_REWARDS_WALLET_RECONNECT_URL = "brave://rewards/reconnect";
    public static final String BRAVE_REWARDS_SETTINGS_MONTHLY_URL = "brave://rewards/#monthly";
    public static final String REWARDS_AC_SETTINGS_URL = "brave://rewards/contribute";
    public static final String BRAVE_REWARDS_RESET_PAGE = "brave://rewards/#reset";
    public static final String BRAVE_AI_CHAT_URL = "chrome-untrusted://chat";
    public static final String REWARDS_LEARN_MORE_URL =
            "https://brave.com/faq-rewards/#unclaimed-funds";
    public static final String BRAVE_TERMS_PAGE =
            "https://basicattentiontoken.org/user-terms-of-service/";
    public static final String BRAVE_PRIVACY_POLICY = "https://brave.com/privacy/browser/#rewards";
    public static final String OPEN_URL = "open_url";
    public static final String BRAVE_WEBCOMPAT_INFO_WIKI_URL =
            "https://github.com/brave/brave-browser/wiki/Web-compatibility-reports";

    private static final int DAYS_1 = 1;
    private static final int DAYS_4 = 4;
    private static final int DAYS_5 = 5;
    private static final int DAYS_7 = 7;
    private static final int DAYS_12 = 12;

    private static final int MONTH_1 = 1;

    public static final int MAX_FAILED_CAPTCHA_ATTEMPTS = 10;

    public static final int APP_OPEN_COUNT_FOR_WIDGET_PROMO = 25;

    public static final String BRAVE_SEARCH_ENGINE_KEYWORD = ":br";
    public static final String BING_SEARCH_ENGINE_KEYWORD = ":b";
    public static final String STARTPAGE_SEARCH_ENGINE_KEYWORD = ":sp";

    private static final boolean ENABLE_IN_APP_UPDATE =
            Build.VERSION.SDK_INT < Build.VERSION_CODES.UPSIDE_DOWN_CAKE;

    /** Settings for sending local notification reminders. */
    public static final String CHANNEL_ID = "com.brave.browser";

    // Explicitly declare this variable to avoid build errors.
    // It will be removed in asm and parent variable will be used instead.
    private UnownedUserDataSupplier<BrowserControlsManager> mBrowserControlsManagerSupplier;

    private static final List<String> sYandexRegions =
            Arrays.asList("AM", "AZ", "BY", "KG", "KZ", "MD", "RU", "TJ", "TM", "UZ");

    private boolean mIsVerification;
    private boolean mIsDefaultCheckOnResume;
    private boolean mIsSetDefaultBrowserNotification;
    public boolean mIsDeepLink;
    private BraveWalletService mBraveWalletService;
    private KeyringService mKeyringService;
    private JsonRpcService mJsonRpcService;
    private MiscAndroidMetrics mMiscAndroidMetrics;
    private SwapService mSwapService;
    @Nullable private WalletModel mWalletModel;
    private BlockchainRegistry mBlockchainRegistry;
    private TxService mTxService;
    private EthTxManagerProxy mEthTxManagerProxy;
    private SolanaTxManagerProxy mSolanaTxManagerProxy;
    private AssetRatioService mAssetRatioService;
    public boolean mLoadedFeed;
    public boolean mComesFromNewTab;
    public CopyOnWriteArrayList<FeedItemsCard> mNewsItemsFeedCards;
    private boolean mIsProcessingPendingDappsTxRequest;
    private int mLastTabId;
    private boolean mNativeInitialized;
    private boolean mSafeBrowsingFlagEnabled;
    private NewTabPageManager mNewTabPageManager;
    private UsageMonitor mUsageMonitor;
    private NotificationPermissionController mNotificationPermissionController;
    private BraveNewsController mBraveNewsController;
    private BraveNewsConnectionErrorHandler mBraveNewsConnectionErrorHandler;
    private MiscAndroidMetricsConnectionErrorHandler mMiscAndroidMetricsConnectionErrorHandler;
    private AppUpdateManager mAppUpdateManager;
    private boolean mWalletBadgeVisible;
    private boolean mSpoofCustomTab;

    private View mQuickSearchEnginesView;

    /** Serves as a general exception for failed attempts to get BraveActivity. */
    public static class BraveActivityNotFoundException extends Exception {
        public BraveActivityNotFoundException(String message) {
            super(message);
        }
    }

    public BraveActivity() {}

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        BraveActivityJni.get().restartStatsUpdater();
        if (BraveVpnUtils.isVpnFeatureSupported(BraveActivity.this)) {
            BraveVpnNativeWorker.getInstance().addObserver(this);
            BraveVpnUtils.reportBackgroundUsageP3A();
        }

        // The check on mNativeInitialized is mostly to ensure that mojo
        // services for wallet are initialized.
        // TODO(sergz): verify do we need it in that phase or not.
        if (mNativeInitialized) {
            BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
            if (layout != null && layout.isWalletIconVisible()) {
                updateWalletBadgeVisibility();
            }
        }

        BraveSafeBrowsingApiHandler.getInstance()
                .setDelegate(BraveActivityJni.get().getSafeBrowsingApiKey(), this);

        // We can store a state of that flag as a browser has to be restarted
        // when the flag state is changed in any case
        mSafeBrowsingFlagEnabled =
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ANDROID_SAFE_BROWSING);

        executeInitSafeBrowsing(0);

        if (ENABLE_IN_APP_UPDATE) {
            if (mAppUpdateManager == null) {
                mAppUpdateManager = AppUpdateManagerFactory.create(BraveActivity.this);
            }
            mAppUpdateManager
                    .getAppUpdateInfo()
                    .addOnSuccessListener(
                            appUpdateInfo -> {
                                if (appUpdateInfo.installStatus() == InstallStatus.DOWNLOADED) {
                                    completeUpdateSnackbar();
                                }
                            });
        }
    }

    @Override
    public void onPauseWithNative() {
        if (mUsageMonitor != null) {
            mUsageMonitor.stop();
        }
        if (BraveVpnUtils.isVpnFeatureSupported(BraveActivity.this)) {
            BraveVpnNativeWorker.getInstance().removeObserver(this);
        }
        super.onPauseWithNative();
    }

    @Override
    public boolean onMenuOrKeyboardAction(int id, boolean fromMenu) {
        final Tab currentTab = getActivityTab();
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
            exitBrave();
        } else if (id == R.id.set_default_browser) {
            BraveSetDefaultBrowserUtils.showBraveSetDefaultBrowserDialog(BraveActivity.this, true);
        } else if (id == R.id.brave_rewards_id) {
            openNewOrSelectExistingTab(BRAVE_REWARDS_SETTINGS_URL);
        } else if (id == R.id.brave_wallet_id) {
            openBraveWallet(false, false, false);
        } else if (id == R.id.brave_playlist_id) {
            openPlaylist(true);
        } else if (id == R.id.add_to_playlist_id) {
            BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
            layout.addMediaToPlaylist();
        } else if (id == R.id.brave_news_id) {
            openBraveNewsSettings();
        } else if (id == R.id.request_brave_vpn_id || id == R.id.request_brave_vpn_check_id) {
            if (!InternetConnection.isNetworkAvailable(BraveActivity.this)) {
                Toast.makeText(BraveActivity.this, R.string.no_internet, Toast.LENGTH_SHORT).show();
            } else {
                if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(BraveActivity.this)) {
                    TimerDialogFragment timerDialogFragment = new TimerDialogFragment();
                    timerDialogFragment.show(getSupportFragmentManager(), TimerDialogFragment.TAG);
                } else {
                    if (BraveVpnNativeWorker.getInstance().isPurchasedUser()) {
                        BraveVpnPrefUtils.setSubscriptionPurchase(true);
                        if (WireguardConfigUtils.isConfigExist(BraveActivity.this)) {
                            BraveVpnProfileUtils.getInstance().startVpn(BraveActivity.this);
                        } else {
                            BraveVpnUtils.openBraveVpnProfileActivity(BraveActivity.this);
                        }
                    } else {
                        BraveVpnUtils.showProgressDialog(
                                BraveActivity.this,
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
        } else if (id == R.id.request_vpn_location_id || id == R.id.request_vpn_location_icon_id) {
            BraveVpnUtils.openVpnServerSelectionActivity(BraveActivity.this);
        } else if (id == R.id.brave_speedreader_id) {
            enableSpeedreaderMode();
        } else if (id == R.id.brave_leo_id) {
            openBraveLeo();
        } else {
            return false;
        }
        return true;
    }

    @Override
    public void cleanUpBraveNewsController() {
        if (mBraveNewsController != null) {
            mBraveNewsController.close();
        }
        mBraveNewsController = null;
    }

    // Handles only wallet related mojo failures. Don't add handlers for mojo connections that
    // are not related to wallet functionality.
    @Override
    public void onConnectionError(MojoException e) {
        cleanUpWalletNativeServices();
        initWalletNativeServices();
    }

    @Override
    protected void onDestroyInternal() {
        if (mNotificationPermissionController != null) {
            NotificationPermissionController.detach(mNotificationPermissionController);
            mNotificationPermissionController = null;
        }

        BraveSafeBrowsingApiHandler.getInstance().shutdownSafeBrowsing();
        if (ENABLE_IN_APP_UPDATE && mAppUpdateManager != null) {
            mAppUpdateManager.unregisterListener(mInstallStateUpdatedListener);
        }
        super.onDestroyInternal();
        cleanUpBraveNewsController();
        cleanUpWalletNativeServices();
        cleanUpMiscAndroidMetrics();
    }

    /**
     * Gets Wallet model for Brave activity. It may be {@code null} if native initialization has not
     * completed yet.
     */
    @Nullable
    public WalletModel getWalletModel() {
        return mWalletModel;
    }

    private void setWalletBadgeVisibility(boolean visible) {
        mWalletBadgeVisible = visible;
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.updateWalletBadgeVisibility(visible);
    }

    private void maybeShowPendingTransactions() {
        if (mWalletModel != null) {
            // Trigger to observer to refresh data to process the pending request.
            mWalletModel.getCryptoModel().refreshTransactions();
        }
    }

    private void maybeShowSignSolTransactionsRequestLayout(
            @NonNull final Runnable openWalletPanelRunnable) {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingSignSolTransactionsRequests(
                requests -> {
                    if (requests != null && requests.length != 0) {
                        openBraveWalletDAppsActivity(
                                BraveWalletDAppsActivity.ActivityType.SIGN_SOL_TRANSACTIONS);
                        return;
                    }
                    maybeShowSignMessageErrorsLayout(openWalletPanelRunnable);
                });
    }

    private void maybeShowSignMessageErrorsLayout(@NonNull final Runnable openWalletPanelRunnable) {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingSignMessageErrors(
                errors -> {
                    if (errors != null && errors.length != 0) {
                        openBraveWalletDAppsActivity(
                                BraveWalletDAppsActivity.ActivityType.SIGN_MESSAGE_ERROR);
                    }
                });
        maybeShowSignMessageRequestLayout(openWalletPanelRunnable);
    }

    private void maybeShowSignMessageRequestLayout(
            @NonNull final Runnable openWalletPanelRunnable) {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingSignMessageRequests(
                requests -> {
                    if (requests != null && requests.length != 0) {
                        BraveWalletDAppsActivity.ActivityType activityType =
                                (requests[0].signData.which() == SignDataUnion.Tag.EthSiweData)
                                        ? BraveWalletDAppsActivity.ActivityType.SIWE_MESSAGE
                                        : BraveWalletDAppsActivity.ActivityType.SIGN_MESSAGE;
                        openBraveWalletDAppsActivity(activityType);
                        return;
                    }
                    maybeShowChainRequestLayout(openWalletPanelRunnable);
                });
    }

    private void maybeShowChainRequestLayout(@NonNull final Runnable openWalletPanelRunnable) {
        assert mJsonRpcService != null;
        mJsonRpcService.getPendingAddChainRequests(
                networks -> {
                    if (networks != null && networks.length != 0) {
                        openBraveWalletDAppsActivity(
                                BraveWalletDAppsActivity.ActivityType.ADD_ETHEREUM_CHAIN);

                        return;
                    }
                    maybeShowSwitchChainRequestLayout(openWalletPanelRunnable);
                });
    }

    private void maybeShowSwitchChainRequestLayout(
            @NonNull final Runnable openWalletPanelRunnable) {
        assert mJsonRpcService != null;
        mJsonRpcService.getPendingSwitchChainRequests(
                requests -> {
                    if (requests != null && requests.length != 0) {
                        openBraveWalletDAppsActivity(
                                BraveWalletDAppsActivity.ActivityType.SWITCH_ETHEREUM_CHAIN);

                        return;
                    }
                    maybeShowAddSuggestTokenRequestLayout(openWalletPanelRunnable);
                });
    }

    private void maybeShowAddSuggestTokenRequestLayout(
            @NonNull final Runnable openWalletPanelRunnable) {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingAddSuggestTokenRequests(
                requests -> {
                    if (requests != null && requests.length != 0) {
                        openBraveWalletDAppsActivity(
                                BraveWalletDAppsActivity.ActivityType.ADD_TOKEN);

                        return;
                    }
                    maybeShowGetEncryptionPublicKeyRequestLayout(openWalletPanelRunnable);
                });
    }

    private void maybeShowGetEncryptionPublicKeyRequestLayout(
            @NonNull final Runnable openWalletPanelRunnable) {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingGetEncryptionPublicKeyRequests(
                requests -> {
                    if (requests != null && requests.length != 0) {
                        openBraveWalletDAppsActivity(
                                BraveWalletDAppsActivity.ActivityType
                                        .GET_ENCRYPTION_PUBLIC_KEY_REQUEST);

                        return;
                    }
                    maybeShowDecryptRequestLayout(openWalletPanelRunnable);
                });
    }

    private void maybeShowDecryptRequestLayout(@NonNull final Runnable openWalletPanelRunnable) {
        assert mBraveWalletService != null;
        mBraveWalletService.getPendingDecryptRequests(
                requests -> {
                    if (requests != null && requests.length != 0) {
                        openBraveWalletDAppsActivity(
                                BraveWalletDAppsActivity.ActivityType.DECRYPT_REQUEST);

                        return;
                    }
                    openWalletPanelRunnable.run();
                });
    }

    public void dismissWalletPanelOrDialog() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.dismissWalletPanelOrDialog();
    }

    public void showWalletPanel(final boolean ignoreWeb3NotificationPreference) {
        showWalletPanel(true, ignoreWeb3NotificationPreference);
    }

    public void showWalletPanel(
            final boolean showPendingTransactions, final boolean ignoreWeb3NotificationPreference) {
        final BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.showWalletIcon(true);
        if (!ignoreWeb3NotificationPreference
                && !BraveWalletPreferences.getPrefWeb3NotificationsEnabled()) {
            return;
        }
        assert mKeyringService != null;
        mKeyringService.isLocked(
                locked -> {
                    if (locked) {
                        if (showPendingTransactions) {
                            layout.showWalletPanel();
                        }
                        return;
                    }
                    mKeyringService.hasPendingUnlockRequest(
                            pending -> {
                                if (pending) {
                                    layout.showWalletPanel();
                                    return;
                                }
                                if (showPendingTransactions && mWalletBadgeVisible) {
                                    maybeShowPendingTransactions();
                                } else {
                                    // Create a runnable that opens the Wallet
                                    // if the pending requests reach the end of the chain
                                    // without returning earlier.
                                    final Runnable openWalletPanelRunnable =
                                            () -> getBraveToolbarLayout().showWalletPanel();
                                    maybeShowSignSolTransactionsRequestLayout(
                                            openWalletPanelRunnable);
                                }
                            });
                });
    }

    public void showWalletOnboarding() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.showWalletIcon(true);
        if (!BraveWalletPreferences.getPrefWeb3NotificationsEnabled()) {
            return;
        }
        layout.showWalletPanel();
    }

    public void walletInteractionDetected(WebContents webContents) {
        Tab tab = getActivityTab();
        if (tab == null
                || !webContents.getLastCommittedUrl().equals(
                        tab.getWebContents().getLastCommittedUrl())) {
            return;
        }
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.showWalletIcon(true);
        updateWalletBadgeVisibility();
    }

    public void showAccountCreation(@CoinType.EnumType int coinType) {
        if (mWalletModel != null) {
            mWalletModel.getDappsModel().addAccountCreationRequest(coinType);
        }
    }

    private void updateWalletBadgeVisibility() {
        if (mWalletModel != null) {
            mWalletModel.getDappsModel().updateWalletBadgeVisibility();
        }
    }

    private void verifySubscription() {
        MutableLiveData<PurchaseModel> _activePurchases = new MutableLiveData();
        LiveData<PurchaseModel> activePurchases = _activePurchases;
        InAppPurchaseWrapper.getInstance()
                .queryPurchases(_activePurchases, InAppPurchaseWrapper.SubscriptionProduct.VPN);
        LiveDataUtil.observeOnce(
                activePurchases,
                activePurchaseModel -> {
                    if (activePurchaseModel != null) {
                        BraveVpnNativeWorker.getInstance()
                                .verifyPurchaseToken(
                                        activePurchaseModel.getPurchaseToken(),
                                        activePurchaseModel.getProductId(),
                                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT,
                                        getPackageName());
                    } else {
                        BraveVpnApiResponseUtils.queryPurchaseFailed(BraveActivity.this);
                        if (!mIsVerification) {
                            BraveVpnUtils.openBraveVpnPlansActivity(BraveActivity.this);
                        }
                    }
                });
    }

    @Override
    public boolean onOptionsItemSelected(int itemId, @Nullable Bundle menuItemData) {
        if (itemId == R.id.new_tab_menu_id) {
            LayoutManagerChrome layoutManager =
                    (LayoutManagerChrome)
                            BraveReflectionUtil.getField(
                                    ChromeTabbedActivity.class, "mLayoutManager", this);
            if (layoutManager != null
                    && layoutManager.getHubLayoutForTesting() != null
                    && !layoutManager.getHubLayoutForTesting().isActive()
                    && mMiscAndroidMetrics != null) {
                mMiscAndroidMetrics.recordAppMenuNewTab();
            }
        }
        return super.onOptionsItemSelected(itemId, menuItemData);
    }

    @Override
    public void onVerifyPurchaseToken(
            String jsonResponse, String purchaseToken, String productId, boolean isSuccess) {
        if (isSuccess) {
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            int paymentState = BraveVpnUtils.getPaymentState(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setPurchaseToken(purchaseToken);
                BraveVpnPrefUtils.setProductId(productId);
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
            braveVpnProfileIntent.setAction(Intent.ACTION_VIEW);
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
            List<Integer> dataTypes =
                    Arrays.asList(
                            BrowsingDataType.HISTORY,
                            BrowsingDataType.SITE_DATA,
                            BrowsingDataType.CACHE);

            int[] dataTypesArray = CollectionUtil.integerCollectionToIntArray(dataTypes);

            // has onBrowsingDataCleared() as an @Override callback from implementing
            // BrowsingDataBridge.OnClearBrowsingDataListener
            BrowsingDataBridge.getForProfile(getCurrentProfile())
                    .clearBrowsingData(this, dataTypesArray, TimePeriod.ALL_TIME);
        }

        setLoadedFeed(false);
        setComesFromNewTab(false);
        setNewsItemsFeedCards(null);
        BraveSearchEngineUtils.initializeBraveSearchEngineStates(getTabModelSelector());
        Intent intent = getIntent();
        if (intent != null
                && intent.getBooleanExtra(BraveWalletActivity.RESTART_WALLET_ACTIVITY, false)) {
            openBraveWallet(
                    false,
                    intent.getBooleanExtra(
                            BraveWalletActivity.RESTART_WALLET_ACTIVITY_SETUP, false),
                    intent.getBooleanExtra(
                            BraveWalletActivity.RESTART_WALLET_ACTIVITY_RESTORE, false));
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
        mIsDefaultCheckOnResume = true;
    }

    @Override
    public void onResume() {
        super.onResume();
        mIsProcessingPendingDappsTxRequest = false;
        if (mIsDefaultCheckOnResume) {
            mIsDefaultCheckOnResume = false;

            if (BraveSetDefaultBrowserUtils.isBraveSetAsDefaultBrowser(this)) {
                BraveSetDefaultBrowserUtils.setBraveDefaultSuccess();
            }
        }

        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> { BraveStatsUtil.removeShareStatsFile(); });

        // We need to enable widget promo for later release
        /* int appOpenCountForWidgetPromo = SharedPreferencesManager.getInstance().readInt(
                BravePreferenceKeys.BRAVE_APP_OPEN_COUNT_FOR_WIDGET_PROMO);
        if (appOpenCountForWidgetPromo < APP_OPEN_COUNT_FOR_WIDGET_PROMO) {
            SharedPreferencesManager.getInstance().writeInt(
                    BravePreferenceKeys.BRAVE_APP_OPEN_COUNT_FOR_WIDGET_PROMO,
                    appOpenCountForWidgetPromo + 1);
        } */
        if (mUsageMonitor != null) {
            mUsageMonitor.start();
        }
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
        String captchaID =
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getString(BravePref.SCHEDULED_CAPTCHA_ID);
        String paymentID =
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getString(BravePref.SCHEDULED_CAPTCHA_PAYMENT_ID);
        if (BraveQAPreferences.shouldVlogRewards()) {
            Log.e(
                    AdaptiveCaptchaHelper.TAG,
                    "captchaID : " + captchaID + " Payment ID : " + paymentID);
        }
        maybeSolveAdaptiveCaptcha();
    }

    @Override
    public void turnSafeBrowsingOff() {
        SafeBrowsingBridge safeBrowsingBridge = new SafeBrowsingBridge(getCurrentProfile());
        safeBrowsingBridge.setSafeBrowsingState(SafeBrowsingState.NO_SAFE_BROWSING);
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
        String captchaID =
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getString(BravePref.SCHEDULED_CAPTCHA_ID);
        String paymentID =
                UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getString(BravePref.SCHEDULED_CAPTCHA_PAYMENT_ID);
        if (!TextUtils.isEmpty(captchaID)
                && !TextUtils.isEmpty(paymentID)
                && !BravePrefServiceBridge.getInstance().getSafetynetCheckFailed()) {
            AdaptiveCaptchaHelper.startAttestation(captchaID, paymentID);
        }
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        boolean isFirstInstall = PackageUtils.isFirstInstall(this);

        BraveVpnNativeWorker.getInstance().reloadPurchasedState();

        BraveHelper.maybeMigrateSettings();

        PrefChangeRegistrar mPrefChangeRegistrar = new PrefChangeRegistrar();
        mPrefChangeRegistrar.addObserver(BravePref.SCHEDULED_CAPTCHA_ID, this);

        if (UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                        .getInteger(BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS)
                >= MAX_FAILED_CAPTCHA_ATTEMPTS) {
            UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .setBoolean(BravePref.SCHEDULED_CAPTCHA_PAUSED, true);
        }

        if (BraveQAPreferences.shouldVlogRewards()) {
            Log.e(
                    AdaptiveCaptchaHelper.TAG,
                    "Failed attempts : "
                            + UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                                    .getInteger(BravePref.SCHEDULED_CAPTCHA_FAILED_ATTEMPTS));
        }
        if (!UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                .getBoolean(BravePref.SCHEDULED_CAPTCHA_PAUSED)) {
            maybeSolveAdaptiveCaptcha();
        }

        if (ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_DOUBLE_RESTART, false)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_DOUBLE_RESTART, false);
            BraveRelaunchUtils.restart();
            return;
        }

        // Make sure this option is disabled
        if (PreloadPagesSettingsBridge.getState(getCurrentProfile())
                != PreloadPagesState.NO_PRELOADING) {
            PreloadPagesSettingsBridge.setState(
                    getCurrentProfile(), PreloadPagesState.NO_PRELOADING);
        }

        if (BraveRewardsHelper.hasRewardsEnvChange()) {
            BravePrefServiceBridge.getInstance().resetPromotionLastFetchStamp();
            BraveRewardsHelper.setRewardsEnvChange(false);
        }

        int appOpenCount =
                ChromeSharedPreferences.getInstance()
                        .readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT);
        ChromeSharedPreferences.getInstance()
                .writeInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT, appOpenCount + 1);

        if (isFirstInstall && appOpenCount == 0) {
            checkForYandexSE();
            enableSearchSuggestions();
        }

        migrateBgPlaybackToFeature();

        Context app = ContextUtils.getApplicationContext();
        if (null != app
                && BraveReflectionUtil.equalTypes(this.getClass(), ChromeTabbedActivity.class)) {
            // Trigger BraveSyncWorker CTOR to make migration from sync v1 if sync is enabled
            BraveSyncWorker.get();
        }

        initMiscAndroidMetrics();
        checkForNotificationData();

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
        //
        // SharedPreferencesManager.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
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

        if (ChromeSharedPreferences.getInstance().readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                == 1) {
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
        BraveSyncAccountDeletedInformer.show();

        if (!OnboardingPrefManager.getInstance().isOneTimeNotificationStarted() && isFirstInstall) {
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

        if (isFirstInstall
                && ChromeSharedPreferences.getInstance()
                                .readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                        == 1) {
            Calendar calender = Calendar.getInstance();
            calender.setTime(new Date());
            calender.add(Calendar.DATE, DAYS_4);
            BraveRewardsHelper.setNextRewardsOnboardingModalDate(calender.getTimeInMillis());
        }

        if (!mIsSetDefaultBrowserNotification) {
            BraveSetDefaultBrowserUtils.checkSetDefaultBrowserModal(this);
        }

        checkFingerPrintingOnUpgrade(isFirstInstall);
        checkForVpnCallout(isFirstInstall);

        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_VPN_LINK_SUBSCRIPTION_ANDROID_UI)
                && BraveVpnPrefUtils.isSubscriptionPurchase()
                && !BraveVpnPrefUtils.isLinkSubscriptionDialogShown()) {
            showLinkVpnSubscriptionDialog();
        }
        if (isFirstInstall
                && (OnboardingPrefManager.getInstance().isDormantUsersEngagementEnabled()
                        || getPackageName().equals(BraveConstants.BRAVE_PRODUCTION_PACKAGE_NAME))) {
            OnboardingPrefManager.getInstance().setDormantUsersPrefs();
            if (!OnboardingPrefManager.getInstance().isDormantUsersNotificationsStarted()) {
                RetentionNotificationUtil.scheduleDormantUsersNotifications(this);
                OnboardingPrefManager.getInstance().setDormantUsersNotificationsStarted(true);
            }
        }
        initWalletNativeServices();

        mNativeInitialized = true;

        String countryCode = Locale.getDefault().getCountry();
        if (countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_AD_FREE_CALLOUT_DIALOG, true)
                && getActivityTab() != null
                && getActivityTab().getUrl().getSpec() != null
                && UrlUtilities.isNtpUrl(getActivityTab().getUrl().getSpec())
                && (ChromeSharedPreferences.getInstance()
                                .readBoolean(BravePreferenceKeys.BRAVE_OPENED_YOUTUBE, false)
                        || ChromeSharedPreferences.getInstance()
                                        .readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                                >= 7)) {
            showAdFreeCalloutDialog();
        }

        initBraveNewsController();
        if (ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_PLAYLIST, false)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_PLAYLIST, false);
            openPlaylist(false);
        } else if (ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_VPN, false)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePreferenceKeys.BRAVE_DEFERRED_DEEPLINK_VPN, false);
            handleDeepLinkVpn();
        } else if (!mIsDeepLink
                && OnboardingPrefManager.getInstance().isOnboardingSearchBoxTooltip()
                && getActivityTab() != null
                && getActivityTab().getUrl().getSpec() != null
                && UrlUtilities.isNtpUrl(getActivityTab().getUrl().getSpec())) {
            showSearchBoxTooltip();
        }

        // Added to reset app links settings for upgrade case
        if (!isFirstInstall
                && !ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePrivacySettings.PREF_APP_LINKS, true)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePrivacySettings.PREF_APP_LINKS_RESET, true)) {
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePrivacySettings.PREF_APP_LINKS, true);
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(BravePrivacySettings.PREF_APP_LINKS_RESET, false);
        }

        if (isFirstInstall
                && ChromeSharedPreferences.getInstance()
                                .readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                        == 1) {
            Calendar calendar = Calendar.getInstance();
            calendar.setTime(new Date());
            calendar.add(Calendar.DATE, DAYS_7);
            BraveRewardsHelper.setRewardsOnboardingIconTiming(calendar.getTimeInMillis());

            if (ENABLE_IN_APP_UPDATE) {
                setInAppUpdateTiming();
            }
        }

        // Check multiwindow toggle for upgrade case
        if (!isFirstInstall
                && !BraveMultiWindowUtils.isCheckUpgradeEnableMultiWindows()
                && MultiWindowUtils.getInstanceCount() > 1
                && !BraveMultiWindowUtils.shouldEnableMultiWindows()) {
            BraveMultiWindowUtils.setCheckUpgradeEnableMultiWindows(true);
            BraveMultiWindowUtils.updateEnableMultiWindows(true);
        } else if (!BraveMultiWindowUtils.isCheckUpgradeEnableMultiWindows()) {
            BraveMultiWindowUtils.setCheckUpgradeEnableMultiWindows(true);
        }

        if (ENABLE_IN_APP_UPDATE
                && System.currentTimeMillis()
                        > ChromeSharedPreferences.getInstance()
                                .readLong(BravePreferenceKeys.BRAVE_IN_APP_UPDATE_TIMING, 0)) {
            checkAppUpdate();
        }
        new KeyboardVisibilityHelper(BraveActivity.this, BraveActivity.this);
    }

    private void enableSearchSuggestions() {
        TemplateUrl defaultSearchEngineTemplateUrl =
                BraveSearchEngineUtils.getTemplateUrlByShortName(
                        getCurrentProfile(),
                        BraveSearchEngineUtils.getDSEShortName(getCurrentProfile(), false));
        if (BRAVE_SEARCH_ENGINE_KEYWORD.equals(defaultSearchEngineTemplateUrl.getKeyword())) {
            UserPrefs.get(getCurrentProfile()).setBoolean(Pref.SEARCH_SUGGEST_ENABLED, true);
        }
    }

    private void setInAppUpdateTiming() {
        Calendar calendar = Calendar.getInstance();
        calendar.setTime(new Date());
        calendar.add(Calendar.MONTH, MONTH_1);
        ChromeSharedPreferences.getInstance()
                .writeLong(
                        BravePreferenceKeys.BRAVE_IN_APP_UPDATE_TIMING, calendar.getTimeInMillis());
    }

    private void completeUpdateSnackbar() {
        Snackbar snackbar =
                Snackbar.make(
                                getResources().getString(R.string.in_app_update_text),
                                new SnackbarController() {
                                    @Override
                                    public void onDismissNoAction(Object actionData) {}

                                    @Override
                                    public void onAction(Object actionData) {
                                        if (mAppUpdateManager != null) {
                                            mAppUpdateManager.completeUpdate();
                                            mAppUpdateManager.unregisterListener(
                                                    mInstallStateUpdatedListener);
                                        }
                                    }
                                },
                                Snackbar.TYPE_ACTION,
                                Snackbar.UMA_UNKNOWN)
                        .setAction(getResources().getString(R.string.update), null)
                        .setSingleLine(false)
                        .setDuration(10000);
        SnackbarManager snackbarManager =
                SnackbarManagerProvider.from(getActivityTab().getWindowAndroid());
        snackbarManager.showSnackbar(snackbar);
    }

    private final InstallStateUpdatedListener mInstallStateUpdatedListener =
            installState -> {
                if (installState.installStatus() == InstallStatus.DOWNLOADED) {
                    completeUpdateSnackbar();
                }
            };

    private void checkAppUpdate() {
        mAppUpdateManager = AppUpdateManagerFactory.create(BraveActivity.this);
        mAppUpdateManager.registerListener(mInstallStateUpdatedListener);

        Task<AppUpdateInfo> appUpdateInfoTask = mAppUpdateManager.getAppUpdateInfo();

        appUpdateInfoTask.addOnSuccessListener(
                appUpdateInfo -> {
                    if (appUpdateInfo.updateAvailability() == UpdateAvailability.UPDATE_AVAILABLE) {
                        if (appUpdateInfo.updatePriority() >= 4 /* high priority */
                                && appUpdateInfo.isUpdateTypeAllowed(AppUpdateType.IMMEDIATE)) {
                            startAppUpdateFlow(appUpdateInfo, AppUpdateType.IMMEDIATE);
                        } else {
                            startAppUpdateFlow(appUpdateInfo, AppUpdateType.FLEXIBLE);
                        }
                    }
                });
    }

    private void startAppUpdateFlow(AppUpdateInfo appUpdateInfo, int appUpdateType) {
        try {
            mAppUpdateManager.startUpdateFlowForResult(
                    appUpdateInfo, appUpdateType, BraveActivity.this, 1);
            setInAppUpdateTiming();
        } catch (IntentSender.SendIntentException e) {
            throw new RuntimeException(e);
        }
    }

    private void handleDeepLinkVpn() {
        mIsDeepLink = true;
        BraveVpnUtils.openBraveVpnPlansActivity(this);
    }

    private void checkForVpnCallout(boolean isFirstInstall) {
        String countryCode = Locale.getDefault().getCountry();

        if (!countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)
                && BraveVpnUtils.isVpnFeatureSupported(BraveActivity.this)) {
            if (!TextUtils.isEmpty(BraveVpnPrefUtils.getPurchaseToken())
                    && !TextUtils.isEmpty(BraveVpnPrefUtils.getProductId())) {
                mIsVerification = true;
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                        BraveVpnPrefUtils.getPurchaseToken(), BraveVpnPrefUtils.getProductId(),
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
            }
        }
    }

    @Override
    public void initBraveNewsController() {
        if (mBraveNewsController != null) {
            return;
        }
        if (mBraveNewsConnectionErrorHandler == null) {
            mBraveNewsConnectionErrorHandler = new BraveNewsConnectionErrorHandler(this);
        }

        if (BravePrefServiceBridge.getInstance().getShowNews()
                && BravePrefServiceBridge.getInstance().getNewsOptIn()) {
            mBraveNewsController = BraveNewsControllerFactory.getInstance().getBraveNewsController(
                    mBraveNewsConnectionErrorHandler);

            BraveNewsUtils.getBraveNewsSettingsData(mBraveNewsController, null);
        }
    }

    private void migrateBgPlaybackToFeature() {
        if (ChromeSharedPreferences.getInstance()
                .readBoolean(
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
        ChromeSharedPreferences.getInstance()
                .writeBoolean(
                        BravePreferenceKeys.BRAVE_BACKGROUND_VIDEO_PLAYBACK_CONVERTED_TO_FEATURE,
                        true);
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
        new Handler()
                .postDelayed(
                        () -> {
                            PopupWindowTooltip popupWindowTooltip =
                                    new PopupWindowTooltip.Builder(this)
                                            .anchorView(anchorView)
                                            .arrowColor(getColor(R.color.onboarding_arrow_color))
                                            .gravity(Gravity.BOTTOM)
                                            .dismissOnOutsideTouch(true)
                                            .dismissOnInsideTouch(false)
                                            .backgroundDimDisabled(true)
                                            .contentArrowAtStart(!isTablet)
                                            .padding(padding)
                                            .parentPaddingHorizontal(dpToPx(this, 10))
                                            .onDismissListener(
                                                    tooltip -> {
                                                        if (viewGroup != null
                                                                && highlightView != null) {
                                                            viewGroup.removeView(highlightView);
                                                        }
                                                    })
                                            .modal(true)
                                            .contentView(R.layout.brave_onboarding_searchbox)
                                            .build();

                            String countryCode = Locale.getDefault().getCountry();
                            if (countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)) {
                                TextView toolTipBody =
                                        popupWindowTooltip.findViewById(R.id.tv_tooltip_title);
                                toolTipBody.setText(
                                        getResources()
                                                .getString(R.string.searchbox_onboarding_india));
                            }
                            viewGroup.addView(highlightView);
                            HighlightItem item = new HighlightItem(anchorView);
                            highlightView.setHighlightTransparent(true);
                            highlightView.setHighlightItem(item);
                            popupWindowTooltip.show();
                        },
                        500);
    }

    public void setDormantUsersPrefs() {
        OnboardingPrefManager.getInstance().setDormantUsersPrefs();
        RetentionNotificationUtil.scheduleDormantUsersNotifications(this);
    }

    private void openPlaylist(boolean shouldHandlePlaylistActivity) {
        if (!shouldHandlePlaylistActivity) mIsDeepLink = true;

        if (ChromeSharedPreferences.getInstance()
                .readBoolean(PlaylistPreferenceUtils.SHOULD_SHOW_PLAYLIST_ONBOARDING, true)) {
            PlaylistUtils.openPlaylistMenuOnboardingActivity(BraveActivity.this);
            ChromeSharedPreferences.getInstance()
                    .writeBoolean(PlaylistPreferenceUtils.SHOULD_SHOW_PLAYLIST_ONBOARDING, false);
        } else if (shouldHandlePlaylistActivity) {
            openPlaylistActivity(BraveActivity.this, ConstantUtils.ALL_PLAYLIST);
        }
    }

    public void openPlaylistActivity(Context context, String playlistId) {
        Intent playlistActivityIntent = new Intent(context, PlaylistHostActivity.class);
        playlistActivityIntent.putExtra(ConstantUtils.PLAYLIST_ID, playlistId);
        playlistActivityIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        playlistActivityIntent.setAction(Intent.ACTION_VIEW);
        context.startActivity(playlistActivityIntent);
    }

    private void showLinkVpnSubscriptionDialog() {
        LinkVpnSubscriptionDialogFragment linkVpnSubscriptionDialogFragment =
                new LinkVpnSubscriptionDialogFragment();
        linkVpnSubscriptionDialogFragment.setCancelable(false);
        linkVpnSubscriptionDialogFragment.show(
                getSupportFragmentManager(), "LinkVpnSubscriptionDialogFragment");
    }

    private void showAdFreeCalloutDialog() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_AD_FREE_CALLOUT_DIALOG, false);

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

    private void checkFingerPrintingOnUpgrade(boolean isFirstInstall) {
        if (!isFirstInstall
                && ChromeSharedPreferences.getInstance()
                                .readInt(BravePreferenceKeys.BRAVE_APP_OPEN_COUNT)
                        == 0) {
            boolean value =
                    ChromeSharedPreferences.getInstance()
                            .readBoolean(BravePrivacySettings.PREF_FINGERPRINTING_PROTECTION, true);
            if (value) {
                BraveShieldsContentSettings.setShieldsValue(
                        ProfileManager.getLastUsedRegularProfile(),
                        "",
                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                        BraveShieldsContentSettings.DEFAULT,
                        false);
            } else {
                BraveShieldsContentSettings.setShieldsValue(
                        ProfileManager.getLastUsedRegularProfile(),
                        "",
                        BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                        BraveShieldsContentSettings.ALLOW_RESOURCE,
                        false);
            }
        }
    }

    public void openQuickSearchEnginesSettings() {
        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(this, QuickSearchEnginesFragment.class);
    }

    public void openBravePlaylistSettings() {
        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(this, BravePlaylistPreferences.class);
    }

    public void openBraveNewsSettings() {
        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(this, BraveNewsPreferencesV2.class);
    }

    public void openBraveContentFilteringSettings() {
        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(this, ContentFilteringFragment.class);
    }

    public void openBraveWalletSettings() {
        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(this, BraveWalletPreferences.class);
    }

    public void openBraveConnectedSitesSettings() {
        SettingsNavigation settingsLauncher = SettingsNavigationFactory.createSettingsNavigation();
        settingsLauncher.startSettings(this, BraveWalletEthereumConnectedSites.class);
    }

    public void openBraveWallet(boolean fromDapp, boolean setupAction, boolean restoreAction) {
        Intent braveWalletIntent = new Intent(this, BraveWalletActivity.class);
        braveWalletIntent.putExtra(BraveWalletActivity.IS_FROM_DAPPS, fromDapp);
        braveWalletIntent.putExtra(BraveWalletActivity.RESTART_WALLET_ACTIVITY_SETUP, setupAction);
        braveWalletIntent.putExtra(
                BraveWalletActivity.RESTART_WALLET_ACTIVITY_RESTORE, restoreAction);
        braveWalletIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        braveWalletIntent.setAction(Intent.ACTION_VIEW);
        startActivity(braveWalletIntent);
    }

    public void openBraveWalletBackup() {
        Intent braveWalletIntent = new Intent(this, BraveWalletActivity.class);
        braveWalletIntent.putExtra(BraveWalletActivity.SHOW_WALLET_ACTIVITY_BACKUP, true);
        braveWalletIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        braveWalletIntent.setAction(Intent.ACTION_VIEW);
        startActivity(braveWalletIntent);
    }

    public void viewOnBlockExplorer(
            String address, @CoinType.EnumType int coinType, NetworkInfo networkInfo) {
        Utils.openAddress("/address/" + address, this, coinType, networkInfo);
    }

    public void openBraveWalletDAppsActivity(
            @NonNull final BraveWalletDAppsActivity.ActivityType activityType) {
        Intent braveWalletIntent = new Intent(this, BraveWalletDAppsActivity.class);
        braveWalletIntent.putExtra("activityType", activityType.getValue());
        braveWalletIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        braveWalletIntent.setAction(Intent.ACTION_VIEW);
        startActivity(braveWalletIntent);
    }

    public MiscAndroidMetrics getMiscAndroidMetrics() {
        return mMiscAndroidMetrics;
    }

    private void checkForYandexSE() {
        String countryCode = Locale.getDefault().getCountry();
        if (sYandexRegions.contains(countryCode)) {
            Profile lastUsedRegularProfile = ProfileManager.getLastUsedRegularProfile();
            TemplateUrl yandexTemplateUrl =
                    BraveSearchEngineUtils.getTemplateUrlByShortName(
                            lastUsedRegularProfile, OnboardingPrefManager.YANDEX);
            if (yandexTemplateUrl != null) {
                BraveSearchEngineUtils.setDSEPrefs(yandexTemplateUrl, lastUsedRegularProfile);
                BraveSearchEngineUtils.setDSEPrefs(
                        yandexTemplateUrl,
                        lastUsedRegularProfile.getPrimaryOTRProfile(/* createIfNeeded= */ true));
            }
        }
    }

    private BraveNotificationWarningDialog.DismissListener mCloseDialogListener =
            new BraveNotificationWarningDialog.DismissListener() {
                @Override
                public void onDismiss() {
                    checkForNotificationData();
                }
            };

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
                    if (getActivityTab() != null
                            && getActivityTab().getUrl().getSpec() != null
                            && !UrlUtilities.isNtpUrl(getActivityTab().getUrl().getSpec())) {
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
                        mIsSetDefaultBrowserNotification = true;
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
            if (getActivityTab() != null
                    && getActivityTab().getUrl().getSpec() != null
                    && !UrlUtilities.isNtpUrl(getActivityTab().getUrl().getSpec())) {
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
        layout.hideRewardsOnboardingIcon();
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
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_CLOSE_TABS_ON_EXIT, false);
    }

    private boolean isClearBrowsingDataOnExit() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_CLEAR_ON_EXIT, false);
    }

    public void onRewardsPanelDismiss() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.onRewardsPanelDismiss();
    }

    public void dismissRewardsPanel() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.dismissRewardsPanel();
    }

    public void dismissShieldsTooltip() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.dismissShieldsTooltip();
    }

    public void openRewardsPanel() {
        BraveToolbarLayoutImpl layout = getBraveToolbarLayout();
        layout.openRewardsPanel();
    }

    public Profile getCurrentProfile() {
        Tab tab = getActivityTab();
        if (tab == null) {
            return ProfileManager.getLastUsedRegularProfile();
        }

        return Profile.fromWebContents(tab.getWebContents());
    }

    /** Close all tabs (including active tab) whose URL origin matches with a given origin. */
    public void closeAllTabsByOrigin(@NonNull final String origin) {
        final TabModel tabModel = getCurrentTabModel();

        Set<Integer> tabIndexes = getTabIndexesByUrlOrigin(tabModel, origin);
        for (Integer index : tabIndexes) {
            Tab tab = tabModel.getTabAt(index);
            if (tab != null) {
                tab.setClosing(true);
                tabModel.closeTabs(TabClosureParams.closeTab(tab).build());
            }
        }
    }

    /**
     * Selects an existing tab if it matches a given origin, marks it as active and returns it.
     *
     * @return Active tab if it exists, {@code null} otherwise.
     */
    public Tab selectExistingUrlOriginTab(@NonNull final String origin) {
        TabModel tabModel = getCurrentTabModel();
        Set<Integer> tabIndexes = getTabIndexesByUrlOrigin(tabModel, origin);

        // Find if tab exists, including tab already active.
        if (!tabIndexes.isEmpty()) {
            int index = tabIndexes.iterator().next();
            Tab tab = tabModel.getTabAt(tabIndexes.iterator().next());
            // Set active tab
            tabModel.setIndex(index, TabSelectionType.FROM_USER);
            return tab;
        } else {
            return null;
        }
    }

    /**
     * Find the {@link Tab} indexes whose URL starts with the specified base URL.
     *
     * @param model The {@link TabModel} to act on.
     * @param origin The URL origin to search for.
     * @return A set of indexes pointing to the matching {@link Tab}s or empty set if no matches are
     *     found.
     */
    @NonNull
    private static Set<Integer> getTabIndexesByUrlOrigin(
            @NonNull final TabList model, @NonNull final String origin) {
        final Set<Integer> result = new HashSet<>();
        int count = model.getCount();

        for (int i = 0; i < count; i++) {
            if (model.getTabAt(i).getUrl().getOrigin().getSpec().contentEquals(origin)) {
                result.add(i);
            }
        }
        return result;
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
            tabModel.setIndex(tabIndex, TabSelectionType.FROM_USER);
            return tab;
        } else {
            return null;
        }
    }

    public Tab openNewOrSelectExistingTab(String url, boolean refresh) {
        Tab tab = selectExistingTab(url);
        if (tab != null) {
            if (refresh) {
                tab.reload();
            }
            return tab;
        } else { // Open a new tab
            return getTabCreator(false).launchUrl(url, TabLaunchType.FROM_CHROME_UI);
        }
    }

    public void openNewOrRefreshExistingTab(
            @NonNull final String origin, @NonNull final String url) {
        Tab tab = selectExistingUrlOriginTab(origin);
        if (tab != null) {
            tab.reload();
        } else {
            // Open a new tab.
            getTabCreator(false).launchUrl(url, TabLaunchType.FROM_CHROME_UI);
        }
    }

    public Tab openNewOrSelectExistingTab(String url) {
        return openNewOrSelectExistingTab(url, false);
    }

    private void clearWalletModelServices() {
        if (mWalletModel == null) {
            return;
        }

        mWalletModel.resetServices(
                getApplicationContext(), null, null, null, null, null, null, null, null, null);
    }

    private void setupWalletModel() {
        PostTask.postTask(TaskTraits.UI_DEFAULT, () -> {
            if (mWalletModel == null) {
                mWalletModel = new WalletModel(getApplicationContext(), mKeyringService,
                        mBlockchainRegistry, mJsonRpcService, mTxService, mEthTxManagerProxy,
                        mSolanaTxManagerProxy, mAssetRatioService, mBraveWalletService,
                        mSwapService);
            } else {
                mWalletModel.resetServices(getApplicationContext(), mKeyringService,
                        mBlockchainRegistry, mJsonRpcService, mTxService, mEthTxManagerProxy,
                        mSolanaTxManagerProxy, mAssetRatioService, mBraveWalletService,
                        mSwapService);
            }
            setupObservers();
        });
    }

    @MainThread
    private void setupObservers() {
        ThreadUtils.assertOnUiThread();
        clearObservers();
        mWalletModel.getCryptoModel().getPendingTxHelper().mSelectedPendingRequest.observe(
                this, transactionInfo -> {
                    if (transactionInfo == null) {
                        return;
                    }
                    // don't show dapps panel if the wallet is locked and requests are being
                    // processed by the approve dialog already
                    mKeyringService.isLocked(locked -> {
                        if (locked) {
                            return;
                        }

                        if (!mIsProcessingPendingDappsTxRequest) {
                            mIsProcessingPendingDappsTxRequest = true;
                            openBraveWalletDAppsActivity(
                                    BraveWalletDAppsActivity.ActivityType.CONFIRM_TRANSACTION);
                        }

                        // update badge if there's a pending tx
                        updateWalletBadgeVisibility();
                    });
                });

        mWalletModel.getDappsModel().mWalletIconNotificationVisible.observe(
                this, this::setWalletBadgeVisibility);

        mWalletModel.getDappsModel().mPendingWalletAccountCreationRequest.observe(this, request -> {
            if (request == null) return;
            mWalletModel.getKeyringModel().isWalletLocked(isLocked -> {
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
                        Intent intent = AddAccountActivity.createIntentToAddAccount(
                                this, info.getCoinType());
                        startActivity(intent);
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
                                    .setPositiveButton(R.string.brave_action_yes,
                                            (dialog, which) -> {
                                                mWalletModel.createAccountAndSetDefaultNetwork(
                                                        networkInfo);
                                            })
                                    .setNegativeButton(
                                            R.string.brave_action_no, (dialog, which) -> {
                                                mWalletModel.getCryptoModel()
                                                        .getNetworkModel()
                                                        .clearCreateAccountState();
                                                dialog.dismiss();
                                            });
                    builder.show();
                });
    }

    @MainThread
    private void clearObservers() {
        ThreadUtils.assertOnUiThread();
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
        mCrossPromotionalModalDialogFragment.show(
                getSupportFragmentManager(), "CrossPromotionalModalDialogFragment");
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

    private static Activity getActivityOfType(Class<?> classOfActivity) {
        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!classOfActivity.isInstance(ref)) continue;

            return ref;
        }

        return null;
    }

    private void enableSpeedreaderMode() {
        final Tab currentTab = getActivityTab();
        if (currentTab != null) {
            BraveSpeedReaderUtils.enableSpeedreaderMode(currentTab);
        }
    }

    public void openBraveLeo() {
        BraveLeoUtils.verifySubscription(null);
        BraveLeoActivity.showPage(this, BRAVE_AI_CHAT_URL);
    }

    public static ChromeTabbedActivity getChromeTabbedActivity() {
        return (ChromeTabbedActivity) getActivityOfType(ChromeTabbedActivity.class);
    }

    public static CustomTabActivity getCustomTabActivity() {
        return (CustomTabActivity) getActivityOfType(CustomTabActivity.class);
    }

    @NonNull
    public static BraveActivity getBraveActivity() throws BraveActivityNotFoundException {
        BraveActivity activity = (BraveActivity) getActivityOfType(BraveActivity.class);
        if (activity != null) {
            return activity;
        }

        throw new BraveActivityNotFoundException("BraveActivity Not Found");
    }

    @NonNull
    public static BraveActivity getBraveActivityFromTaskId(int taskId)
            throws BraveActivityNotFoundException {

        for (Activity ref : ApplicationStatus.getRunningActivities()) {
            if (!BraveActivity.class.isInstance(ref) || ref.getTaskId() != taskId) continue;

            return (BraveActivity) ref;
        }

        throw new BraveActivityNotFoundException("BraveActivity Not Found");
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
            setUpdatePreferences();
        }

        return super.maybeDispatchLaunchIntent(intent, savedInstanceState);
    }

    private void setUpdatePreferences() {
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
        String getSafeBrowsingApiKey();
    }

    private void initBraveWalletService() {
        if (mBraveWalletService != null) {
            return;
        }

        mBraveWalletService = BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
    }

    private void initKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = BraveWalletServiceFactory.getInstance().getKeyringService(this);
    }

    private void initJsonRpcService() {
        if (mJsonRpcService != null) {
            return;
        }

        mJsonRpcService = BraveWalletServiceFactory.getInstance().getJsonRpcService(this);
    }

    private void initTxService() {
        if (mTxService != null) {
            return;
        }

        mTxService = BraveWalletServiceFactory.getInstance().getTxService(this);
    }

    private void initEthTxManagerProxy() {
        if (mEthTxManagerProxy != null) {
            return;
        }

        mEthTxManagerProxy = BraveWalletServiceFactory.getInstance().getEthTxManagerProxy(this);
    }

    private void initSolanaTxManagerProxy() {
        if (mSolanaTxManagerProxy != null) {
            return;
        }

        mSolanaTxManagerProxy =
                BraveWalletServiceFactory.getInstance().getSolanaTxManagerProxy(this);
    }

    private void initBlockchainRegistry() {
        if (mBlockchainRegistry != null) {
            return;
        }

        mBlockchainRegistry = BlockchainRegistryFactory.getInstance().getBlockchainRegistry(this);
    }

    private void initAssetRatioService() {
        if (mAssetRatioService != null) {
            return;
        }

        mAssetRatioService = AssetRatioServiceFactory.getInstance().getAssetRatioService(this);
    }

    @Override
    public void initMiscAndroidMetricsFromAWorkerThread() {
        runOnUiThread(
                () -> {
                    initMiscAndroidMetrics();
                });
    }

    private void initMiscAndroidMetrics() {
        ThreadUtils.assertOnUiThread();
        if (mMiscAndroidMetrics != null) {
            return;
        }
        if (mMiscAndroidMetricsConnectionErrorHandler == null) {
            mMiscAndroidMetricsConnectionErrorHandler =
                    new MiscAndroidMetricsConnectionErrorHandler(this);
        }

        MiscAndroidMetricsFactory.getInstance()
                .getMetricsService(mMiscAndroidMetricsConnectionErrorHandler)
                .then(
                        miscAndroidMetrics -> {
                            mMiscAndroidMetrics = miscAndroidMetrics;
                            mMiscAndroidMetrics.recordPrivacyHubEnabledStatus(
                                    OnboardingPrefManager.getInstance().isBraveStatsEnabled());
                            mMiscAndroidMetrics.recordSetAsDefault(
                                    BraveSetDefaultBrowserUtils.isAppSetAsDefaultBrowser(
                                            BraveActivity.this));
                            if (mUsageMonitor == null) {
                                mUsageMonitor = UsageMonitor.getInstance(mMiscAndroidMetrics);
                            }
                            mUsageMonitor.start();
                        });
    }

    private void initSwapService() {
        if (mSwapService != null) {
            return;
        }
        mSwapService = SwapServiceFactory.getInstance().getSwapService(this);
    }

    private void initWalletNativeServices() {
        initBlockchainRegistry();
        initTxService();
        initEthTxManagerProxy();
        initSolanaTxManagerProxy();
        initAssetRatioService();
        initBraveWalletService();
        initKeyringService();
        initJsonRpcService();
        initSwapService();
        setupWalletModel();
    }

    private void cleanUpWalletNativeServices() {
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

    @Override
    public void cleanUpMiscAndroidMetrics() {
        if (mUsageMonitor != null) {
            mUsageMonitor.stop();
        }
        if (mMiscAndroidMetrics != null) mMiscAndroidMetrics.close();
        mMiscAndroidMetrics = null;
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
        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    SafeBrowsingBridge safeBrowsingBridge =
                            new SafeBrowsingBridge(getCurrentProfile());
                    if (safeBrowsingBridge.getSafeBrowsingState()
                            != SafeBrowsingState.NO_SAFE_BROWSING) {
                        // initSafeBrowsing could be executed on a background thread
                        PostTask.postTask(
                                TaskTraits.USER_VISIBLE_MAY_BLOCK,
                                () -> {
                                    BraveSafeBrowsingApiHandler.getInstance().initSafeBrowsing();
                                });
                    }
                    executeInitSafeBrowsing(
                            BraveSafeBrowsingApiHandler.SAFE_BROWSING_INIT_INTERVAL_MS);
                },
                delay);
    }

    public void updateBottomSheetPosition(int orientation) {
        if (BottomToolbarConfiguration.isBottomToolbarEnabled()) {
            // Ensure the bottom sheet's container is adjusted to the height of the bottom toolbar.
            ViewGroup sheetContainer = findViewById(R.id.sheet_container);
            assert sheetContainer != null;

            if (sheetContainer != null) {
                CoordinatorLayout.LayoutParams params =
                        (CoordinatorLayout.LayoutParams) sheetContainer.getLayoutParams();
                params.bottomMargin = orientation == Configuration.ORIENTATION_LANDSCAPE
                        ? 0
                        : getResources().getDimensionPixelSize(R.dimen.bottom_controls_height);
                sheetContainer.setLayoutParams(params);
            }
        }
    }

    /**
     * Calls to {@link ChromeTabbedActivity#maybeHandleUrlIntent} will be redirected here via
     * bytecode changes.
     */
    public boolean maybeHandleUrlIntent(Intent intent) {
        // Redirect requests if necessary
        String url = IntentHandler.getUrlFromIntent(intent);
        if (url != null && url.equals(BraveIntentHandler.CONNECTION_INFO_HELP_URL)) {
            intent.setData(Uri.parse(BraveIntentHandler.BRAVE_CONNECTION_INFO_HELP_URL));
        }
        String appLinkAction = intent.getAction();
        Uri appLinkData = intent.getData();

        if (Intent.ACTION_VIEW.equals(appLinkAction) && appLinkData != null) {
            String lastPathSegment = appLinkData.getLastPathSegment();
            if (lastPathSegment != null
                    && (lastPathSegment.equalsIgnoreCase(BraveConstants.DEEPLINK_ANDROID_PLAYLIST)
                            || lastPathSegment.equalsIgnoreCase(
                                    BraveConstants.DEEPLINK_ANDROID_VPN))) {
                return false;
            }
        }
        // Call ChromeTabbedActivity's version.
        return (boolean)
                BraveReflectionUtil.invokeMethod(
                        ChromeTabbedActivity.class,
                        this,
                        "maybeHandleUrlIntent",
                        Intent.class,
                        intent);
    }

    public RootUiCoordinator getRootUiCoordinator() {
        return mRootUiCoordinator;
    }

    public MultiInstanceManager getMultiInstanceManager() {
        return (MultiInstanceManager)
                BraveReflectionUtil.getField(
                        ChromeTabbedActivity.class, "mMultiInstanceManager", this);
    }

    private void exitBrave() {
        LayoutInflater inflater =
                (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.brave_exit_confirmation, null);
        DialogInterface.OnClickListener onClickListener =
                (dialog, button) -> {
                    if (button == AlertDialog.BUTTON_POSITIVE) {
                        ApplicationLifetime.terminate(false);
                    } else {
                        dialog.dismiss();
                    }
                };

        AlertDialog.Builder alert =
                new AlertDialog.Builder(this, R.style.ThemeOverlay_BrowserUI_AlertDialog);
        AlertDialog alertDialog =
                alert.setTitle(R.string.menu_exit)
                        .setView(view)
                        .setPositiveButton(R.string.brave_action_yes, onClickListener)
                        .setNegativeButton(R.string.brave_action_no, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.show();
    }

    /*
     * Whether we want to pretend to be a custom tab. May be usefull to avoid certain patches,
     * when we want to have the same behaviour as in custom tabs.
     */
    public void spoofCustomTab(boolean spoof) {
        mSpoofCustomTab = spoof;
    }

    @Override
    public boolean isCustomTab() {
        if (mSpoofCustomTab) {
            return true;
        }

        return super.isCustomTab();
    }

    public void showQuickActionSearchEnginesView(int keypadHeight) {
        mQuickSearchEnginesView =
                getLayoutInflater().inflate(R.layout.quick_serach_engines_view, null);
        RecyclerView recyclerView =
                (RecyclerView)
                        mQuickSearchEnginesView.findViewById(
                                R.id.quick_search_engines_recyclerview);
        LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(BraveActivity.this, LinearLayoutManager.HORIZONTAL, false);
        recyclerView.setLayoutManager(linearLayoutManager);

        ImageView quickSearchEnginesSettings =
                (ImageView)
                        mQuickSearchEnginesView.findViewById(R.id.quick_search_engines_settings);
        quickSearchEnginesSettings.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        openQuickSearchEnginesSettings();
                    }
                });

        List<QuickSearchEnginesModel> searchEngines =
                QuickSearchEnginesUtil.getQuickSearchEnginesForView(getCurrentProfile());
        QuickSearchEnginesModel leoQuickSearchEnginesModel =
                new QuickSearchEnginesModel("", "", "", true);
        searchEngines.add(0, leoQuickSearchEnginesModel);

        QuickSearchEnginesViewAdapter adapter =
                new QuickSearchEnginesViewAdapter(BraveActivity.this, searchEngines, this);
        recyclerView.setAdapter(adapter);
        if (mQuickSearchEnginesView.getParent() == null) {
            WindowManager.LayoutParams params =
                    new WindowManager.LayoutParams(
                            WindowManager.LayoutParams.MATCH_PARENT,
                            WindowManager.LayoutParams.WRAP_CONTENT,
                            WindowManager.LayoutParams.TYPE_APPLICATION_PANEL,
                            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                            WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            params.gravity = Gravity.BOTTOM;
            params.y = keypadHeight; // Position the view above the keyboard

            WindowManager windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
            windowManager.addView(mQuickSearchEnginesView, params);
        }
    }

    public void removeQuickActionSearchEnginesView() {
        if (mQuickSearchEnginesView != null) {
            WindowManager windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
            windowManager.removeView(mQuickSearchEnginesView);
        }
    }

    // QuickSearchCallback
    @Override
    public void onSearchEngineClick(int position, QuickSearchEnginesModel quickSearchEnginesModel) {
        if (getActivityTab() == null) {
            return;
        }
        String query = getBraveToolbarLayout().getLocationBarQuery();
        if (position == 0) {
            BraveLeoUtils.openLeoQuery(getActivityTab().getWebContents(), "", query, true);
        } else {
            LoadUrlParams loadUrlParams =
                    new LoadUrlParams(
                            quickSearchEnginesModel.getUrl().replace("{searchTerms}", query));
            getActivityTab().loadUrl(loadUrlParams);
        }
        getBraveToolbarLayout().clearOmniboxFocus();
    }

    @Override
    public void loadSearchEngineLogo(
            ImageView logoView, QuickSearchEnginesModel quickSearchEnginesModel) {
        ImageUtils.loadSearchEngineLogo(
                getCurrentProfile(), logoView, quickSearchEnginesModel.getKeyword());
    }

    @Override
    public void onKeyboardOpened(int keyboardHeight) {
        if (QuickSearchEnginesUtil.getQuickSearchEnginesFeature()) {
            showQuickActionSearchEnginesView(keyboardHeight);
        }
    }

    @Override
    public void onKeyboardClosed() {
        if (QuickSearchEnginesUtil.getQuickSearchEnginesFeature()) {
            removeQuickActionSearchEnginesView();
        }
    }
}

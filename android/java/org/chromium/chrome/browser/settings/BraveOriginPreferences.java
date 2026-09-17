/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.view.LayoutInflater;
import android.view.TouchDelegate;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.core.text.HtmlCompat;
import androidx.preference.Preference;

import com.google.android.material.snackbar.Snackbar;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.Log;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.brave.browser.brave_origin.BraveOriginServiceFactory;
import org.chromium.brave_origin.mojom.BraveOriginSettingsHandler;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.LinkSubscriptionUtils;
import org.chromium.chrome.browser.brave_origin.BraveOriginSubscriptionPrefs;
import org.chromium.chrome.browser.brave_origin.BraveOriginSubscriptionPrefs.CredentialFetchResult;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.policy.BravePolicyConstants;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.PreferenceParser;
import org.chromium.components.browser_ui.settings.search.SearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

import java.util.Map;

/** Fragment for Brave Origin purchase preferences. */
@NullMarked
public class BraveOriginPreferences extends BravePreferenceFragment
        implements Preference.OnPreferenceChangeListener {
    private static final String TAG = "BraveOriginPrefs";

    /** Where "contact support" in the activation limit message goes. */
    private static final String BRAVE_SUPPORT_URL =
            "https://support.brave.app/hc/en-us/requests/new?ticket_form_id=360003078831";

    /** Minimum tappable size for the snackbar action, which is shorter than that on its own. */
    private static final int MIN_TOUCH_TARGET_DP = 48;

    /**
     * Fragment argument: when true, show the restart snackbar on open (set by native when a
     * purchase is first detected via credential refresh).
     */
    public static final String EXTRA_SHOW_RESTART_PROMPT = "show_restart_prompt";

    // Preference keys
    private static final String PREF_REWARDS_SWITCH = "rewards_switch";
    private static final String PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH =
            "privacy_preserving_analytics_switch";
    private static final String PREF_EMAIL_ALIASES_SWITCH = "email_aliases_switch";
    private static final String PREF_LEO_AI_SWITCH = "leo_ai_switch";
    private static final String PREF_NEWS_SWITCH = "news_switch";
    private static final String PREF_STATISTICS_REPORTING_SWITCH = "statistics_reporting_switch";
    private static final String PREF_VPN_SWITCH = "vpn_switch";
    private static final String PREF_WALLET_SWITCH = "wallet_switch";
    private static final String PREF_WEB_DISCOVERY_PROJECT_SWITCH = "web_discovery_project_switch";
    private static final String PREF_RESET_TO_DEFAULTS = "reset_to_defaults";
    private static final String PREF_LINK_PURCHASE = "link_purchase";
    private static final String PREF_PURCHASE_SECTION = "origin_purchase_section";
    private static final String PREF_BRAVE_ORIGIN = "brave_origin";

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();
    @Nullable private BraveOriginSettingsHandler mBraveOriginSettingsHandler;
    @Nullable private Snackbar mRestartSnackbar;

    /** Whether we are in the post-purchase "fetching credentials" state. */
    private boolean mIsFetchingCredentials;

    /**
     * Whether to show the restart snackbar on open. Set when the screen is opened by native after a
     * credential-refresh purchase, where credentials are already present (no fetching spinner).
     */
    private boolean mShowRestartPrompt;

    /** References to the fetching/restart containers in the snackbar for toggling visibility. */
    @Nullable private View mFetchingContainer;

    @Nullable private View mRestartContainer;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_origin_preferences);
        mPageTitle.set(getString(R.string.menu_origin));

        // Initialize BraveOriginSettingsHandler
        Profile profile = getProfile();
        if (profile != null) {
            mBraveOriginSettingsHandler =
                    BraveOriginServiceFactory.getInstance()
                            .getBraveOriginSettingsHandler(profile, null);
        }

        // Set up toggle preferences
        setupTogglePreference(PREF_REWARDS_SWITCH);
        setupTogglePreference(PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH);
        if (BraveConfig.ENABLE_EMAIL_ALIASES
                && ChromeFeatureList.isEnabled(BraveFeatureList.EMAIL_ALIASES)) {
            setupTogglePreference(PREF_EMAIL_ALIASES_SWITCH);
        } else {
            ChromeSwitchPreference emailAliasesPref =
                    (ChromeSwitchPreference) findPreference(PREF_EMAIL_ALIASES_SWITCH);
            if (emailAliasesPref != null) {
                emailAliasesPref.setVisible(false);
            }
        }
        setupTogglePreference(PREF_LEO_AI_SWITCH);
        setupTogglePreference(PREF_NEWS_SWITCH);
        setupTogglePreference(PREF_STATISTICS_REPORTING_SWITCH);
        setupTogglePreference(PREF_VPN_SWITCH);
        setupTogglePreference(PREF_WALLET_SWITCH);
        setupTogglePreference(PREF_WEB_DISCOVERY_PROJECT_SWITCH);

        // Set up click preferences
        Preference resetToDefaults = findPreference(PREF_RESET_TO_DEFAULTS);
        if (resetToDefaults != null) {
            resetToDefaults.setOnPreferenceClickListener(
                    preference -> {
                        showResetConfirmationDialog();
                        return true;
                    });
        }

        Preference linkPurchase = findPreference(PREF_LINK_PURCHASE);
        if (linkPurchase != null) {
            // "Link Purchase" is only relevant when there's a Play Store purchase
            // that hasn't been linked to a Brave account yet. For desktop purchases
            // there's no Play Store receipt to link.
            boolean hasPlayStorePurchase =
                    BraveOriginSubscriptionPrefs.getIsSubscriptionActive(profile);
            boolean isLinked = BraveOriginSubscriptionPrefs.isSubscriptionLinked(profile);
            boolean showLink = hasPlayStorePurchase && !isLinked;
            linkPurchase.setVisible(showLink);

            Preference purchaseSection = findPreference(PREF_PURCHASE_SECTION);
            if (purchaseSection != null) {
                purchaseSection.setVisible(showLink);
            }

            linkPurchase.setOnPreferenceClickListener(
                    preference -> {
                        TabUtils.openURLWithBraveActivity(
                                LinkSubscriptionUtils.getBraveAccountLinkUrl(
                                        InAppPurchaseWrapper.SubscriptionProduct.ORIGIN));
                        return true;
                    });
        }

        Bundle args = getArguments();
        if (args != null) {
            mShowRestartPrompt = args.getBoolean(EXTRA_SHOW_RESTART_PROMPT, false);
        }

        // Check if we are in the post-purchase state (credentials being fetched)
        if (BraveOriginSubscriptionPrefs.isFetchingCredentials(profile)) {
            mIsFetchingCredentials = true;
            setAllPreferencesEnabled(false);

            BraveOriginSubscriptionPrefs.setCredentialsFetchedCallback(
                    (result) -> {
                        mIsFetchingCredentials = false;
                        if (result == CredentialFetchResult.SUCCESS) {
                            setAllPreferencesEnabled(true);
                            transitionSnackbarToRestart();
                        } else if (result
                                == CredentialFetchResult.ACTIVATION_LIMIT_EXTENDABLE) {
                            showActivationLimitSnackbar(/* canExtend= */ true);
                        } else if (result == CredentialFetchResult.ACTIVATION_LIMIT_REACHED) {
                            showActivationLimitSnackbar(/* canExtend= */ false);
                        } else {
                            // Close the preferences screen on failure
                            dismissRestartSnackbar();
                            if (getActivity() != null) {
                                getActivity().finish();
                            }
                        }
                    },
                    profile);

            // A failed fetch leaves the prefs in the same state a killed one does, so returning to
            // this screen finds "fetching" with nothing actually running. Restart it, or the
            // spinner never resolves. No-op when a fetch is already in flight.
            BraveOriginSubscriptionPrefs.resumeCredentialFetchIfNeeded(
                    profile, /* openSettings= */ false);
        }
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        if (mIsFetchingCredentials) {
            showFetchingSnackbar();
        } else if (mShowRestartPrompt) {
            showRestartSnackbar();
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mIsFetchingCredentials) {
            return false;
        }
        String key = preference.getKey();
        boolean isEnabled = (Boolean) newValue;

        String policyKey = getPolicyKeyForPreference(key);
        if (policyKey == null || mBraveOriginSettingsHandler == null) {
            return false;
        }
        // For DISABLED policies, invert the value (checked = enabled = false policy value)
        // For ENABLED policies, use the value as-is (checked = enabled = true policy value)
        boolean policyValue =
                BraveOriginSubscriptionPrefs.isPolicyInverted(policyKey) ? !isEnabled : isEnabled;
        mBraveOriginSettingsHandler.setPolicyValue(
                policyKey,
                policyValue,
                (success) -> {
                    if (!success) {
                        Log.e(TAG, "Failed to set policy value for " + policyKey);
                    }
                    showRestartSnackbar();
                });
        return true;
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    /**
     * Shows a custom snackbar prompting the user to restart the browser after toggling a feature.
     */
    private void showRestartSnackbar() {
        showRestartSnackbar(
                R.string.origin_changing_brave_features_title,
                R.string.origin_changing_brave_features_message);
    }

    /**
     * Shows the restart snackbar with its own title and message, for callers whose reason for
     * restarting is not a feature toggle.
     */
    private void showRestartSnackbar(int titleRes, int messageRes) {
        // Don't replace the fetching snackbar while credentials are loading
        if (mIsFetchingCredentials) {
            return;
        }
        // Don't show another snackbar if one is already visible
        if (mRestartSnackbar != null && mRestartSnackbar.isShown()) {
            return;
        }

        View view = getView();
        if (view == null) {
            return;
        }

        mRestartSnackbar = Snackbar.make(view, "", Snackbar.LENGTH_INDEFINITE);
        Snackbar.SnackbarLayout snackbarLayout =
                (Snackbar.SnackbarLayout) mRestartSnackbar.getView();

        // Remove default snackbar content, padding, and background so custom layout shows through
        snackbarLayout.removeAllViews();
        snackbarLayout.setPadding(0, 0, 0, 0);
        snackbarLayout.setBackground(null);

        // Inflate custom layout
        View customView =
                LayoutInflater.from(requireContext())
                        .inflate(R.layout.origin_restart_snackbar, null);

        ((TextView) customView.findViewById(R.id.snackbar_title)).setText(titleRes);
        ((TextView) customView.findViewById(R.id.snackbar_message)).setText(messageRes);

        // Set up restart action
        TextView actionButton = customView.findViewById(R.id.snackbar_action);
        // The action row is only as tall as its text, so hang the delegate off the snackbar root,
        // which has the vertical room the expanded hit rect needs.
        expandActionTouchTarget(actionButton, (ViewGroup) customView);
        actionButton.setOnClickListener(
                v -> {
                    dismissRestartSnackbar();
                    BraveRelaunchUtils.restart();
                });


        snackbarLayout.addView(customView, 0);
        mRestartSnackbar.show();
    }

    /**
     * Shows the snackbar in the "activation limit reached" state: the subscription has been set up
     * on as many devices as it allows.
     *
     * @param canExtend Whether the payment service will grant more activations. When true the
     *     action asks for them and retries the credential fetch that the limit blocked; when false
     *     there is no action to offer, and the message's support link is the only way forward.
     */
    private void showActivationLimitSnackbar(boolean canExtend) {
        View view = getView();
        if (view == null) {
            return;
        }
        dismissRestartSnackbar();

        mRestartSnackbar = Snackbar.make(view, "", Snackbar.LENGTH_INDEFINITE);
        Snackbar.SnackbarLayout snackbarLayout =
                (Snackbar.SnackbarLayout) mRestartSnackbar.getView();

        snackbarLayout.removeAllViews();
        snackbarLayout.setPadding(0, 0, 0, 0);
        snackbarLayout.setBackground(null);

        View customView =
                LayoutInflater.from(requireContext())
                        .inflate(R.layout.origin_restart_snackbar, null);

        TextView titleView = customView.findViewById(R.id.snackbar_title);
        titleView.setText(R.string.origin_activation_limit_title);

        TextView messageView = customView.findViewById(R.id.snackbar_message);
        messageView.setText(buildActivationLimitMessage());
        messageView.setMovementMethod(LinkMovementMethod.getInstance());

        View fetchingContainer = customView.findViewById(R.id.snackbar_fetching_container);
        View restartContainer = customView.findViewById(R.id.snackbar_restart_container);
        mFetchingContainer = fetchingContainer;
        mRestartContainer = restartContainer;
        fetchingContainer.setVisibility(View.GONE);
        restartContainer.setVisibility(canExtend ? View.VISIBLE : View.GONE);

        if (!canExtend) {
            snackbarLayout.addView(customView, 0);
            mRestartSnackbar.show();
            return;
        }

        TextView fetchingText = customView.findViewById(R.id.snackbar_fetching_text);
        fetchingText.setText(R.string.origin_processing);

        TextView actionButton = customView.findViewById(R.id.snackbar_action);
        // The action row is only as tall as its text, so hang the delegate off the snackbar root,
        // which has the vertical room the expanded hit rect needs.
        expandActionTouchTarget(actionButton, (ViewGroup) customView);
        actionButton.setText(R.string.origin_request_more_activations);
        actionButton.setOnClickListener(
                v -> {
                    // Title and message stay put; only the action area becomes the spinner.
                    restartContainer.setVisibility(View.GONE);
                    fetchingContainer.setVisibility(View.VISIBLE);

                    // Re-register before asking: extendActivationLimit reports the retried fetch
                    // through the credentials-fetched callback, which fired once to get us here
                    // and cleared itself.
                    Profile profile = getProfile();
                    BraveOriginSubscriptionPrefs.setCredentialsFetchedCallback(
                            this::onActivationExtendResult, profile);
                    BraveOriginSubscriptionPrefs.extendActivationLimit(profile);
                });

        snackbarLayout.addView(customView, 0);
        mRestartSnackbar.show();
    }

    /** Handles the credential fetch that follows a granted activation request. */
    private void onActivationExtendResult(@CredentialFetchResult int result) {
        if (result == CredentialFetchResult.ACTIVATION_LIMIT_REACHED) {
            // The retry hit the limit again with nothing left to grant, so drop the action
            // instead of offering a request that cannot succeed.
            showActivationLimitSnackbar(/* canExtend= */ false);
            return;
        }
        if (result != CredentialFetchResult.SUCCESS) {
            // Put the action back so the user can try again or reach support, rather than
            // stranding them on a spinner or closing the screen.
            if (mFetchingContainer != null) {
                mFetchingContainer.setVisibility(View.GONE);
            }
            if (mRestartContainer != null) {
                mRestartContainer.setVisibility(View.VISIBLE);
            }
            return;
        }

        setAllPreferencesEnabled(true);
        // Rebuild rather than transition: this snackbar's action was repurposed for the
        // activation request, so it needs the restart text and listener back. The restart is
        // prompted by the activation grant, not by a feature toggle, so it says so.
        dismissRestartSnackbar();
        showRestartSnackbar(
                R.string.origin_ready_to_activate_title,
                R.string.origin_ready_to_activate_message);
    }

    /**
     * Builds the activation limit message with "contact support" rendered as a link. The link text
     * is a separate resource so translations can move it within the sentence.
     */
    private CharSequence buildActivationLimitMessage() {
        String linkText = getString(R.string.origin_contact_support);
        String message = getString(R.string.origin_activation_limit_message, linkText);
        SpannableString spannable = new SpannableString(message);
        int start = message.indexOf(linkText);
        if (start >= 0) {
            spannable.setSpan(
                    new ClickableSpan() {
                        @Override
                        public void onClick(View widget) {
                            TabUtils.openURLWithBraveActivity(BRAVE_SUPPORT_URL);
                        }
                    },
                    start,
                    start + linkText.length(),
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        return spannable;
    }

    /**
     * Expands {@code action}'s touch area to {@link #MIN_TOUCH_TARGET_DP}, registering the
     * delegate on {@code delegateParent} because the action's own row is only as tall as its text
     * and a delegate never sees touches outside its view's bounds.
     *
     * <p>The listener is registered on the action itself so it is released along with it. A global
     * layout listener would outlive the snackbar: once attached, getViewTreeObserver() hands back
     * the window's observer, and nothing here would remove the listener from it.
     */
    private static void expandActionTouchTarget(View action, ViewGroup delegateParent) {
        action.addOnLayoutChangeListener(
                (v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
                    Rect bounds = new Rect();
                    action.getDrawingRect(bounds);
                    delegateParent.offsetDescendantRectToMyCoords(action, bounds);

                    float density = action.getResources().getDisplayMetrics().density;
                    int extra =
                            (Math.round(MIN_TOUCH_TARGET_DP * density) - bounds.height()) / 2;
                    if (extra <= 0) {
                        return;
                    }
                    bounds.top -= extra;
                    bounds.bottom += extra;
                    // One action per snackbar, so a plain delegate is enough - no need to
                    // compose it with others.
                    delegateParent.setTouchDelegate(new TouchDelegate(bounds, action));
                });
    }

    private void dismissRestartSnackbar() {
        if (mRestartSnackbar != null && mRestartSnackbar.isShown()) {
            mRestartSnackbar.dismiss();
        }
        mRestartSnackbar = null;
    }

    /**
     * Enables or disables all toggle and clickable preferences.
     *
     * @param enabled Whether to enable preferences
     */
    private void setAllPreferencesEnabled(boolean enabled) {
        String[] allKeys = {
            PREF_REWARDS_SWITCH,
            PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH,
            PREF_EMAIL_ALIASES_SWITCH,
            PREF_LEO_AI_SWITCH,
            PREF_NEWS_SWITCH,
            PREF_STATISTICS_REPORTING_SWITCH,
            PREF_VPN_SWITCH,
            PREF_WALLET_SWITCH,
            PREF_WEB_DISCOVERY_PROJECT_SWITCH,
            PREF_RESET_TO_DEFAULTS,
            PREF_LINK_PURCHASE,
        };

        for (String key : allKeys) {
            Preference pref = findPreference(key);
            if (pref != null) {
                pref.setEnabled(enabled);
            }
        }
    }

    /**
     * Shows the snackbar in the "fetching credentials" state with a spinner and "Disabling
     * features" text instead of the "Restart now" button.
     */
    private void showFetchingSnackbar() {
        View view = getView();
        if (view == null) {
            return;
        }

        mRestartSnackbar = Snackbar.make(view, "", Snackbar.LENGTH_INDEFINITE);
        Snackbar.SnackbarLayout snackbarLayout =
                (Snackbar.SnackbarLayout) mRestartSnackbar.getView();

        snackbarLayout.removeAllViews();
        snackbarLayout.setPadding(0, 0, 0, 0);
        snackbarLayout.setBackground(null);

        View customView =
                LayoutInflater.from(requireContext())
                        .inflate(R.layout.origin_restart_snackbar, null);

        // Use the post-purchase message (contains <b> tags for "Restart now")
        TextView messageView = customView.findViewById(R.id.snackbar_message);
        messageView.setText(
                HtmlCompat.fromHtml(
                        getString(R.string.origin_changing_brave_features_message_post_purchase),
                        HtmlCompat.FROM_HTML_MODE_COMPACT));

        // Show the fetching container, hide the restart container
        mFetchingContainer = customView.findViewById(R.id.snackbar_fetching_container);
        mRestartContainer = customView.findViewById(R.id.snackbar_restart_container);
        mFetchingContainer.setVisibility(View.VISIBLE);
        mRestartContainer.setVisibility(View.GONE);

        // Wire up the restart container buttons for when we transition
        TextView actionButton = customView.findViewById(R.id.snackbar_action);
        // The action row is only as tall as its text, so hang the delegate off the snackbar root,
        // which has the vertical room the expanded hit rect needs.
        expandActionTouchTarget(actionButton, (ViewGroup) customView);
        actionButton.setOnClickListener(
                v -> {
                    dismissRestartSnackbar();
                    BraveRelaunchUtils.restart();
                });

        snackbarLayout.addView(customView, 0);
        mRestartSnackbar.show();
    }

    /**
     * Transitions the snackbar from the "fetching" spinner state to the "Restart now" button state.
     */
    private void transitionSnackbarToRestart() {
        if (mRestartSnackbar == null || !mRestartSnackbar.isShown()) {
            showRestartSnackbar();
            return;
        }

        // Switch message to the standard text
        Snackbar.SnackbarLayout snackbarLayout =
                (Snackbar.SnackbarLayout) mRestartSnackbar.getView();
        TextView messageView = snackbarLayout.findViewById(R.id.snackbar_message);
        if (messageView != null) {
            messageView.setText(R.string.origin_changing_brave_features_message);
        }

        if (mFetchingContainer != null) {
            mFetchingContainer.setVisibility(View.GONE);
        }
        if (mRestartContainer != null) {
            mRestartContainer.setVisibility(View.VISIBLE);
        }
    }

    /**
     * Sets up a toggle preference with listener and initial state. Also initializes the preference
     * value from the policy service if available.
     *
     * @param key The preference key
     */
    private void setupTogglePreference(String key) {
        ChromeSwitchPreference preference = (ChromeSwitchPreference) findPreference(key);
        if (preference == null) {
            assert false : "Preference not found for key: " + key;
            return;
        }
        preference.setOnPreferenceChangeListener(this);

        // Initialize from policy service if available
        String policyKey = getPolicyKeyForPreference(key);
        if (policyKey == null || mBraveOriginSettingsHandler == null) {
            return;
        }
        mBraveOriginSettingsHandler.getPolicyValue(
                policyKey,
                (value) -> {
                    if (value != null) {
                        // For DISABLED policies, invert the value (!value)
                        // For ENABLED policies, use the value as-is
                        boolean checkedValue =
                                BraveOriginSubscriptionPrefs.isPolicyInverted(policyKey)
                                        ? !value
                                        : value;
                        preference.setChecked(checkedValue);
                    }
                });
    }

    /**
     * Gets the policy key for a given preference key.
     *
     * @param preferenceKey The preference key
     * @return The policy key, or null if not mapped
     */
    @Nullable
    private String getPolicyKeyForPreference(String preferenceKey) {
        // Map preference keys to policy keys
        if (PREF_REWARDS_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_REWARDS_DISABLED;
        } else if (PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_P3A_ENABLED;
        } else if (PREF_LEO_AI_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_AI_CHAT_ENABLED;
        } else if (PREF_NEWS_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_NEWS_DISABLED;
        } else if (PREF_STATISTICS_REPORTING_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_STATS_PING_ENABLED;
        } else if (PREF_VPN_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_VPN_DISABLED;
        } else if (PREF_WALLET_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_WALLET_DISABLED;
        } else if (PREF_WEB_DISCOVERY_PROJECT_SWITCH.equals(preferenceKey)) {
            return BravePolicyConstants.BRAVE_WEB_DISCOVERY_ENABLED;
        }
        // TODO: Add mappings for other preferences as they are implemented
        // PREF_EMAIL_ALIASES_SWITCH - no policy mapping found
        return null;
    }

    private void showResetConfirmationDialog() {
        View dialogView =
                LayoutInflater.from(requireContext()).inflate(R.layout.origin_reset_dialog, null);

        AlertDialog dialog =
                new AlertDialog.Builder(
                                requireContext(), R.style.ThemeOverlay_BrowserUI_AlertDialog)
                        .setView(dialogView)
                        .create();

        dialogView.findViewById(R.id.reset_dialog_cancel).setOnClickListener(v -> dialog.dismiss());
        dialogView
                .findViewById(R.id.reset_dialog_confirm)
                .setOnClickListener(
                        v -> {
                            dialog.dismiss();
                            resetAllToggles();
                        });

        dialog.show();
    }

    private void resetAllToggles() {
        String[] toggleKeys = {
            PREF_REWARDS_SWITCH,
            PREF_PRIVACY_PRESERVING_ANALYTICS_SWITCH,
            PREF_EMAIL_ALIASES_SWITCH,
            PREF_LEO_AI_SWITCH,
            PREF_NEWS_SWITCH,
            PREF_STATISTICS_REPORTING_SWITCH,
            PREF_VPN_SWITCH,
            PREF_WALLET_SWITCH,
            PREF_WEB_DISCOVERY_PROJECT_SWITCH,
        };

        boolean anyChanged = false;
        for (String key : toggleKeys) {
            ChromeSwitchPreference pref = (ChromeSwitchPreference) findPreference(key);
            if (pref == null || !pref.isChecked()) {
                continue;
            }
            pref.setChecked(false);
            anyChanged = true;

            String policyKey = getPolicyKeyForPreference(key);
            if (policyKey != null && mBraveOriginSettingsHandler != null) {
                boolean policyValue = BraveOriginSubscriptionPrefs.isPolicyInverted(policyKey);
                mBraveOriginSettingsHandler.setPolicyValue(
                        policyKey,
                        policyValue,
                        (success) -> {
                            if (!success) {
                                Log.e(TAG, "Failed to reset policy value for " + policyKey);
                            }
                        });
            }
        }

        if (anyChanged) {
            showRestartSnackbar();
        }
    }

    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    BraveOriginPreferences.class.getName(), R.xml.brave_origin_preferences) {

                @Override
                public void initPreferenceXml(
                        Context context,
                        SettingsIndexData indexData,
                        Map<String, SearchIndexProvider> providerMap) {
                    super.initPreferenceXml(context, indexData, providerMap);
                    indexData.addChildParentLink(
                            BraveOriginPreferences.class.getName(),
                            PreferenceParser.createUniqueId(
                                    MainSettings.class.getName(), PREF_BRAVE_ORIGIN));
                }

                @Override
                public void updateDynamicPreferences(Context context, SettingsIndexData indexData) {
                    String frag = BraveOriginPreferences.class.getName();
                    if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ORIGIN)) {
                        indexData.removeEntryForKey(
                                MainSettings.class.getName(), PREF_BRAVE_ORIGIN);
                        return;
                    }
                    // origin_description is an informational widget with no title; exclude it.
                    indexData.removeEntryForKey(frag, "origin_description");
                    if (!BraveConfig.ENABLE_EMAIL_ALIASES
                            || !ChromeFeatureList.isEnabled(BraveFeatureList.EMAIL_ALIASES)) {
                        indexData.removeEntryForKey(frag, PREF_EMAIL_ALIASES_SWITCH);
                    }
                }
            };

    @Override
    public void onDestroy() {
        dismissRestartSnackbar();
        BraveOriginSubscriptionPrefs.setCredentialsFetchedCallback(null, null);
        super.onDestroy();
        if (mBraveOriginSettingsHandler != null) {
            mBraveOriginSettingsHandler.close();
            mBraveOriginSettingsHandler = null;
        }
    }
}

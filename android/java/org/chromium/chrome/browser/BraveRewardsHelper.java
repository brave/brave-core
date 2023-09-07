/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Handler;
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ClickableSpan;
import android.text.style.StyleSpan;
import android.util.DisplayMetrics;
import android.view.TouchDelegate;
import android.view.View;

import androidx.annotation.Nullable;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.browser_ui.widget.RoundedIconGenerator;
import org.chromium.components.favicon.IconType;
import org.chromium.components.favicon.LargeIconBridge;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.url.GURL;

import java.math.BigDecimal;
import java.math.RoundingMode;
import java.text.NumberFormat;
import java.util.Calendar;
import java.util.Locale;

public class BraveRewardsHelper implements LargeIconBridge.LargeIconCallback {
    private static final String PREF_SHOW_BRAVE_REWARDS_ONBOARDING_MODAL =
            "show_brave_rewards_onboarding_modal";
    private static final String PREF_SHOW_BRAVE_REWARDS_ONBOARDING_ONCE =
            "show_brave_rewards_onboarding_once";
    private static final String PREF_SHOW_DECLARE_GEO_MODAL = "show_declare_geo_modal";
    private static final String PREF_SHOW_ONBOARDING_MINI_MODAL = "show_onboarding_mini_modal";
    private static final String PREF_NEXT_REWARDS_ONBOARDING_MODAL_DATE =
            "next_rewards_onboarding_modal_date";
    private static final String PREF_REWARDS_ENV_CHANGE = "rewards_env_change";
    private static final String PREF_REWARDS_ONBOARDING_MODAL = "rewards_onboarding_modal";
    private static final int FAVICON_CIRCLE_MEASUREMENTS = 70; // dp
    private static final int FAVICON_TEXT_SIZE = 50; // dp
    private static final int FAVICON_FETCH_INTERVAL = 1000; // In milliseconds
    private static final int FAVICON_DESIRED_SIZE = 64; // px
    private static LargeIconBridge mLargeIconBridge;

    public static final String BAT_TEXT = "BAT";
    public static final String USD_TEXT = "USD";
    public static final String ONE_BAT_TEXT = "1 BAT";
    public static final String FIVE_BAT_TEXT = "5 BAT";
    public static final String TEN_BAT_TEXT = "10 BAT";

    private String mFaviconUrl;
    private LargeIconReadyCallback mCallback;
    private final Handler mHandler = new Handler();
    private int mFetchCount;
    private final int MAX_FAVICON_FETCH_COUNT = 8;
    public static final int CROSS_FADE_DURATION = 1000; //ms
    public static final int THANKYOU_FADE_OUT_DURATION = 1500; //ms
    public static final int THANKYOU_FADE_IN_DURATION = 1500; //ms
    public static final int THANKYOU_STAY_DURATION = 2000; //ms
    private static final float DP_PER_INCH_MDPI = 160f;
    private Tab mTab;
    private Profile mProfile;

    public static boolean isRewardsEnabled() {
        BraveRewardsNativeWorker worker = BraveRewardsNativeWorker.getInstance();
        return worker != null && worker.isRewardsEnabled();
    }

    public static void resetRewards() {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_REWARDS_ENV_CHANGE, false);
        sharedPreferencesEditor.putLong(PREF_NEXT_REWARDS_ONBOARDING_MODAL_DATE, 0);
        sharedPreferencesEditor.putBoolean(PREF_REWARDS_ONBOARDING_MODAL, false);
        sharedPreferencesEditor.putBoolean(PREF_SHOW_ONBOARDING_MINI_MODAL, true);
        sharedPreferencesEditor.putBoolean(PREF_SHOW_BRAVE_REWARDS_ONBOARDING_MODAL, true);
        sharedPreferencesEditor.putBoolean(PREF_SHOW_BRAVE_REWARDS_ONBOARDING_ONCE, false);
        sharedPreferencesEditor.putBoolean(PREF_SHOW_DECLARE_GEO_MODAL, false);
        sharedPreferencesEditor.apply();
    }

    public static void setRewardsEnvChange(boolean isEnabled) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_REWARDS_ENV_CHANGE, isEnabled);
        sharedPreferencesEditor.apply();
    }

    public static boolean hasRewardsEnvChange() {
        return ContextUtils.getAppSharedPreferences().getBoolean(PREF_REWARDS_ENV_CHANGE, false);
    }

    public static long getNextRewardsOnboardingModalDate() {
        return ContextUtils.getAppSharedPreferences().getLong(
                PREF_NEXT_REWARDS_ONBOARDING_MODAL_DATE, 0);
    }

    public static void setNextRewardsOnboardingModalDate(long nextDate) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putLong(PREF_NEXT_REWARDS_ONBOARDING_MODAL_DATE, nextDate);
        sharedPreferencesEditor.apply();
    }

    public static void setRewardsOnboardingModalShown(boolean isShown) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_REWARDS_ONBOARDING_MODAL, isShown);
        sharedPreferencesEditor.apply();
    }

    public static boolean hasRewardsOnboardingModalShown() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_REWARDS_ONBOARDING_MODAL, false);
    }

    public static boolean shouldShowRewardsOnboardingModalOnDay4() {
        BraveRewardsNativeWorker braveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (!hasRewardsOnboardingModalShown()
                && (getNextRewardsOnboardingModalDate() > 0
                        && System.currentTimeMillis() > getNextRewardsOnboardingModalDate())
                && shouldShowBraveRewardsOnboardingModal() && braveRewardsNativeWorker != null
                && braveRewardsNativeWorker.IsSupported()) {
            if (isRewardsEnabled()) {
                setRewardsOnboardingModalShown(true);
                return false;
            } else {
                return true;
            }
        }
        return false;
    }

    public static boolean shouldShowMiniOnboardingModal() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_SHOW_ONBOARDING_MINI_MODAL, true);
    }

    public static void setShowMiniOnboardingModal(boolean enabled) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_SHOW_ONBOARDING_MINI_MODAL, enabled);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldShowBraveRewardsOnboardingModal() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_SHOW_BRAVE_REWARDS_ONBOARDING_MODAL, true);
    }

    public static void setShowBraveRewardsOnboardingModal(boolean enabled) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_SHOW_BRAVE_REWARDS_ONBOARDING_MODAL, enabled);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldShowBraveRewardsOnboardingOnce() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_SHOW_BRAVE_REWARDS_ONBOARDING_ONCE, false);
    }

    public static void setShowBraveRewardsOnboardingOnce(boolean enabled) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_SHOW_BRAVE_REWARDS_ONBOARDING_ONCE, enabled);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldShowDeclareGeoModal() {
        return ContextUtils.getAppSharedPreferences().getBoolean(
                PREF_SHOW_DECLARE_GEO_MODAL, false);
    }

    public static void setShowDeclareGeoModal(boolean enabled) {
        SharedPreferences.Editor sharedPreferencesEditor =
                ContextUtils.getAppSharedPreferences().edit();
        sharedPreferencesEditor.putBoolean(PREF_SHOW_DECLARE_GEO_MODAL, enabled);
        sharedPreferencesEditor.apply();
    }

    public interface LargeIconReadyCallback {
        void onLargeIconReady(Bitmap icon);
    }

    public BraveRewardsHelper(Tab tab) {
        mTab = tab;
        assert mTab != null;
        mProfile = Profile.getLastUsedRegularProfile();
        if (mLargeIconBridge == null && mTab != null && mProfile != null) {
            mLargeIconBridge = new LargeIconBridge(mProfile);
        }
    }

    /**
     *  we don't destroy mLargeIconBridge sisnce it's static
     */
    private void destroy() {
        if (mLargeIconBridge != null) {
            mLargeIconBridge.destroy();
            mLargeIconBridge = null;
        }
        mCallback =  null;
    }

    public void detach() {
        mCallback =  null;
    }

    public void retrieveLargeIcon(String favIconURL, LargeIconReadyCallback callback) {
        mCallback = callback;
        mFaviconUrl = favIconURL;
        retrieveLargeIconInternal();
    }

    @SuppressLint("VisibleForTests")
    private void retrieveLargeIconInternal() {
        mFetchCount++;

        // FavIconURL (or content URL) is still not available, try to read it again.
        if (mFaviconUrl == null || mFaviconUrl.isEmpty() || mFaviconUrl.equals("clear")) {
            if (mTab != null) {
                mFaviconUrl = mTab.getUrl().getSpec();
            }

            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    retrieveLargeIconInternal();
                }
            }, FAVICON_FETCH_INTERVAL);

            return;
        }

        // Get the icon.
        if (mLargeIconBridge != null && mCallback != null && !mFaviconUrl.isEmpty()
                && mProfile.isNativeInitialized()) {
            mLargeIconBridge.getLargeIconForUrl(new GURL(mFaviconUrl), FAVICON_DESIRED_SIZE, this);
        }
    }

    public Tab getTab() {
        return mTab;
    }

    @Override
    @CalledByNative("LargeIconCallback")
    public void onLargeIconAvailable(@Nullable Bitmap icon, int fallbackColor,
                                     boolean isFallbackColorDefault, @IconType int iconType) {
        if (mFaviconUrl.isEmpty()) {
            return;
        }

        if (mFetchCount == MAX_FAVICON_FETCH_COUNT
                || (icon == null && false == isFallbackColorDefault)) {
            RoundedIconGenerator mIconGenerator = new RoundedIconGenerator(Resources.getSystem(),
                    FAVICON_CIRCLE_MEASUREMENTS, FAVICON_CIRCLE_MEASUREMENTS,
                    FAVICON_CIRCLE_MEASUREMENTS, fallbackColor, FAVICON_TEXT_SIZE);

            mIconGenerator.setBackgroundColor(fallbackColor);
            icon = mIconGenerator.generateIconForUrl(mFaviconUrl);
        } else if (icon == null && true == isFallbackColorDefault) {
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    retrieveLargeIconInternal();
                }
            }, FAVICON_FETCH_INTERVAL);
            return;
        }
        //else: icon is available

        if (mCallback != null) {
            mCallback.onLargeIconReady(icon);
        }
    }


    public static Bitmap getCircularBitmap(Bitmap bitmap) {
        Bitmap output;

        if (bitmap.getWidth() > bitmap.getHeight()) {
            output = Bitmap.createBitmap(bitmap.getHeight(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
        } else {
            output = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getWidth(), Bitmap.Config.ARGB_8888);
        }

        Canvas canvas = new Canvas(output);

        final int color = 0xff424242;
        final Paint paint = new Paint();
        final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());

        float r = 0;

        if (bitmap.getWidth() > bitmap.getHeight()) {
            r = bitmap.getHeight() / 2;
        } else {
            r = bitmap.getWidth() / 2;
        }

        paint.setAntiAlias(true);
        canvas.drawARGB(0, 0, 0, 0);
        paint.setColor(color);
        canvas.drawCircle(r, r, r, paint);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));
        canvas.drawBitmap(bitmap, rect, rect, paint);
        return output;
    }

    static public ChromeTabbedActivity getChromeTabbedActivity() {
      return BraveActivity.getChromeTabbedActivity();
    }

    static public BraveActivity getBraveActivity()
            throws BraveActivity.BraveActivityNotFoundException {
      return BraveActivity.getBraveActivity();
    }

  static public String getCurrentMonth(Calendar currentTime,
      Resources resources, boolean upper_case) {
    String month = resources.getString(R.string.brave_ui_month_jan);
    switch (currentTime.get(Calendar.MONTH)) {
      case Calendar.JANUARY:
        month = resources.getString(R.string.brave_ui_month_jan);
        break;
      case Calendar.FEBRUARY:
        month = resources.getString(R.string.brave_ui_month_feb);
        break;
      case Calendar.MARCH:
        month = resources.getString(R.string.brave_ui_month_mar);
        break;
      case Calendar.APRIL:
        month = resources.getString(R.string.brave_ui_month_apr);
        break;
      case Calendar.MAY:
        month = resources.getString(R.string.brave_ui_month_may);
        break;
      case Calendar.JUNE:
        month = resources.getString(R.string.brave_ui_month_jun);
        break;
      case Calendar.JULY:
        month = resources.getString(R.string.brave_ui_month_jul);
        break;
      case Calendar.AUGUST:
        month = resources.getString(R.string.brave_ui_month_aug);
        break;
      case Calendar.SEPTEMBER:
        month = resources.getString(R.string.brave_ui_month_sep);
        break;
      case Calendar.OCTOBER:
        month = resources.getString(R.string.brave_ui_month_oct);
        break;
      case Calendar.NOVEMBER:
        month = resources.getString(R.string.brave_ui_month_nov);
        break;
      case Calendar.DECEMBER:
        month = resources.getString(R.string.brave_ui_month_dec);
        break;
    }
    if (!upper_case && !month.isEmpty()) {
      return month.substring(0, 1) + month.substring(1).toLowerCase(Locale.getDefault());
    }

    return month;
  }

  static public String getCurrentYear(Resources resources) {
    Calendar currentTime = Calendar.getInstance();
    return Integer.toString(currentTime.get(Calendar.YEAR));
  }

  public static Tab currentActiveChromeTabbedActivityTab() {
      ChromeTabbedActivity activity = BraveRewardsHelper.getChromeTabbedActivity();
      if (activity == null || activity.getTabModelSelector() == null) {
          return null;
      }
      return activity.getActivityTab();
  }

  /**
   *
   * @param fadeout: can be null
   * @param fadein: can be null
   * @param fade_out_visibility: View.INVISIBLE or View.GONE
   * @param fadeInAlpha: fade in alpha level
   * @param fade_out_visibility: fade in/out time (ms)
   */
  public static void crossfade(final View fadeout, final View fadein, int fade_out_visibility, float fadeInAlpha, int fade_time) {
    if (fade_time < 0) {
        fade_time = 0;
    }

    if (fadeInAlpha < 0 || fadeInAlpha > 1) {
        fadeInAlpha= 1f;
    }

    final int fade_out_visibility_local =
    (fade_out_visibility != View.GONE && fade_out_visibility != View.INVISIBLE) ?
      View.GONE : fade_out_visibility;

    // Set the content view to 0% opacity but visible, so that it is visible
    // (but fully transparent) during the animation.
    if (fadein != null) {
      fadein.setAlpha(0f);
      fadein.setVisibility(View.VISIBLE);

      // Animate the content view to 100% opacity, and clear any animation
      // listener set on the view.
      fadein.animate()
              .alpha(fadeInAlpha)
              .setDuration(fade_time)
              .setListener(null);
    }

    // Animate the loading view to 0% opacity. After the animation ends,
    // set its visibility to GONE as an optimization step (it won't
    // participate in layout passes, etc.)
    if (fadeout != null) {
      fadeout.animate()
              .alpha(0f)
              .setDuration(fade_time)
              .setListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                  fadeout.setVisibility(fade_out_visibility_local);
                }
              });
    }
  }

  public static double probiToDouble(String probi) {
      final String PROBI_POWER = "1000000000000000000";
      double val = Double.NaN;
      try {
          BigDecimal probiNumber = new BigDecimal(probi);
          BigDecimal dividerNumber = new BigDecimal(PROBI_POWER);
          val = probiNumber.divide(dividerNumber).doubleValue();
      }
      catch(NumberFormatException e) {
          val = Double.NaN;
      }
      return val;
  }

    /**
     * Expands touchable area of a small view
      * @param parentView
     * @param childView
     * @param extraPadding: dp
     */
  public static void expandTouchArea(final View parentView, final View childView, final int extraPadding) {
        parentView.post(new Runnable() {
            @Override
            public void run() {
                Rect rect = new Rect();
                childView.getHitRect(rect);

                int pxPadding = dp2px(extraPadding);
                rect.top -= pxPadding;
                rect.left -= pxPadding;
                rect.right += pxPadding;
                rect.bottom += pxPadding;
                parentView.setTouchDelegate(new TouchDelegate(rect, childView));
            }
        });
    }

    /**
     * Converts DP into PX
     * @param dp
     * @return
     */
    public static int dp2px(int dp) {
        DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();
        float px = dp * (metrics.densityDpi / DP_PER_INCH_MDPI);
        return Math.round(px);
    }


    public static boolean subtextAtOffset(String text, String subtext, int offset) {
        boolean ret_value = false;
        final int startIndex = text.indexOf(subtext);
        if (startIndex >= 0) {
            final int endIndex = startIndex + subtext.length();
            if (offset >= startIndex && offset  < endIndex) {
                ret_value = true;
            }
        }
        return ret_value;
    }

    public static Spanned spannedFromHtmlString(String string) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            return Html.fromHtml(string, Html.FROM_HTML_MODE_LEGACY);
        } else {
            return Html.fromHtml(string);
        }
    }

    public static String getFormattedAmount(double amount) {
        NumberFormat numberFormat = NumberFormat.getNumberInstance(Locale.getDefault());
        numberFormat.setRoundingMode(RoundingMode.CEILING);
        numberFormat.setMinimumFractionDigits(3);
        return numberFormat.format(amount);
    }

    public static SpannableString tosSpannableString(String text, int colorRes) {
        Context context = ContextUtils.getApplicationContext();
        Spanned textSpanned = spannedFromHtmlString(text);
        SpannableString textSpannableString = new SpannableString(textSpanned.toString());

        NoUnderlineClickableSpan termsOfServiceClickableSpan =
                new NoUnderlineClickableSpan(context, colorRes, (textView) -> {
                    CustomTabActivity.showInfoPage(context, BraveActivity.BRAVE_TERMS_PAGE);
                });

        NoUnderlineClickableSpan privacyPolicyClickableSpan =
                new NoUnderlineClickableSpan(context, colorRes, (textView) -> {
                    CustomTabActivity.showInfoPage(context, BraveActivity.BRAVE_PRIVACY_POLICY);
                });

        setSpan(context, text, textSpannableString, R.string.terms_of_service,
                termsOfServiceClickableSpan); // terms of service
        setSpan(context, text, textSpannableString, R.string.privacy_policy,
                privacyPolicyClickableSpan); // privacy policy
        return textSpannableString;
    }

    public static void setSpan(Context context, String text, SpannableString tosTextSS,
            int stringId, ClickableSpan clickableSpan) {
        String spanString = context.getResources().getString(stringId);
        int spanLength = spanString.length();
        int index = text.indexOf(spanString);
        tosTextSS.setSpan(
                clickableSpan, index, index + spanLength, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        Typeface typeface = Typeface.create("sans-serif", Typeface.NORMAL);
        tosTextSS.setSpan(new StyleSpan(typeface.getStyle()), index, index + spanLength,
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }
}

/** Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.PorterDuff;
import android.os.Handler;
import android.os.Build;
import android.text.Html;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.GridLayout;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.Switch;
import android.widget.TextView;
import android.content.Intent;
import android.widget.Toast;
import android.view.MotionEvent;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.SysUtils;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsExternalWallet.WalletStatus;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsPublisher.PublisherStatus;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorImpl;
import org.chromium.chrome.R;
import org.chromium.content_public.browser.LoadUrlParams;

import org.json.JSONException;
import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Locale;
import java.util.TimeZone;
import java.util.Timer;
import java.util.TimerTask;


public class BraveRewardsPanelPopup implements BraveRewardsObserver, BraveRewardsHelper.LargeIconReadyCallback {
    private static final String TAG = "BraveRewards";
    private static final int UPDATE_BALANCE_INTERVAL = 60000;  // In milliseconds
    private static final int PUBLISHER_INFO_FETCH_RETRY = 3 * 1000; // In milliseconds
    private static final int PUBLISHER_FETCHES_COUNT = 3;
    private static final String YOUTUBE_TYPE = "youtube#";
    private static final String TWITCH_TYPE = "twitch#";
    private static final String COPYRIGHT_SPECIAL = "\u2122";
    private static final char NOTIFICATION_PROMID_SEPARATOR = '_';

    public static final String PREF_WAS_BRAVE_REWARDS_TURNED_ON = "brave_rewards_turned_on";
    public static final String PREF_GRANTS_NOTIFICATION_RECEIVED = "grants_notification_received";
    public static final String PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED = "was_toolbar_bat_logo_button_pressed";
    private static final String ADS_GRANT_TYPE = "1";

    // Custom Android notification
    private static final int REWARDS_NOTIFICATION_NO_INTERNET = 1000;
    private static final String REWARDS_NOTIFICATION_NO_INTERNET_ID = "29d835c2-5752-4152-93c3-8a1ded9dd4ec";
    private static final int REWARDS_PROMOTION_CLAIM_ERROR = REWARDS_NOTIFICATION_NO_INTERNET + 1;
    private static final String REWARDS_PROMOTION_CLAIM_ERROR_ID = "rewards_promotion_claim_error_id";
    //

    // Auto contribute results
    private static final String AUTO_CONTRIBUTE_SUCCESS = "0";
    private static final String AUTO_CONTRIBUTE_GENERAL_ERROR = "1";
    private static final String AUTO_CONTRIBUTE_NOT_ENOUGH_FUNDS = "15";
    private static final String AUTO_CONTRIBUTE_TIPPING_ERROR = "16";
    private static final String ERROR_CONVERT_PROBI = "ERROR";

    // Balance report codes
    private static final int BALANCE_REPORT_GRANTS = 0;
    private static final int BALANCE_REPORT_EARNING_FROM_ADS = 1;
    private static final int BALANCE_REPORT_AUTO_CONTRIBUTE = 2;
    private static final int BALANCE_REPORT_RECURRING_DONATION = 3;
    private static final int BALANCE_REPORT_ONE_TIME_DONATION = 4;

    public static final String PREF_VERIFY_WALLET_ENABLE = "verify_wallet_enable";

    protected final View anchor;
    private final PopupWindow window;
    private final BraveRewardsPanelPopup thisObject;
    private final ChromeTabbedActivity mActivity;
    private final BraveActivity mBraveActivity;
    private View root;
    private Button btJoinRewards;
    private Button btAddFunds;
    private Button btRewardsSettings;
    private Switch btAutoContribute;
    private TextView tvLearnMore;
    private TextView tvYourWalletTitle;
    private BraveRewardsNativeWorker mBraveRewardsNativeWorker;
    private Timer balanceUpdater;
    private Timer mPublisherFetcher;
    private int publisherFetchesCount;

    private int currentTabId;
    private OnCheckedChangeListener autoContributeSwitchListener;
    private Button btRewardsSummary;
    private boolean publisherExist;
    private String currentNotificationId;
    private TextView tvPublisherNotVerified;
    private TextView tvPublisherNotVerifiedSummary;
    private boolean walletDetailsReceived;      //flag: wallet details received
    private boolean showRewardsSummary;        //flag: we don't want OnGetCurrentBalanceReport always opens up Rewards Summary window
    private AnimationDrawable wallet_init_animation;
    private BraveRewardsHelper mIconFetcher;

    private DonationsAdapter mTip_amount_spinner_data_adapter; //data adapter for mTip_amount_spinner (Spinner)
    private Spinner mTip_amount_spinner;
    private boolean mTip_amount_spinner_auto_select; //spinner selection was changed programmatically (true)

    //flag, Handler and delay to prevent quick opening of multiple site banners
    private boolean mTippingInProgress;
    private final Handler mHandler = new Handler();
    private static final int CLICK_DISABLE_INTERVAL = 1000; // In milliseconds
    private static final int WALLET_BALANCE_LIMIT = 25;

    private boolean mClaimInProcess;
    private boolean mWalletCreateInProcess;

    private boolean mAutoContributeEnabled;
    private boolean mPubInReccuredDonation;

    private String batText;
    private String batPointsText;
    private boolean isAnonWallet;

    private BraveRewardsExternalWallet mExternal_wallet;

    private double walletBalance = .0;

    private boolean isVerifyWalletEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(BraveRewardsPanelPopup.PREF_VERIFY_WALLET_ENABLE, false);
    }

    public BraveRewardsPanelPopup(View anchor) {
        currentNotificationId = "";
        publisherExist = false;
        publisherFetchesCount = 0;
        currentTabId = -1;
        this.anchor = anchor;
        this.window = new PopupWindow(anchor.getContext());
        this.window.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
        this.window.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        this.window.setBackgroundDrawable(new ColorDrawable(Color.WHITE));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            this.window.setElevation(20);
        }
        thisObject = this;
        mIconFetcher = new BraveRewardsHelper(BraveRewardsHelper.currentActiveChromeTabbedActivityTab());

        this.window.setTouchInterceptor(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                    thisObject.dismiss();
                    return true;
                }
                return false;
            }
        });
        this.window.setOnDismissListener(new PopupWindow.OnDismissListener() {
            @Override
            public void onDismiss() {
                if (balanceUpdater != null) {
                    balanceUpdater.cancel();
                }

                if (mPublisherFetcher != null) {
                    mPublisherFetcher.cancel();
                }

                if (mIconFetcher != null) {
                    mIconFetcher.detach();
                }

                if (mBraveRewardsNativeWorker != null) {
                    mBraveRewardsNativeWorker.RemoveObserver(thisObject);
                }

                if (currentTabId != -1 && mBraveRewardsNativeWorker != null) {
                    mBraveRewardsNativeWorker.RemovePublisherFromMap(currentTabId);
                }

                if (mBraveActivity != null) {
                    mBraveActivity.OnRewardsPanelDismiss();
                }
            }
        });
        mBraveActivity = BraveRewardsHelper.getBraveActivity();
        mActivity = BraveRewardsHelper.getChromeTabbedActivity();
        mBraveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.AddObserver(thisObject);
        }
        balanceUpdater = new Timer();
        onCreate();
    }

    private void CreateUpdateBalanceTask() {
        balanceUpdater.schedule(new TimerTask() {
            @Override
            public void run() {
                if (thisObject.mBraveRewardsNativeWorker == null) {
                    return;
                }
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mBraveRewardsNativeWorker.FetchGrants();
                    }
                });
            }
        }, 0, UPDATE_BALANCE_INTERVAL);
    }

    protected void onCreate() {
        LayoutInflater inflater =
            (LayoutInflater) this.anchor.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        ViewGroup root = (ViewGroup) inflater.inflate(R.layout.brave_rewards_panel, null);

        isAnonWallet = BraveRewardsHelper.isAnonWallet();

        batText = isAnonWallet ? root.getResources().getString(R.string.brave_ui_bap_text) : root.getResources().getString(R.string.brave_ui_bat_text);
        batPointsText = isAnonWallet ? root.getResources().getString(R.string.brave_ui_bat_points_text) : root.getResources().getString(R.string.brave_ui_bat_text);

        setContentView(root);

        ((TextView)root.findViewById(R.id.get_paid_id)).setText(String.format(root.getResources().getString(R.string.brave_ui_welcome_desc_two), isAnonWallet ? root.getResources().getString(R.string.point) : root.getResources().getString(R.string.token)));

        tvPublisherNotVerifiedSummary = (TextView)root.findViewById(R.id.publisher_not_verified_summary);
        tvPublisherNotVerifiedSummary.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                    int offset = tvPublisherNotVerifiedSummary.getOffsetForPosition(
                                     motionEvent.getX(), motionEvent.getY());

                    String learn_more = BraveRewardsPanelPopup.this.root.getResources().getString(R.string.learn_more);
                    if (BraveRewardsHelper.subtextAtOffset(tvPublisherNotVerifiedSummary.getText().toString(), learn_more, offset) ) {
                        mBraveActivity.openNewOrSelectExistingTab (BraveActivity.REWARDS_LEARN_MORE_URL);
                        dismiss();
                    }
                }
                return false;
            }
        });


        btJoinRewards = (Button)root.findViewById(R.id.join_rewards_id);
        if (mBraveRewardsNativeWorker != null) {
            //check if 'CreateWallet' request has been sent
            mWalletCreateInProcess = mBraveRewardsNativeWorker.IsCreateWalletInProcess();
            if (mWalletCreateInProcess) {
                startJoinRewardsAnimation();
            } else {
                btJoinRewards.setEnabled(true);
            }

            mBraveRewardsNativeWorker.GetRewardsMainEnabled();
        }

        String braveRewardsTitle = root.getResources().getString(R.string.brave_ui_brave_rewards) + COPYRIGHT_SPECIAL;
        ((TextView)root.findViewById(R.id.brave_rewards_id)).setText(braveRewardsTitle);
        Context context = ContextUtils.getApplicationContext();
        if (btJoinRewards != null) {
            btJoinRewards.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mBraveRewardsNativeWorker != null && false == mWalletCreateInProcess) {
                        mBraveRewardsNativeWorker.CreateWallet();
                        mWalletCreateInProcess = true;
                        startJoinRewardsAnimation();
                    }
                }
            }));
        }
        tvLearnMore = (TextView)root.findViewById(R.id.learn_more_id);
        if (tvLearnMore != null) {
            tvLearnMore.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mBraveActivity.openNewOrSelectExistingTab(BraveActivity.REWARDS_SETTINGS_URL);
                    dismiss();
                }
            }));
        }

        tvYourWalletTitle = (TextView)root.findViewById(R.id.your_wallet_title);
        tvYourWalletTitle.setText(isAnonWallet ? root.getResources().getString(R.string.brave_ui_your_balance) : root.getResources().getString(R.string.brave_ui_your_wallet));

        ((TextView)root.findViewById(R.id.br_bat)).setText(batPointsText);

        btRewardsSettings = (Button)root.findViewById(R.id.br_rewards_settings);
        if (btRewardsSettings != null) {
            btRewardsSettings.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mBraveActivity.openNewOrSelectExistingTab(BraveActivity.REWARDS_SETTINGS_URL);
                    dismiss();
                }
            }));
        }

        btAutoContribute = (Switch)root.findViewById(R.id.brave_ui_auto_contribute);

        if (btAutoContribute != null) {
            autoContributeSwitchListener = new OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView,
                                             boolean isChecked) {
                    thisObject.mBraveRewardsNativeWorker.IncludeInAutoContribution(currentTabId, !isChecked);
                }
            };
            btAutoContribute.setOnCheckedChangeListener(autoContributeSwitchListener);
        }

        btRewardsSummary = (Button)root.findViewById(R.id.rewards_summary);
        //btRewardsSummary.setBackgroundColor(Color.parseColor("#e9ebff"));
        btRewardsSummary.getBackground().setColorFilter(Color.parseColor("#e9ebff"), PorterDuff.Mode.SRC);
        if (btRewardsSummary != null) {
            btRewardsSummary.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    GridLayout gl = (GridLayout)thisObject.root.findViewById(R.id.website_summary_grid);
                    LinearLayout ll = (LinearLayout)thisObject.root.findViewById(R.id.br_central_layout);
                    if (gl == null || ll == null) {
                        return;
                    }
                    if (gl.getVisibility() == View.VISIBLE) {
                        gl.setVisibility(View.GONE);
                        ll.setBackgroundColor(Color.parseColor("#e9ebff")/*((ColorDrawable)btRewardsSummary.getBackground()).getColor()*/);
                        SetRewardsSummaryMonthYear();
                        ShowRewardsSummary();
                    } else {
                        TextView tv = (TextView)thisObject.root.findViewById(R.id.br_no_activities_yet);
                        GridLayout glActivities = (GridLayout)thisObject.root.findViewById(R.id.br_activities);
                        if (publisherExist) {
                            gl.setVisibility(View.VISIBLE);
                            ll.setBackgroundColor(Color.WHITE);
                            RemoveRewardsSummaryMonthYear();
                            if (tv != null && glActivities != null) {
                                tv.setVisibility(View.GONE);
                                glActivities.setVisibility(View.GONE);
                            }
                        } else if (tv != null) {
                            if (tv.getVisibility() == View.VISIBLE ||
                                    glActivities.getVisibility() == View.VISIBLE) {
                                tv.setVisibility(View.GONE);
                                glActivities.setVisibility(View.GONE);
                                btRewardsSummary.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.slide_up, 0);
                            } else {
                                ShowRewardsSummary();
                                btRewardsSummary.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.slide_down, 0);
                            }
                        }
                    }
                }
            }));
        }

        SetRewardsSummaryMonthYear();
        // Starts Send a tip Activity
        Button btSendATip = (Button)root.findViewById(R.id.send_a_tip);
        if (btSendATip != null) {
            btSendATip.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mTippingInProgress) {
                        return;
                    }
                    mTippingInProgress = true;

                    Intent intent = new Intent(ContextUtils.getApplicationContext(), BraveRewardsSiteBannerActivity.class);
                    intent.putExtra(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, currentTabId);
                    mActivity.startActivityForResult(intent, BraveActivity.SITE_BANNER_REQUEST_CODE);

                    //BraveRewardsPanelPopup is not an Activity and onActivityResult is not available
                    //to dismiss mTippingInProgress. Post a delayed task to flip a mTippingInProgress flag.
                    mHandler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            mTippingInProgress = false;
                        }
                    }, CLICK_DISABLE_INTERVAL);
                }
            }));
        }
        tvPublisherNotVerified = (TextView)root.findViewById(R.id.publisher_not_verified);
        tvPublisherNotVerified.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                    int offset = tvPublisherNotVerified.getOffsetForPosition(
                                     motionEvent.getX(), motionEvent.getY());

                    String learn_more = BraveRewardsPanelPopup.this.root.getResources().getString(R.string.learn_more);
                    if (BraveRewardsHelper.subtextAtOffset(tvPublisherNotVerified.getText().toString(), learn_more, offset) ) {
                        mBraveActivity.openNewOrSelectExistingTab(BraveActivity.REWARDS_LEARN_MORE_URL);
                        dismiss();
                    }
                }
                return false;
            }
        });

        Button btEnableRewards = (Button)root.findViewById(R.id.enable_rewards_id);
        if (btEnableRewards != null) {
            btEnableRewards.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    thisObject.mBraveRewardsNativeWorker.SetRewardsMainEnabled(true);
                    thisObject.mBraveRewardsNativeWorker.GetRewardsMainEnabled();
                }
            }));
        }

        SetupNotificationsControls();

        //setting Recurrent Donations spinner
        mTip_amount_spinner = root.findViewById(R.id.auto_tip_amount);
        mTip_amount_spinner_data_adapter = new DonationsAdapter(ContextUtils.getApplicationContext());
        mTip_amount_spinner.setAdapter(mTip_amount_spinner_data_adapter);
        mTip_amount_spinner.setOnItemSelectedListener(
        new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(
                AdapterView<?> parent, View view, int position, long id) {
                if (mTip_amount_spinner_auto_select) { //do nothing
                    mTip_amount_spinner_auto_select = false;
                    return;
                }

                Integer intObj = (Integer)mTip_amount_spinner_data_adapter.getItem (position);
                if (null == intObj) {
                    Log.e(TAG, "Wrong position at Recurrent Donations Spinner");
                    return;
                }
                int tipValue = (int)intObj;
                String pubId = mBraveRewardsNativeWorker.GetPublisherId(currentTabId);
                if (0 == tipValue) {
                    //remove recurrent donation for this publisher
                    mBraveRewardsNativeWorker.RemoveRecurring(pubId);
                } else {
                    //update recurrent donation amount for this publisher
                    mBraveRewardsNativeWorker.Donate(pubId, tipValue, true);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                //Do nothing
            }
        });

        mBraveRewardsNativeWorker.GetExternalWallet(
            BraveRewardsBalance.WALLET_UPHOLD);
    }

    private void startJoinRewardsAnimation() {
        Button btJoinRewards = (Button)root.findViewById(R.id.join_rewards_id);
        btJoinRewards.setEnabled(false);
        btJoinRewards.setText(root.getResources().getString(R.string.brave_ui_rewards_creating_text));
        btJoinRewards.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.brave_rewards_loader, 0);
        wallet_init_animation = (AnimationDrawable)btJoinRewards.getCompoundDrawables()[2];
        if (wallet_init_animation != null) {
            wallet_init_animation.start();
        }
    }

    private void SetupNotificationsControls() {
        // Check for notifications
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.GetAllNotifications();
        }

        TextView tvCloseNotification = (TextView)root.findViewById(R.id.br_notification_close);
        if (tvCloseNotification != null) {
            tvCloseNotification.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (currentNotificationId.isEmpty()) {
                        assert false;

                        return;
                    }
                    if (mBraveRewardsNativeWorker != null) {
                        mBraveRewardsNativeWorker.DeleteNotification(currentNotificationId);
                    }
                }
            }));
        }
    }

    private void SetRewardsSummaryMonthYear() {
        if (btRewardsSummary == null) {
            return;
        }
        String currentMonth = BraveRewardsHelper.getCurrentMonth(Calendar.getInstance(),
                              this.root.getResources(), true);
        String currentYear = BraveRewardsHelper.getCurrentYear(this.root.getResources());
        String rewardsText = this.root.getResources().getString(R.string.brave_ui_rewards_summary);
        rewardsText += "\n" + String.format(this.root.getResources().getString(R.string.brave_ui_month_year), currentMonth,
                                            currentYear);
        btRewardsSummary.setText(rewardsText);
        btRewardsSummary.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.slide_down, 0);
    }

    private void RemoveRewardsSummaryMonthYear() {
        if (btRewardsSummary == null) {
            return;
        }
        btRewardsSummary.setText(this.root.getResources().getString(R.string.brave_ui_rewards_summary));
        btRewardsSummary.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.slide_up, 0);
    }

    protected void onShow() {}

    private void preShow() {
        if (this.root == null) {
            throw new IllegalStateException("setContentView was not called with a view to display.");
        }
        onShow();

        this.window.setTouchable(true);
        this.window.setFocusable(true);
        this.window.setOutsideTouchable(true);

        this.window.setContentView(this.root);
    }

    public void setContentView(View root) {
        this.root = root;
        this.window.setContentView(root);
    }

    public void showLikePopDownMenu() {
        this.showLikePopDownMenu(0, 0);
    }

    public void showLikePopDownMenu(int xOffset, int yOffset) {
        this.preShow();

        this.window.setAnimationStyle(R.style.OverflowMenuAnim);

        if (SysUtils.isLowEndDevice()) {
            this.window.setAnimationStyle(0);
        }

        this.window.showAsDropDown(this.anchor, xOffset, yOffset);
    }

    public void dismiss() {
        this.window.dismiss();
    }

    private Tab launchTabInRunningTabbedActivity(LoadUrlParams loadUrlParams) {
        if (mActivity == null || mActivity.getTabModelSelector() == null) return null;

        TabModelSelectorImpl tabbedModeTabModelSelector = (TabModelSelectorImpl) mActivity.getTabModelSelector();
        Tab tab = tabbedModeTabModelSelector.openNewTab(
                      loadUrlParams, TabLaunchType.FROM_BROWSER_ACTIONS, null, false);
        assert tab != null;

        return tab;
    }



    public void ShowRewardsSummary() {
        if (mBraveRewardsNativeWorker != null) {
            showRewardsSummary =  true;
            mBraveRewardsNativeWorker.GetCurrentBalanceReport();
        }
    }

    public void ShowWebSiteView(boolean returning_to_rewards) {
        ((TextView)this.root.findViewById(R.id.br_bat_wallet)).setText(String.format(Locale.getDefault(), "%.3f", 0.0));
        String usdText = String.format(this.root.getResources().getString(R.string.brave_ui_usd), "0.00");
        ((TextView)this.root.findViewById(R.id.br_usd_wallet)).setText(usdText);

        ScrollView sv = (ScrollView)this.root.findViewById(R.id.activity_brave_rewards_panel);
        sv.setVisibility(View.GONE);
        CreateUpdateBalanceTask();
        ScrollView sv_new = (ScrollView)this.root.findViewById(R.id.sv_no_website);
        sv_new.setVisibility(View.VISIBLE);
        if (!returning_to_rewards) {
            ((LinearLayout)this.root.findViewById(R.id.website_summary)).setVisibility(View.VISIBLE);
            ((LinearLayout)this.root.findViewById(R.id.rewards_welcome_back)).setVisibility(View.GONE);
            ShowRewardsSummary();
            Tab currentActiveTab = BraveRewardsHelper.currentActiveChromeTabbedActivityTab();
            if (currentActiveTab != null && !currentActiveTab.isIncognito()) {
                String url = currentActiveTab.getUrlString();
                if (URLUtil.isValidUrl(url)) {
                    mBraveRewardsNativeWorker.GetPublisherInfo(currentActiveTab.getId(), url);
                    mPublisherFetcher = new Timer();
                    mPublisherFetcher.schedule(new PublisherFetchTimer(currentActiveTab.getId(), url),
                                               PUBLISHER_INFO_FETCH_RETRY, PUBLISHER_INFO_FETCH_RETRY);
                } else {
                    btRewardsSummary.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
                    btRewardsSummary.setClickable(false);
                }
            }
        } else {
            HideNotifications();
            ((LinearLayout)this.root.findViewById(R.id.website_summary)).setVisibility(View.GONE);
            ((LinearLayout)this.root.findViewById(R.id.rewards_welcome_back)).setVisibility(View.VISIBLE);
        }
    }

    class PublisherFetchTimer extends TimerTask {

        private final int tabId;
        private final String url;

        PublisherFetchTimer(int tabId, String url) {
            this.tabId = tabId;
            this.url = url;
        }

        @Override
        public void run() {
            if (thisObject.publisherExist || publisherFetchesCount >= PUBLISHER_FETCHES_COUNT) {
                if (mPublisherFetcher != null) {
                    mPublisherFetcher.cancel();
                    mPublisherFetcher = null;
                }

                return;
            }
            if (thisObject.mBraveRewardsNativeWorker == null) {
                return;
            }
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    thisObject.mBraveRewardsNativeWorker.GetPublisherInfo(tabId, url);
                }
            });
            publisherFetchesCount++;
        }
    }

    public class DonationsAdapter extends BaseAdapter implements SpinnerAdapter {
        Context context;
        LayoutInflater inflater;
        int bat_donations [] = {0, 1, 5, 10};

        public DonationsAdapter(Context applicationContext) {
            this.context = applicationContext;
            inflater = (LayoutInflater.from(applicationContext));
        }

        @Override
        public int getCount() {
            return bat_donations.length;
        }


        public int getPosition (int tipValue) {
            int index = Arrays.binarySearch(bat_donations, tipValue);
            return (index < 0) ? -1 : index;
        }

        @Override
        public Object getItem(int i) {
            Integer intObj = null;
            if (i >= 0 && i < bat_donations.length) {
                intObj = Integer.valueOf(bat_donations[i]);
            }
            return intObj;
        }

        @Override
        public long getItemId(int i) {
            return 0;
        }

        @Override
        public View getView(int position, View view, ViewGroup viewGroup) {
            TextView tv;
            if (null == view) {
                tv = (TextView)inflater.inflate(R.layout.brave_rewards_spinnner_item, null);
            } else {
                tv = (TextView)view;
            }

            String strValue = String.format("%d.0 " + batPointsText, bat_donations[position]);
            tv.setText(strValue);
            return tv;
        }

        @Override
        public View getDropDownView(int position, View convertView, ViewGroup parent) {
            //read USD rate
            double usdrate = mBraveRewardsNativeWorker.GetWalletRate();

            TextView tv;
            if (null == convertView) {
                tv = (TextView)inflater.inflate(R.layout.brave_rewards_spinnner_item_dropdown, null);
            } else {
                tv = (TextView)convertView;
            }

            String strValue = String.format("%d.0 " + batPointsText + " (%.2f USD)", bat_donations[position], bat_donations[position] * usdrate);
            tv.setText(strValue);
            int selected = mTip_amount_spinner.getSelectedItemPosition();
            if (selected == position) {
                tv.setCompoundDrawablesWithIntrinsicBounds(R.drawable.changetip_icon, 0, 0, 0);
            }
            return tv;
        }
    }

    /**
     * Validates if a notification can be processed and has an expected
     * number of arguments
     */
    private boolean IsValidNotificationType(int type, int argsNum) {
        boolean valid = false;

        switch (type) {
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_AUTO_CONTRIBUTE:
            valid  = (argsNum >= 4) ? true : false;
            break;
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_VERIFIED_PUBLISHER:
            valid  = (argsNum >= 1) ? true : false;
            break;
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT:
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT_ADS:
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS:
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET:
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_TIPS_PROCESSED:
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_ADS_ONBOARDING:
        case REWARDS_NOTIFICATION_NO_INTERNET:
        case REWARDS_PROMOTION_CLAIM_ERROR:
            valid = true;
            break;
        default:
            valid = false;
        }
        Log.i(TAG, "IsValidNotificationType: type %d argnum %d ", type, argsNum);
        return valid;
    }

    private void ShowNotification(String id, int type, long timestamp,
                                  String[] args) {
        if (mBraveRewardsNativeWorker == null) {
            return;
        }

        // don't process unknown notifications
        if ( !IsValidNotificationType (type, args.length) && mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.DeleteNotification(id);
            return;
        }

        currentNotificationId = id;
        LinearLayout hl = (LinearLayout)root.findViewById(R.id.header_layout);
        hl.setBackgroundResource(R.drawable.notification_header);
        GridLayout gl = (GridLayout)root.findViewById(R.id.wallet_info_gridlayout);
        gl.setVisibility(View.GONE);
        TimeZone utc = TimeZone.getTimeZone("UTC");
        Calendar calTime = Calendar.getInstance(utc);
        calTime.setTimeInMillis(timestamp * 1000);
        String currentMonth = BraveRewardsHelper.getCurrentMonth(calTime,
                              root.getResources(), false);
        String currentDay = Integer.toString(calTime.get(Calendar.DAY_OF_MONTH));
        String notificationTime = currentMonth + " " + currentDay;
        String title = "";
        String description = "";
        Button btClaimOk = (Button)root.findViewById(R.id.br_claim_button);
        View claim_progress = root.findViewById(R.id.progress_br_claim_button);


        //hide or show 'Claim/OK' button if Grant claim is (not) in process
        mClaimInProcess = mBraveRewardsNativeWorker.IsGrantClaimInProcess();
        if (mClaimInProcess) {
            BraveRewardsHelper.crossfade(btClaimOk, claim_progress, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);
        } else {
            btClaimOk.setEnabled(true);
            BraveRewardsHelper.crossfade(claim_progress, btClaimOk, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);
        }

        TextView notificationClose = (TextView)root.findViewById(R.id.br_notification_close);
        notificationClose.setVisibility(View.VISIBLE);
        ImageView notification_icon = (ImageView)root.findViewById(R.id.br_notification_icon);
        LinearLayout nit = (LinearLayout)root.findViewById(R.id.notification_image_text);
        nit.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams params = (LinearLayout.LayoutParams)nit.getLayoutParams();
        params.setMargins(params.leftMargin, 5, params.rightMargin, params.bottomMargin);
        nit.setLayoutParams(params);
        TextView tv = (TextView)root.findViewById(R.id.br_notification_description);
        tv.setGravity(Gravity.CENTER);

        LinearLayout ll = (LinearLayout)root.findViewById(R.id.notification_info_layout);
        ll.setVisibility(View.VISIBLE);

        // TODO other types of notifications
        switch (type) {
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_AUTO_CONTRIBUTE:
            // TODO find the case where it is used
            notification_icon.setImageResource(R.drawable.icon_validated_notification);
            String result = args[1];
            switch (result) {
            case AUTO_CONTRIBUTE_SUCCESS:
                btClaimOk.setText(root.getResources().getString(R.string.ok));
                title = root.getResources().getString(R.string.brave_ui_rewards_contribute);
                notification_icon.setImageResource(R.drawable.contribute_icon);
                hl.setBackgroundResource(R.drawable.notification_header_normal);

                double value = 0;
                String valueString = "";
                String[] splittedValue = args[3].split("\\.", 0);
                // 18 digits is a probi min digits count
                if (splittedValue.length != 0 && splittedValue[0].length() >= 18) {
                    value = BraveRewardsHelper.probiToDouble(args[3]);
                    valueString = Double.isNaN(value) ?
                                  ERROR_CONVERT_PROBI : String.format("%.3f", value);
                } else {
                    value = Double.parseDouble(args[3]);
                    valueString = String.format("%.3f", value);
                }

                description = String.format(
                                  root.getResources().getString(R.string.brave_ui_rewards_contribute_description),
                                  valueString, batPointsText);
                break;
            case AUTO_CONTRIBUTE_NOT_ENOUGH_FUNDS:
                title = "";
                notification_icon.setImageResource(R.drawable.icon_warning_notification);
                hl.setBackgroundResource(R.drawable.notification_header_warning);
                description =
                    root.getResources().getString(R.string.brave_ui_notification_desc_no_funds);
                break;
            case AUTO_CONTRIBUTE_TIPPING_ERROR:
                title = "";
                notification_icon.setImageResource(R.drawable.icon_error_notification);
                hl.setBackgroundResource(R.drawable.notification_header_error);
                description =
                    root.getResources().getString(R.string.brave_ui_notification_desc_tip_error);
                break;
            default:
                title = "";
                notification_icon.setImageResource(R.drawable.icon_error_notification);
                hl.setBackgroundResource(R.drawable.notification_header_error);
                description =
                    root.getResources().getString(R.string.brave_ui_notification_desc_contr_error);
            }
            if (title.isEmpty()) {
                btClaimOk.setVisibility(View.GONE);
                nit.setOrientation(LinearLayout.HORIZONTAL);
                params.setMargins(params.leftMargin, 35, params.rightMargin, params.bottomMargin);
                nit.setLayoutParams(params);
                tv.setGravity(Gravity.START);
            }
            break;
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT:
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT_ADS:
            btClaimOk.setText(root.getResources().getString(R.string.brave_ui_claim));

            int grant_icon_id = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type ) ?
                                R.drawable.grant_icon : R.drawable.notification_icon;
            notification_icon.setImageResource(grant_icon_id);

            title = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type ) ?
                    (isAnonWallet ? root.getResources().getString(R.string.brave_ui_new_point_grant) : root.getResources().getString(R.string.brave_ui_new_token_grant)) :
                    root.getResources().getString(R.string.notification_category_group_brave_ads);

            description = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type ) ?
                          String.format(root.getResources().getString(R.string.brave_ui_new_grant), isAnonWallet ? root.getResources().getString(R.string.point) : root.getResources().getString(R.string.token)) :
                          root.getResources().getString(R.string.brave_ads_you_earned);
            break;
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS:
            btClaimOk.setText(root.getResources().getString(R.string.ok));
            notification_icon.setImageResource(R.drawable.notification_icon);
            title = root.getResources().getString(R.string.brave_ui_insufficient_funds_msg);
            description = root.getResources().getString(R.string.brave_ui_insufficient_funds_desc);
            break;
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET:
            btClaimOk.setText(root.getResources().getString(R.string.ok));
            notification_icon.setImageResource(R.drawable.notification_icon);
            title = root.getResources().getString(R.string.brave_ui_backup_wallet_msg);
            description = root.getResources().getString(R.string.brave_ui_backup_wallet_desc);
            break;
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_TIPS_PROCESSED:
            btClaimOk.setText(root.getResources().getString(R.string.ok));
            title = root.getResources().getString(R.string.brave_ui_contribution_tips);
            description = root.getResources().getString(R.string.brave_ui_tips_processed_notification);
            notification_icon.setImageResource(R.drawable.contribute_icon);
            break;
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_ADS_ONBOARDING:
            btClaimOk.setText(root.getResources().getString(R.string.brave_ui_turn_on_ads));
            title = root.getResources().getString(R.string.brave_ui_brave_ads_launch_title);
            description = root.getResources().getString(R.string.brave_ui_brave_ads_launch_msg);
            notification_icon.setImageResource(R.drawable.notification_icon);
            break;
        case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_VERIFIED_PUBLISHER:
            String pubName = args[0];
            btClaimOk.setText(root.getResources().getString(R.string.ok));
            title = root.getResources().getString(R.string.brave_ui_pending_contribution_title);
            description = root.getResources().getString(R.string.brave_ui_verified_publisher_notification, pubName);
            notification_icon.setImageResource(R.drawable.contribute_icon);
            break;
        case REWARDS_NOTIFICATION_NO_INTERNET:
            title = "";
            notification_icon.setImageResource(R.drawable.icon_error_notification);
            hl.setBackgroundResource(R.drawable.notification_header_error);
            description = "<b>" + root.getResources().getString(R.string.brave_rewards_local_uh_oh)
                          + "</b> " + root.getResources().getString(R.string.brave_rewards_local_server_not_responding);
            btClaimOk.setVisibility(View.GONE);
            notificationClose.setVisibility(View.GONE);
            nit.setOrientation(LinearLayout.HORIZONTAL);
            params.setMargins(params.leftMargin, 180, params.rightMargin, params.bottomMargin);
            nit.setLayoutParams(params);
            tv.setGravity(Gravity.START);
            break;
        case REWARDS_PROMOTION_CLAIM_ERROR:
            title = "";
            btClaimOk.setText(root.getResources().getString(R.string.ok));
            description = "<b>" +
                          root.getResources().getString(
                              R.string.brave_rewards_local_general_grant_error_title)
                          + "</b>";
            notification_icon.setImageResource(R.drawable.coin_stack);
            hl.setBackgroundResource(R.drawable.notification_header_error);
            notificationClose.setVisibility(View.GONE);
            nit.setOrientation(LinearLayout.HORIZONTAL);
            params.setMargins(params.leftMargin, 180, params.rightMargin, params.bottomMargin);
            nit.setLayoutParams(params);
            tv.setGravity(Gravity.START);
            break;
        default:
            Log.e(TAG, "This notification type is either invalid or not handled yet: " + type);
            assert false;
            return;
        }
        String stringToInsert = (title.isEmpty() ? "" : ("<b>" + title + "</b>" + " | ")) + description +
                                (title.isEmpty() ? "" : ("  <font color=#a9aab4>" + notificationTime + "</font>"));
        Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(stringToInsert);
        tv.setText(toInsert);

        SetNotificationButtoClickListener();
    }

    private void SetNotificationButtoClickListener() {
        Button btClaimOk = (Button)root.findViewById(R.id.br_claim_button);
        String strAction = (btClaimOk != null && mBraveRewardsNativeWorker != null ) ? btClaimOk.getText().toString() : "";
        if (strAction.equals(root.getResources().getString(R.string.ok))) {
            btClaimOk.setOnClickListener( new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // This is custom Android notification and thus should be dismissed intead of
                    // deleting
                    if (currentNotificationId.equals(REWARDS_PROMOTION_CLAIM_ERROR_ID)) {
                        DismissNotification(currentNotificationId);
                        return;
                    }
                    mBraveRewardsNativeWorker.DeleteNotification(currentNotificationId);
                }
            });
        } else if (strAction.equals(root.getResources().getString(R.string.brave_ui_claim))) {
            btClaimOk.setOnClickListener( new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    //disable and hide CLAIM button
                    btClaimOk.setEnabled(false);

                    //btClaimOk.setEnabled(false) sometimes not fast enough, so block multiple 'Claim' clicks
                    if (mClaimInProcess || currentNotificationId.isEmpty()) {
                        return;
                    }

                    int prom_id_separator = currentNotificationId.lastIndexOf(NOTIFICATION_PROMID_SEPARATOR);
                    String promId = "";
                    if (-1 != prom_id_separator ) {
                        promId = currentNotificationId.substring(prom_id_separator + 1);
                    }
                    if (promId.isEmpty()) {
                        return;
                    }

                    mClaimInProcess = true;

                    View fadein = root.findViewById(R.id.progress_br_claim_button);
                    BraveRewardsHelper.crossfade(btClaimOk, fadein, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);

                    mBraveRewardsNativeWorker.GetGrant(promId);
                    walletDetailsReceived = false; //re-read wallet status
                    EnableWalletDetails(false);
                }
            });
        } else if (strAction.equals(root.getResources().getString(R.string.brave_ui_turn_on_ads))) {
            btClaimOk.setOnClickListener( new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    mBraveRewardsNativeWorker.DeleteNotification(currentNotificationId);
                    mActivity.openNewOrSelectExistingTab (ChromeTabbedActivity.REWARDS_SETTINGS_URL);
                    dismiss();
                }
            });
        }
    }

    private void DismissNotification(String id) {
        if (!currentNotificationId.equals(id)) {
            return;
        }
        HideNotifications();
        currentNotificationId = "";
        if (mBraveRewardsNativeWorker != null) {
            mBraveRewardsNativeWorker.GetAllNotifications();
        }
    }

    private void HideNotifications() {
        LinearLayout ll = (LinearLayout)root.findViewById(R.id.header_layout);
        ll.setBackgroundResource(R.drawable.header);
        GridLayout gl = (GridLayout)root.findViewById(R.id.wallet_info_gridlayout);
        gl.setVisibility(View.VISIBLE);
        ll = (LinearLayout)root.findViewById(R.id.notification_info_layout);
        ll.setVisibility(View.GONE);
    }

    @Override
    public void OnWalletInitialized(int error_code) {
        if (BraveRewardsNativeWorker.WALLET_CREATED == error_code) {
            BraveAdsNativeHelper.nativeSetAdsEnabled(Profile.getLastUsedRegularProfile());
            ShowWebSiteView(false);
        } else if (BraveRewardsNativeWorker.SAFETYNET_ATTESTATION_FAILED == error_code) {
            dismiss();
        } else if (BraveRewardsNativeWorker.LEDGER_OK != error_code) {
            Button btJoinRewards = (Button)BraveRewardsPanelPopup.this.root.findViewById(R.id.join_rewards_id);
            btJoinRewards.setText(BraveRewardsPanelPopup.this.root.getResources().getString(R.string.brave_ui_welcome_button_text_two));
            btJoinRewards.setClickable(true);
            btJoinRewards.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            if (wallet_init_animation != null) {
                wallet_init_animation.stop();
            }

            Context context = ContextUtils.getApplicationContext();
            Toast toast = Toast.makeText(context, root.getResources().getString(R.string.brave_ui_error_init_wallet), Toast.LENGTH_SHORT);
            toast.show();

        }
        wallet_init_animation = null;
        mWalletCreateInProcess = false;
    }

    /**
     *The 'progress_wallet_update' substitutes 'br_bat_wallet' when not initialized
     *The 'br_usd_wallet' is invisible when not initialized
     * @param enable
     */
    private void EnableWalletDetails(boolean enable) {
        View fadein  = enable ? root.findViewById(R.id.br_bat_wallet) : root.findViewById(R.id.progress_wallet_update);
        View fadeout  = enable ? root.findViewById(R.id.progress_wallet_update) : root.findViewById(R.id.br_bat_wallet);
        BraveRewardsHelper.crossfade(fadeout, fadein, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);

        View usd = root.findViewById(R.id.br_usd_wallet_layout);
        if (enable) {
            BraveRewardsHelper.crossfade(null, usd, 0, 1f, BraveRewardsHelper.CROSS_FADE_DURATION );
        } else {
            BraveRewardsHelper.crossfade(usd, null, View.INVISIBLE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);
        }
    }

    @Override
    public void onLargeIconReady(Bitmap icon) {
        SetFavIcon(icon);
    }


    private void SetFavIcon(Bitmap bmp) {
        if (bmp != null) {
            mActivity.runOnUiThread(
            new Runnable() {
                @Override
                public void run() {
                    ImageView iv = (ImageView) thisObject.root.findViewById(R.id.publisher_favicon);
                    iv.setImageBitmap(BraveRewardsHelper.getCircularBitmap(bmp));

                    View fadeout  = thisObject.root.findViewById(R.id.publisher_favicon_update);
                    BraveRewardsHelper.crossfade(fadeout, iv, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);
                }
            });
        }
    }


    @Override
    public void OnPublisherInfo(int tabId) {
        publisherExist = true;
        currentTabId = tabId;
        RemoveRewardsSummaryMonthYear();
        if (btRewardsSummary != null) {
            btRewardsSummary.setClickable(true);
        }

        String publisherFavIconURL = mBraveRewardsNativeWorker.GetPublisherFavIconURL(currentTabId);
        Tab currentActiveTab = BraveRewardsHelper.currentActiveChromeTabbedActivityTab();
        String url = currentActiveTab.getUrlString();
        final String favicon_url = (publisherFavIconURL.isEmpty()) ? url : publisherFavIconURL;

        mIconFetcher.retrieveLargeIcon(favicon_url, this);

        GridLayout gl = (GridLayout)this.root.findViewById(R.id.website_summary_grid);
        gl.setVisibility(View.VISIBLE);
        LinearLayout ll = (LinearLayout)this.root.findViewById(R.id.br_central_layout);
        ll.setBackgroundColor(Color.WHITE);

        String pubName = thisObject.mBraveRewardsNativeWorker.GetPublisherName(currentTabId);
        String pubId = thisObject.mBraveRewardsNativeWorker.GetPublisherId(currentTabId);
        String pubSuffix = "";
        if (pubId.startsWith(YOUTUBE_TYPE)) {
            pubSuffix = thisObject.root.getResources().getString(R.string.brave_ui_on_youtube);
        } else if (pubName.startsWith(TWITCH_TYPE)) {
            pubSuffix = thisObject.root.getResources().getString(R.string.brave_ui_on_twitch);
        }
        pubName = "<b>" + pubName + "</b> " + pubSuffix;
        TextView tv = (TextView)thisObject.root.findViewById(R.id.publisher_name);
        tv.setText(Html.fromHtml(pubName));
        tv = (TextView)thisObject.root.findViewById(R.id.publisher_attention);
        String percent = Integer.toString(thisObject.mBraveRewardsNativeWorker.GetPublisherPercent(currentTabId)) + "%";
        tv.setText(percent);
        if (btAutoContribute != null) {
            btAutoContribute.setOnCheckedChangeListener(null);
            btAutoContribute.setChecked(!thisObject.mBraveRewardsNativeWorker.GetPublisherExcluded(currentTabId));
            btAutoContribute.setOnCheckedChangeListener(autoContributeSwitchListener);
        }

        UpdatePublisherStatus(
                thisObject.mBraveRewardsNativeWorker.GetPublisherStatus(currentTabId));

        tv = (TextView)root.findViewById(R.id.br_no_activities_yet);
        gl = (GridLayout)thisObject.root.findViewById(R.id.br_activities);
        if (tv != null && gl != null) {
            tv.setVisibility(View.GONE);
            gl.setVisibility(View.GONE);
        }
        thisObject.mBraveRewardsNativeWorker.GetRecurringDonations();

        mBraveRewardsNativeWorker.GetAutoContributeProperties();
    }

    @Override
    public void OnGetCurrentBalanceReport(double[] report) {
        boolean no_activity = true;
        for (int i = 0; i < report.length; i++) {
            TextView tvTitle = null;
            TextView tv = null;
            TextView tvUSD = null;
            String text = "";
            String textUSD = "";

            double  probiDouble = report[i];
            boolean hideControls = (probiDouble == 0);
            String value = Double.isNaN(probiDouble) ? ERROR_CONVERT_PROBI : String.format(Locale.getDefault(), "%.3f", probiDouble);
 
            String usdValue = ERROR_CONVERT_PROBI;
            if (! Double.isNaN(probiDouble)) {
                double usdValueDouble = probiDouble * mBraveRewardsNativeWorker.GetWalletRate();
                usdValue = String.format(Locale.getDefault(), "%.2f USD", usdValueDouble);
            }

            switch (i) {
            case BALANCE_REPORT_GRANTS:
                tvTitle = (TextView)root.findViewById(R.id.br_grants_claimed_title);
                tvTitle.setText(isAnonWallet ? BraveRewardsPanelPopup.this.root.getResources().getString(R.string.brave_ui_point_grant_claimed) : BraveRewardsPanelPopup.this.root.getResources().getString(R.string.brave_ui_token_grant_claimed));
                tv = (TextView)root.findViewById(R.id.br_grants_claimed_bat);
                tvUSD = (TextView)root.findViewById(R.id.br_grants_claimed_usd);
                text = "<font color=#8E2995>" + value + "</font><font color=#000000> " + batText + "</font>";
                textUSD = usdValue;
                break;
            case BALANCE_REPORT_EARNING_FROM_ADS:
                tvTitle = (TextView)root.findViewById(R.id.br_earnings_ads_title);
                tv = (TextView)root.findViewById(R.id.br_earnings_ads_bat);
                tvUSD = (TextView)root.findViewById(R.id.br_earnings_ads_usd);
                text = "<font color=#8E2995>" + value + "</font><font color=#000000> " + batText + "</font>";
                textUSD = usdValue;
                break;
            case BALANCE_REPORT_AUTO_CONTRIBUTE:
                tvTitle = (TextView)root.findViewById(R.id.br_auto_contribute_title);
                tv = (TextView)root.findViewById(R.id.br_auto_contribute_bat);
                tvUSD = (TextView)root.findViewById(R.id.br_auto_contribute_usd);
                text = "<font color=#6537AD>" + value + "</font><font color=#000000> " + batText + "</font>";
                textUSD = usdValue;
                break;
            case BALANCE_REPORT_RECURRING_DONATION:
                tvTitle = (TextView)root.findViewById(R.id.br_recurring_donation_title);
                tv = (TextView)root.findViewById(R.id.br_recurring_donation_bat);
                tvUSD = (TextView)root.findViewById(R.id.br_recurring_donation_usd);
                text = "<font color=#392DD1>" + value + "</font><font color=#000000> " + batText + "</font>";
                textUSD = usdValue;
                break;
            case BALANCE_REPORT_ONE_TIME_DONATION:
                tvTitle = (TextView)root.findViewById(R.id.br_one_time_donation_title);
                tv = (TextView)root.findViewById(R.id.br_one_time_donation_bat);
                tvUSD = (TextView)root.findViewById(R.id.br_one_time_donation_usd);
                text = "<font color=#392DD1>" + value + "</font><font color=#000000> " + batText + "</font>";
                textUSD = usdValue;
                break;
            }
            if (tvTitle != null && tv != null && tvUSD != null &&
                    !text.isEmpty() && !textUSD.isEmpty()) {
                tvTitle.setVisibility(hideControls ? View.GONE : View.VISIBLE);
                tv.setVisibility(hideControls ? View.GONE : View.VISIBLE);
                tvUSD.setVisibility(hideControls ? View.GONE : View.VISIBLE);
                Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(text);
                tv.setText(toInsert);
                tvUSD.setText(textUSD);
            }
            if (report[i] != 0) {
                no_activity = false;
            }
        }

        if (showRewardsSummary) {
            showRewardsSummary = false;
            TextView tv = (TextView) root.findViewById(R.id.br_no_activities_yet);
            GridLayout gr = (GridLayout) root.findViewById(R.id.br_activities);
            if (tv != null && gr != null) {
                if (no_activity) {
                    tv.setVisibility(View.VISIBLE);
                    gr.setVisibility(View.GONE);
                } else {
                    tv.setVisibility(View.GONE);
                    gr.setVisibility(View.VISIBLE);
                }
            }
        }

        mBraveRewardsNativeWorker.GetPendingContributionsTotal();
    }

    @Override
    public void OnNotificationAdded(String id, int type, long timestamp,
                                    String[] args) {
        // Do nothing here as we will receive the most recent notification
        // in OnGetLatestNotification
    }

    @Override
    public void OnNotificationsCount(int count) {}

    @Override
    public void OnGetLatestNotification(String id, int type, long timestamp,
                                        String[] args) {
        if (type == BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET) {
            if (mBraveRewardsNativeWorker != null) {
                mBraveRewardsNativeWorker.DeleteNotification(id);
                mBraveRewardsNativeWorker.GetAllNotifications();
            }
            return;
        }

        // This is to make sure that user saw promotion error message before showing the
        // rest of messages
        if (!currentNotificationId.equals(REWARDS_PROMOTION_CLAIM_ERROR_ID)) {
            ShowNotification(id, type, timestamp, args);
        }
    }

    @Override
    public void OnNotificationDeleted(String id) {
        DismissNotification(id);
    }

    @Override
    public void OnIsWalletCreated(boolean created) {
        if (created) {
            ShowWebSiteView(false);
            mBraveRewardsNativeWorker.FetchGrants();
        } else {
            //wallet hasn't been created yet: show 'Join Rewards' button
            View fadein  = root.findViewById(R.id.join_rewards_id);
            View fadeout  = root.findViewById(R.id.progress_join_rewards);
            BraveRewardsHelper.crossfade(fadeout, fadein, View.GONE, .5f, BraveRewardsHelper.CROSS_FADE_DURATION);
        }
    }

    @Override
    public void OnGetPendingContributionsTotal(double amount) {
        if (amount > 0.0) {
            String non_verified_summary =
                String.format(root.getResources().getString(
                                  R.string.brave_ui_reserved_amount_text), String.format("%.3f", amount), batPointsText) +
                " <font color=#73CBFF>" + root.getResources().getString(R.string.learn_more) +
                ".</font>";
            Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(non_verified_summary);
            tvPublisherNotVerifiedSummary.setText(toInsert);
            tvPublisherNotVerifiedSummary.setVisibility(View.VISIBLE);
        } else {
            tvPublisherNotVerifiedSummary.setVisibility(View.GONE);
        }
    }

    @Override
    public void OnGetRewardsMainEnabled(boolean enabled) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        boolean wasTurnedOn = sharedPreferences.getBoolean(
                                  PREF_WAS_BRAVE_REWARDS_TURNED_ON, false);
        if (!enabled && wasTurnedOn) {
            ShowWebSiteView(true);
        } else {
            mBraveRewardsNativeWorker.WalletExist();
        }
        if (enabled) {
            mBraveRewardsNativeWorker.GetRewardsParameters();
        }
    }

    @Override
    public void OnGetAutoContributeProperties() {
        mAutoContributeEnabled  = mBraveRewardsNativeWorker.IsAutoContributeEnabled();
        mBraveRewardsNativeWorker.GetRecurringDonations();
    }

    @Override
    public void OnGetReconcileStamp(long timestamp) {}


    /**
     * OnRecurringDonationUpdated is fired after a publisher was added or removed to/from
     * recurrent donation list
     */
    @Override
    public void OnRecurringDonationUpdated() {
        String pubId = mBraveRewardsNativeWorker.GetPublisherId(currentTabId);
        mPubInReccuredDonation = mBraveRewardsNativeWorker.IsCurrentPublisherInRecurrentDonations(pubId);

        //all (mPubInReccuredDonation, mAutoContributeEnabled) are false: exit
        //one is true: ac_enabled_controls on
        //mAutoContributeEnabled: attention_layout and include_in_ac_layout on
        //mPubInReccuredDonation: auto_tip_layout is on

        if (mAutoContributeEnabled || mPubInReccuredDonation) {
            root.findViewById(R.id.ac_enabled_controls).setVisibility(View.VISIBLE);

            if (mAutoContributeEnabled) {
                root.findViewById(R.id.attention_layout).setVisibility(View.VISIBLE);
                root.findViewById(R.id.include_in_ac_layout).setVisibility(View.VISIBLE);
                root.findViewById(R.id.brave_ui_auto_contribute_separator_top).setVisibility(View.VISIBLE);
                root.findViewById(R.id.brave_ui_auto_contribute_separator_bottom).setVisibility(View.VISIBLE);
            }

            //Temporary commented out due to dropdown spinner inflating issue on PopupWindow (API 24)
            /*
            if (mPubInReccuredDonation){
                double amount  = mBraveRewardsNativeWorker.GetPublisherRecurrentDonationAmount(pubId);
                UpdateRecurentDonationSpinner(amount);
                root.findViewById(R.id.auto_tip_layout).setVisibility(View.VISIBLE);
            }*/
        }
    }

    @Override
    public void OnResetTheWholeState(boolean success) {}

    @Override
    public void OnRewardsMainEnabled(boolean enabled) {}

    @Override
    public void OnRewardsParameters(int errorCode) {
        boolean former_walletDetailsReceived = walletDetailsReceived;
        if (errorCode == BraveRewardsNativeWorker.LEDGER_OK) {
            DismissNotification(REWARDS_NOTIFICATION_NO_INTERNET_ID);
            if (mBraveRewardsNativeWorker != null) {
                BraveRewardsBalance balance_obj = mBraveRewardsNativeWorker.GetWalletBalance();
                if (balance_obj != null) {
                    walletBalance = balance_obj.mTotal;
                }

                DecimalFormat df = new DecimalFormat("#.###");
                df.setRoundingMode(RoundingMode.FLOOR);
                df.setMinimumFractionDigits(3);
                ((TextView) this.root.findViewById(R.id.br_bat_wallet))
                .setText(df.format(walletBalance));
                ((TextView) this.root.findViewById(R.id.br_bat)).setText(batPointsText);
                double usdValue = walletBalance * mBraveRewardsNativeWorker.GetWalletRate();
                String usdText =
                    String.format(this.root.getResources().getString(R.string.brave_ui_usd),
                                  String.format(Locale.getDefault(), "%.2f", usdValue));
                ((TextView)this.root.findViewById(R.id.br_usd_wallet)).setText(usdText);

                Button btnVerifyWallet = (Button) root.findViewById(R.id.btn_verify_wallet);
                if (btnVerifyWallet != null) {
                    if (walletBalance < WALLET_BALANCE_LIMIT && !isVerifyWalletEnabled()) {
                        btnVerifyWallet.setBackgroundResource(
                            R.drawable.wallet_disconnected_button);
                    } else {
                        btnVerifyWallet.setBackgroundResource(R.drawable.wallet_verify_button);
                    }
                }
            }
            walletDetailsReceived = true;
        } else if (errorCode == BraveRewardsNativeWorker.LEDGER_ERROR) {   // No Internet connection
            String args[] = {};
            ShowNotification(REWARDS_NOTIFICATION_NO_INTERNET_ID, REWARDS_NOTIFICATION_NO_INTERNET, 0, args);
        } else {
            walletDetailsReceived = false;
        }

        if (former_walletDetailsReceived != walletDetailsReceived) {
            EnableWalletDetails(walletDetailsReceived);
        }
    }

    void UpdateRecurentDonationSpinner(double amount) {
        int RequestedPosition = mTip_amount_spinner_data_adapter.getPosition ((int)amount);
        if (RequestedPosition < 0 || RequestedPosition >= mTip_amount_spinner_data_adapter.getCount() ) {
            Log.e(TAG, "Requested position in Recurrent Donations Spinner doesn't exist");
            return;
        }

        int selectedItemPos = mTip_amount_spinner.getSelectedItemPosition();
        if (RequestedPosition != selectedItemPos) {
            mTip_amount_spinner_auto_select = true; //spinner selection was changed programmatically
            mTip_amount_spinner.setSelection(RequestedPosition);
        }
    }

    @Override
    public void OnGrantFinish(int result) {
        mBraveRewardsNativeWorker.GetAllNotifications();
    }

    private void SetVerifyWalletControl(@WalletStatus final int status) {
        Button btnVerifyWallet = (Button)root.findViewById(R.id.btn_verify_wallet);
        boolean annonwallet = BraveRewardsHelper.isAnonWallet();
        SharedPreferences sharedPref = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor editor = sharedPref.edit();
        if (!annonwallet) {
            int rightDrawable = 0;
            int leftDrawable = 0;
            int text = 0;

            switch (status) {
            case BraveRewardsExternalWallet.NOT_CONNECTED:
                rightDrawable = R.drawable.disclosure;
                text = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                break;
            case BraveRewardsExternalWallet.CONNECTED:
                rightDrawable = R.drawable.verified_disclosure;
                text = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                break;
            case BraveRewardsExternalWallet.PENDING:
                editor.putBoolean(PREF_VERIFY_WALLET_ENABLE, true);
                editor.apply();

                rightDrawable = R.drawable.verified_disclosure;
                text = R.string.brave_ui_wallet_button_unverified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(0, 0, rightDrawable, 0);
                break;
            case BraveRewardsExternalWallet.VERIFIED:
                editor.putBoolean(PREF_VERIFY_WALLET_ENABLE, true);
                editor.apply();

                leftDrawable = R.drawable.uphold_white;
                rightDrawable = R.drawable.verified_disclosure;
                text = R.string.brave_ui_wallet_button_verified;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(leftDrawable, 0, rightDrawable, 0);
                btnVerifyWallet.setBackgroundColor(Color.TRANSPARENT);

                //show Add funds button
                Button btnAddFunds = (Button)root.findViewById(R.id.br_add_funds);
                btnAddFunds.setVisibility (View.VISIBLE);
                break;
            case BraveRewardsExternalWallet.DISCONNECTED_NOT_VERIFIED:
            case BraveRewardsExternalWallet.DISCONNECTED_VERIFIED:
                leftDrawable = R.drawable.uphold_white;
                text = R.string.brave_ui_wallet_button_disconnected;
                btnVerifyWallet.setCompoundDrawablesWithIntrinsicBounds(leftDrawable, 0, 0, 0);
                btnVerifyWallet.setBackgroundDrawable(root.getResources().getDrawable(
                        R.drawable.wallet_disconnected_button));
                break;
            default:
                Log.e (TAG, "Unexpected external wallet status");
                return;
            }

            tvYourWalletTitle.setVisibility(View.GONE);
            btnVerifyWallet.setVisibility(View.VISIBLE);
            btnVerifyWallet.setText(root.getResources().getString(text));
            SetVerifyWalletBtnClickHandler(status);
            SetAddFundsBtnClickHandler(status);
        }
    }

    private void SetAddFundsBtnClickHandler(@WalletStatus final int status) {
        Button btnAddFunds = (Button)root.findViewById(R.id.br_add_funds);
        if (status != BraveRewardsExternalWallet.VERIFIED) {
            btnAddFunds.setEnabled(false);
            return;
        }

        btnAddFunds.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismiss();
                mBraveActivity.openNewOrSelectExistingTab (mExternal_wallet.mAdd_url);
            }
        }));
    }

    private void SetVerifyWalletBtnClickHandler(@WalletStatus final int status) {
        Button btnVerifyWallet = (Button)root.findViewById(R.id.btn_verify_wallet);
        BraveRewardsBalance balance_obj = mBraveRewardsNativeWorker.GetWalletBalance();
        if (balance_obj != null) {
            walletBalance = balance_obj.mTotal;
        }

        btnVerifyWallet.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                switch (status) {
                case BraveRewardsExternalWallet.NOT_CONNECTED:
                case BraveRewardsExternalWallet.CONNECTED:
                case BraveRewardsExternalWallet.PENDING:
                case BraveRewardsExternalWallet.VERIFIED:
                    if (walletBalance < WALLET_BALANCE_LIMIT && !isVerifyWalletEnabled()) {
                        Toast.makeText(ContextUtils.getApplicationContext(), root.getResources().getString(R.string.required_minium_balance), Toast.LENGTH_SHORT).show();
                    } else {
                        int requestCode =
                            (status == BraveRewardsExternalWallet.NOT_CONNECTED) ?
                            BraveActivity.VERIFY_WALLET_ACTIVITY_REQUEST_CODE :
                            BraveActivity.USER_WALLET_ACTIVITY_REQUEST_CODE;
                        Intent intent = BuildVerifyWalletActivityIntent(status);
                        mActivity.startActivityForResult(intent, requestCode);
                    }
                    break;
                case BraveRewardsExternalWallet.DISCONNECTED_NOT_VERIFIED:
                case BraveRewardsExternalWallet.DISCONNECTED_VERIFIED:
                    if (walletBalance < WALLET_BALANCE_LIMIT && !isVerifyWalletEnabled()) {
                        Toast.makeText(ContextUtils.getApplicationContext(), root.getResources().getString(R.string.required_minium_balance), Toast.LENGTH_SHORT).show();
                    } else {
                        if (!TextUtils.isEmpty(mExternal_wallet.mVerify_url)) {
                            dismiss();
                            mBraveActivity.openNewOrSelectExistingTab (mExternal_wallet.mVerify_url);
                        }
                    }
                    break;
                default:
                    Log.e (TAG, "Unexpected external wallet status");
                    return;
                }
            }
        }));
    }

    private Intent BuildVerifyWalletActivityIntent(@WalletStatus final int status) {
        Class clazz = null;
        switch (status) {
        case BraveRewardsExternalWallet.NOT_CONNECTED:
            clazz = BraveRewardsVerifyWalletActivity.class;
            break;
        case BraveRewardsExternalWallet.CONNECTED:
        case BraveRewardsExternalWallet.PENDING:
        case BraveRewardsExternalWallet.VERIFIED:
            clazz = BraveRewardsUserWalletActivity.class;
            break;
        default:
            Log.e (TAG, "Unexpected external wallet status");
            return null;
        }

        Intent intent = new Intent(ContextUtils.getApplicationContext(), clazz);
        intent.putExtra(BraveRewardsExternalWallet.ACCOUNT_URL, mExternal_wallet.mAccount_url);
        intent.putExtra(BraveRewardsExternalWallet.ADD_URL, mExternal_wallet.mAdd_url);
        intent.putExtra(BraveRewardsExternalWallet.ADDRESS, mExternal_wallet.mAddress);
        intent.putExtra(BraveRewardsExternalWallet.STATUS, mExternal_wallet.mStatus);
        intent.putExtra(BraveRewardsExternalWallet.TOKEN, mExternal_wallet.mToken);
        intent.putExtra(BraveRewardsExternalWallet.TYPE, mExternal_wallet.mType);
        intent.putExtra(BraveRewardsExternalWallet.USER_NAME, mExternal_wallet.mUser_name);
        intent.putExtra(BraveRewardsExternalWallet.VERIFY_URL, mExternal_wallet.mVerify_url);
        intent.putExtra(BraveRewardsExternalWallet.WITHDRAW_URL, mExternal_wallet.mWithdraw_url);
        return intent;
    }

    @Override
    public void OnGetExternalWallet(int error_code, String external_wallet) {
        try {
            mExternal_wallet = new BraveRewardsExternalWallet(external_wallet);
            SetVerifyWalletControl(mExternal_wallet.mStatus);
        } catch (JSONException e) {
            Log.e (TAG, "Error parsing external wallet status");
            mExternal_wallet = null;
        }
    }

    /**
     *  Show the "promotion claim failed" error message.
     *  Succesful claims are dismissed by a notification.
     */
    @Override
    public void OnClaimPromotion(int error_code) {
        if (error_code != BraveRewardsNativeWorker.LEDGER_OK) {
            String args[] = {};
            ShowNotification(REWARDS_PROMOTION_CLAIM_ERROR_ID, REWARDS_PROMOTION_CLAIM_ERROR, 0, args);
        }
    }

    private void UpdatePublisherStatus(int pubStatus) {
        // Set publisher verified/unverified status
        String verified_text = "";
        TextView publisherVerified = (TextView) root.findViewById(R.id.publisher_verified);
        publisherVerified.setAlpha(1f);
        TextView publisherDelimiter = (TextView) root.findViewById(R.id.publisher_delimiter);
        publisherDelimiter.setAlpha(1f);
        publisherDelimiter.setText(" | ");
        TextView refreshPublisher = (TextView) root.findViewById(R.id.refresh_publisher);
        refreshPublisher.setAlpha(1f);
        refreshPublisher.setEnabled(true);
        View refreshStatusProgress = root.findViewById(R.id.progress_refresh_status);
        refreshStatusProgress.setVisibility(View.GONE);
        refreshPublisher.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String pubId = thisObject.mBraveRewardsNativeWorker.GetPublisherId(currentTabId);
                refreshStatusProgress.setVisibility(View.VISIBLE);
                refreshPublisher.setEnabled(false);
                publisherVerified.setAlpha(.3f);
                publisherDelimiter.setAlpha(.3f);
                refreshPublisher.setAlpha(.3f);
                mBraveRewardsNativeWorker.RefreshPublisher(pubId);
            }
        }));
        if (pubStatus == BraveRewardsPublisher.CONNECTED
                || pubStatus == BraveRewardsPublisher.VERIFIED) {
            verified_text = root.getResources().getString(R.string.brave_ui_verified_publisher);
            publisherVerified.setCompoundDrawablesWithIntrinsicBounds(
                    R.drawable.bat_verified, 0, 0, 0);
            publisherDelimiter.setVisibility(View.GONE);
            refreshPublisher.setVisibility(View.GONE);
        } else {
            verified_text = root.getResources().getString(R.string.brave_ui_not_verified_publisher);
            publisherVerified.setCompoundDrawablesWithIntrinsicBounds(
                    R.drawable.bat_unverified, 0, 0, 0);
            publisherDelimiter.setVisibility(View.VISIBLE);
            refreshPublisher.setVisibility(View.VISIBLE);
        }
        publisherVerified.setText(verified_text);
        publisherVerified.setVisibility(View.VISIBLE);

        // show |brave_ui_panel_connected_text| text if
        // publisher is CONNECTED and user doesn't have any Brave funds (anonymous or
        // blinded wallets)
        String verified_description = "";
        if (pubStatus == BraveRewardsPublisher.CONNECTED) {
            BraveRewardsBalance balance_obj = mBraveRewardsNativeWorker.GetWalletBalance();
            if (balance_obj != null) {
                double braveFunds =
                        ((balance_obj.mWallets.containsKey(BraveRewardsBalance.WALLET_ANONYMOUS)
                                 && balance_obj.mWallets.get(BraveRewardsBalance.WALLET_ANONYMOUS)
                                         != null)
                                        ? balance_obj.mWallets.get(
                                                BraveRewardsBalance.WALLET_ANONYMOUS)
                                        : .0)
                        + ((balance_obj.mWallets.containsKey(BraveRewardsBalance.WALLET_BLINDED)
                                   && balance_obj.mWallets.get(BraveRewardsBalance.WALLET_BLINDED)
                                           != null)
                                        ? balance_obj.mWallets.get(
                                                BraveRewardsBalance.WALLET_BLINDED)
                                        : .0);
                if (braveFunds <= 0) {
                    verified_description =
                            root.getResources().getString(R.string.brave_ui_panel_connected_text);
                }
            }
        } else if (pubStatus == BraveRewardsPublisher.NOT_VERIFIED) {
            verified_description = root.getResources().getString(
                    R.string.brave_ui_not_verified_publisher_description);
        }

        if (!TextUtils.isEmpty(verified_description)) {
            verified_description += "<br/><font color=#73CBFF>"
                    + root.getResources().getString(R.string.learn_more) + ".</font>";
            Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(verified_description);
            TextView tv_note = (TextView) root.findViewById(R.id.publisher_not_verified);
            tv_note.setText(toInsert);
            tv_note.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void OnRefreshPublisher(int status, String publisherKey) {
        String pubName = thisObject.mBraveRewardsNativeWorker.GetPublisherName(currentTabId);
        if (pubName.equals(publisherKey)) {
            UpdatePublisherStatus(status);
        }
    };
}

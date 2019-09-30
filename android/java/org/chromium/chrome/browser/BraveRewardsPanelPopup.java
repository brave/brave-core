/** Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Build;
import android.text.Html;
import android.text.Spanned;
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
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.widget.Toast;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.SysUtils;
import org.chromium.base.task.AsyncTask;
// TODO
// import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveActivity;
import org.chromium.chrome.browser.ChromeTabbedActivity;
// TODO
// import org.chromium.chrome.browser.dialogs.BraveAdsSignupDialog;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModelImpl;
import org.chromium.chrome.browser.tabmodel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorImpl;
// TODO
// import org.chromium.chrome.browser.util.PackageUtils;
import org.chromium.chrome.R;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.base.ContextUtils;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.math.RoundingMode;
import java.net.URL;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.List;
import java.util.Locale;
import java.util.TimeZone;
import java.util.Timer;
import java.util.TimerTask;


public class BraveRewardsPanelPopup implements BraveRewardsObserver, BraveRewardsHelper.LargeIconReadyCallback{
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
    public static final String PREF_IS_BRAVE_REWARDS_ENABLED = "brave_rewards_enabled";
    public static final String PREF_WAS_TOOLBAR_BAT_LOGO_BUTTON_PRESSED = "was_toolbar_bat_logo_button_pressed";
    private static final String ADS_GRANT_TYPE = "ads";

    // Custom Android notification
    private static final int REWARDS_NOTIFICATION_NO_INTERNET = 1000;
    private static final String REWARDS_NOTIFICATION_NO_INTERNET_ID = "29d835c2-5752-4152-93c3-8a1ded9dd4ec";
    //

    // Auto contribute results
    private static final String AUTO_CONTRIBUTE_SUCCESS = "0";
    private static final String AUTO_CONTRIBUTE_GENERAL_ERROR = "1";
    private static final String AUTO_CONTRIBUTE_NOT_ENOUGH_FUNDS = "15";
    private static final String AUTO_CONTRIBUTE_TIPPING_ERROR = "16";
    private static final String ERROR_CONVERT_PROBI = "ERROR";

    // Balance report codes
    private static final int BALANCE_REPORT_DEPOSITS = 0;
    private static final int BALANCE_REPORT_GRANTS = 1;
    private static final int BALANCE_REPORT_EARNING_FROM_ADS = 2;
    private static final int BALANCE_REPORT_AUTO_CONTRIBUTE = 3;
    private static final int BALANCE_REPORT_RECURRING_DONATION = 4;
    private static final int BALANCE_REPORT_ONE_TIME_DONATION = 5;
    private static final int BALANCE_REPORT_TOTAL = 6;

    protected final View anchor;
    private final PopupWindow window;
    private final BraveRewardsPanelPopup thisObject;
    private final ChromeTabbedActivity mActivity;
    private View root;
    private Button btJoinRewards;
    private Button btAddFunds;
    private Button btRewardsSettings;
    private Switch btAutoContribute;
    private TextView tvLearnMore;
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
    private boolean walletInitialized;          //flag: wallet is initialized
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

    private boolean mClaimInProcess;
    private boolean mWalletCreateInProcess;

    private boolean mAutoContributeEnabled;
    private boolean mPubInReccuredDonation;

    public static boolean isBraveRewardsEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(BraveRewardsPanelPopup.PREF_IS_BRAVE_REWARDS_ENABLED, false);
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
        mIconFetcher = new BraveRewardsHelper();

        this.window.setTouchInterceptor(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_OUTSIDE) {
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

                // TODO
                // mActivity.OnRewardsPanelDismiss();
            }
        });
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
                  thisObject.mBraveRewardsNativeWorker.GetWalletProperties();
              }
          });
        }
      }, 0, UPDATE_BALANCE_INTERVAL);
    }

    protected void onCreate() {
        LayoutInflater inflater =
                (LayoutInflater) this.anchor.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        ViewGroup root = (ViewGroup) inflater.inflate(R.layout.brave_rewards_panel, null);
        setContentView(root);
        tvPublisherNotVerifiedSummary = (TextView)root.findViewById(R.id.publisher_not_verified_summary);
        tvPublisherNotVerifiedSummary.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                    int offset = tvPublisherNotVerifiedSummary.getOffsetForPosition(
                      motionEvent.getX(), motionEvent.getY());

                    String learn_more = BraveRewardsPanelPopup.this.root.getResources().getString(R.string.learn_more);
                    if (BraveRewardsHelper.subtextAtOffset(tvPublisherNotVerifiedSummary.getText().toString(), learn_more, offset) ){
                        // TODO
                        // mActivity.openNewOrSelectExistingTab (BraveActivity.REWARDS_LEARN_MORE_URL);
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
            if (mWalletCreateInProcess){
                startJoinRewardsAnimation();
            }
            else{
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
                // TODO
                //mActivity.openNewOrSelectExistingTab(BraveActivity.REWARDS_SETTINGS_URL);
                dismiss();
            }
          }));
        }

        btAddFunds = (Button)root.findViewById(R.id.br_add_funds);
        if (btAddFunds != null) {
          btAddFunds.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO
                //mActivity.openNewOrSelectExistingTab(BraveActivity.ADD_FUNDS_URL);
                dismiss();
            }
          }));
        }
        btRewardsSettings = (Button)root.findViewById(R.id.br_rewards_settings);
        if (btRewardsSettings != null) {
            btRewardsSettings.setOnClickListener((new View.OnClickListener() {
              @Override
              public void onClick(View v) {
                  // TODO
                  // mActivity.openNewOrSelectExistingTab(BraveActivity.REWARDS_SETTINGS_URL);
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

        Button btGrants = (Button)this.root.findViewById(R.id.grants_dropdown);
        if (btGrants != null) {
            btGrants.setOnClickListener((new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    ListView listView = (ListView)thisObject.root.findViewById(R.id.grants_listview);
                    Button btGrants = (Button)thisObject.root.findViewById(R.id.grants_dropdown);
                    if (listView == null || btGrants == null) {
                      return;
                    }
                    if (listView.getVisibility() == View.VISIBLE) {
                      btGrants.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.down_icon, 0);
                      listView.setVisibility(View.GONE);
                    } else {
                      btGrants.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.up_icon, 0);
                      listView.setVisibility(View.VISIBLE);
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
                if (mTippingInProgress){
                    return;
                }
                mTippingInProgress = true;

                Intent intent = new Intent(ContextUtils.getApplicationContext(), BraveRewardsSiteBannerActivity.class);
                intent.putExtra(BraveRewardsSiteBannerActivity.TAB_ID_EXTRA, currentTabId);
                // TODO
                // mActivity.startActivityForResult(intent, BraveActivity.SITE_BANNER_REQUEST_CODE);

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
                    if (BraveRewardsHelper.subtextAtOffset(tvPublisherNotVerified.getText().toString(), learn_more, offset) ){
                        // TODO
                        // mActivity.openNewOrSelectExistingTab(BraveActivity.REWARDS_LEARN_MORE_URL);
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
                        if (mTip_amount_spinner_auto_select){ //do nothing
                            mTip_amount_spinner_auto_select = false;
                            return;
                        }

                        Integer intObj = (Integer)mTip_amount_spinner_data_adapter.getItem (position);
                        if (null == intObj){
                            Log.e(TAG, "Wrong position at Recurrent Donations Spinner");
                            return;
                        }
                        int tipValue = (int)intObj;
                        String pubId = mBraveRewardsNativeWorker.GetPublisherId(currentTabId);
                        if (0 == tipValue){
                            //remove recurrent donation for this publisher
                            mBraveRewardsNativeWorker.RemoveRecurring(pubId);
                        }
                        else{
                            //update recurrent donation amount for this publisher
                            mBraveRewardsNativeWorker.Donate(pubId, tipValue, true);
                        }
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parent) {
                        //Do nothing
                    }
                });
    }

    private void startJoinRewardsAnimation(){
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
        Button btClaimOk = (Button)root.findViewById(R.id.br_claim_button);
        if (btClaimOk != null) {
          btClaimOk.setOnClickListener((new View.OnClickListener() {
            @Override
            public void onClick(View v) {
              Button claimOk = (Button)v;
              if (claimOk.getText().toString().equals(
                root.getResources().getString(R.string.ok))) {
                  if (mBraveRewardsNativeWorker != null) {
                      mBraveRewardsNativeWorker.DeleteNotification(currentNotificationId);
                  }
              }
              else if (mBraveRewardsNativeWorker != null) {
                  //disable and hide CLAIM button
                  claimOk.setEnabled(false);

                  //claimOk.setEnabled(false) sometimes not fast enough, so block multiple 'Claim' clicks
                  if (mClaimInProcess || currentNotificationId.isEmpty()){
                      return;
                  }

                  int prom_id_separator = currentNotificationId.lastIndexOf(NOTIFICATION_PROMID_SEPARATOR);
                  String promId = "";
                  if (-1 != prom_id_separator ) {
                      promId = currentNotificationId.substring(prom_id_separator + 1);
                  }
                  if (promId.isEmpty()){
                      return;
                  }

                  mClaimInProcess = true;

                  View fadein = root.findViewById(R.id.progress_br_claim_button);
                  BraveRewardsHelper.crossfade(claimOk, fadein, View.GONE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);


                  mBraveRewardsNativeWorker.GetGrant(promId);
                  walletDetailsReceived = false; //re-read wallet status
                  EnableWalletDetails(false);

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
        if(this.root == null) {
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
      ((TextView)this.root.findViewById(R.id.br_bat_wallet)).setText(String.format(Locale.getDefault(), "%.1f", 0.0));
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
          Tab currentActiveTab = BraveRewardsHelper.currentActiveTab();
          if (currentActiveTab != null && !currentActiveTab.isIncognito()) {
            String url = currentActiveTab.getUrl();
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
        int bat_donations [] = {0,1,5,10};

        public DonationsAdapter(Context applicationContext) {
            this.context = applicationContext;
            inflater = (LayoutInflater.from(applicationContext));
        }

        @Override
        public int getCount() {
            return bat_donations.length;
        }


        public int getPosition (int tipValue){
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
            }
            else{
                tv = (TextView)view;
            }

            String strValue = String.format(Locale.getDefault(), "%d.0 BAT", bat_donations[position]);
            tv.setText(strValue);
            return tv;
        }

        @Override
        public View getDropDownView(int position, View convertView, ViewGroup parent) {
            //read USD rate
            double usdrate = mBraveRewardsNativeWorker.GetWalletRate("USD");

            TextView tv;
            if (null == convertView) {
                tv = (TextView)inflater.inflate(R.layout.brave_rewards_spinnner_item_dropdown, null);
            }
            else{
                tv = (TextView)convertView;
            }

            String strValue = String.format(Locale.getDefault(), "%d.0 BAT (%.2f USD)", bat_donations[position], bat_donations[position] * usdrate);
            tv.setText(strValue);
            int selected = mTip_amount_spinner.getSelectedItemPosition();
            if (selected == position){
                tv.setCompoundDrawablesWithIntrinsicBounds(R.drawable.changetip_icon, 0, 0, 0);
            }
            return tv;
        }
    }


    private void ShowNotification(String id, int type, long timestamp,
            String[] args) {
        currentNotificationId = id;
        LinearLayout hl = (LinearLayout)root.findViewById(R.id.header_layout);
        hl.setBackgroundResource(R.drawable.notification_header);
        GridLayout gl = (GridLayout)root.findViewById(R.id.wallet_info_gridlayout);
        gl.setVisibility(View.GONE);
        LinearLayout ll = (LinearLayout)root.findViewById(R.id.notification_info_layout);
        ll.setVisibility(View.VISIBLE);
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

        //hide 'Claim' button if Grant claim is in process
        mClaimInProcess = mBraveRewardsNativeWorker.IsGrantClaimInProcess();
        if (mClaimInProcess){
            btClaimOk.setVisibility(View.GONE);
            claim_progress.setVisibility(View.VISIBLE);
        }
        else {
            claim_progress.setVisibility(View.GONE);
            btClaimOk.setVisibility(View.VISIBLE);
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
        // TODO other types of notifications
        switch (type) {
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_AUTO_CONTRIBUTE:
                if (args.length >= 4) {
                    // TODO find the case where it is used
                    notification_icon.setImageResource(R.drawable.icon_validated_notification);
                    String result = args[1];
                    switch (result) {
                        case AUTO_CONTRIBUTE_SUCCESS:
                            btClaimOk.setText(root.getResources().getString(R.string.ok));
                            title = root.getResources().getString(R.string.brave_ui_rewards_contribute);
                            notification_icon.setImageResource(R.drawable.contribute_icon);
                            hl.setBackgroundResource(R.drawable.notification_header_normal);

                            double  probiDouble = BraveRewardsHelper.probiToDouble(args[3]);
                            String probiString = Double.isNaN(probiDouble) ? ERROR_CONVERT_PROBI : String.format(Locale.getDefault(), "%.2f", probiDouble);
                            description = String.format(
                                root.getResources().getString(R.string.brave_ui_rewards_contribute_description),
                                    probiString);
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
                } else {
                    assert false;
                }
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT:
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT_ADS:
                btClaimOk.setText(root.getResources().getString(R.string.brave_ui_claim));

                int grant_icon_id = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type ) ?
                        R.drawable.grant_icon : R.drawable.notification_icon;
                notification_icon.setImageResource(grant_icon_id);

                title = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type ) ?
                        root.getResources().getString(R.string.brave_ui_new_token_grant):
                        root.getResources().getString(R.string.notification_category_group_brave_ads);

                description = (BraveRewardsNativeWorker.REWARDS_NOTIFICATION_GRANT == type ) ?
                        root.getResources().getString(R.string.brave_ui_new_grant) :
                        root.getResources().getString(R.string.brave_ads_you_earned);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS:
                btClaimOk.setText(root.getResources().getString(R.string.ok));
                notification_icon.setImageResource(R.drawable.notification_icon);
                title = root.getResources().getString(R.string.brave_ui_insufficient_funds_msg);
                description = root.getResources().getString(R.string.brave_ui_insufficient_funds_desc);
                break;
            case BraveRewardsNativeWorker.REWARDS_NOTIFICATION_BACKUP_WALLET:
                Log.i("TAG", "!!!here3");
                btClaimOk.setText(root.getResources().getString(R.string.ok));
                notification_icon.setImageResource(R.drawable.notification_icon);
                title = root.getResources().getString(R.string.brave_ui_backup_wallet_msg);
                description = root.getResources().getString(R.string.brave_ui_backup_wallet_desc);
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
            default:
                Log.e(TAG, "This notification type is either invalid or not handled yet: " + type);
                assert false;
                break;
        }
        String stringToInsert = (title.isEmpty() ? "" : ("<b>" + title + "</b>" + " | ")) + description +
          (title.isEmpty() ? "" : ("  <font color=#a9aab4>" + notificationTime + "</font>"));
        Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(stringToInsert);
        tv.setText(toInsert);
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
            walletInitialized = true;
            // TODO
            // BraveAdsNativeHelper.nativeSetAdsEnabled(Profile.getLastUsedProfile());
            ShowWebSiteView(false);
        } else if (BraveRewardsNativeWorker.SAFETYNET_ATTESTATION_FAILED == error_code) {
            dismiss();
        } else {
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

    @Override
    public void OnWalletProperties(int error_code) {
      boolean  former_walletDetailsReceived = walletDetailsReceived;
      if (BraveRewardsNativeWorker.LEDGER_OK == error_code) {
        DismissNotification(REWARDS_NOTIFICATION_NO_INTERNET_ID);
        if (mBraveRewardsNativeWorker != null) {
          double balance = mBraveRewardsNativeWorker.GetWalletBalance();

          DecimalFormat df = new DecimalFormat("#.#");
          df.setRoundingMode(RoundingMode.FLOOR);
          df.setMinimumFractionDigits(1);
          ((TextView)this.root.findViewById(R.id.br_bat_wallet)).setText(df.format(balance));
          double usdValue = balance * mBraveRewardsNativeWorker.GetWalletRate("USD");
          String usdText = String.format(this.root.getResources().getString(R.string.brave_ui_usd),
            String.format(Locale.getDefault(), "%.2f", usdValue));
          ((TextView)this.root.findViewById(R.id.br_usd_wallet)).setText(usdText);

          int currentGrantsCount = mBraveRewardsNativeWorker.GetCurrentGrantsCount();
          Button btGrants = (Button)this.root.findViewById(R.id.grants_dropdown);
          if (currentGrantsCount != 0) {
              btGrants.setCompoundDrawablesWithIntrinsicBounds(0, 0, R.drawable.down_icon, 0);
              btGrants.setVisibility(View.VISIBLE);

              ListView listView = (ListView)this.root.findViewById(R.id.grants_listview);

              ArrayAdapter<Spanned> adapter = new ArrayAdapter<Spanned>(
                ContextUtils.getApplicationContext(), R.layout.brave_rewards_grants_list_item);
              for (int i = 0; i < currentGrantsCount; i++) {
                  String[] grant = mBraveRewardsNativeWorker.GetCurrentGrant(i);
                  if (grant.length < 3) {
                    continue;
                  }

                  double  probiDouble = BraveRewardsHelper.probiToDouble(grant[0]);
                  String probiString = Double.isNaN(probiDouble) ? ERROR_CONVERT_PROBI : String.format(Locale.getDefault(), "%.1f", probiDouble);
                  String toInsert = "<b><font color=#ffffff>" + probiString + " BAT</font></b> ";

                  if (grant[2].equals(BraveRewardsPanelPopup.ADS_GRANT_TYPE) == false) {
                      TimeZone utc = TimeZone.getTimeZone("UTC");
                      Calendar calTime = Calendar.getInstance(utc);
                      calTime.setTimeInMillis(Long.parseLong(grant[1]) * 1000);
                      String date = Integer.toString(calTime.get(Calendar.MONTH) + 1) + "/" +
                              Integer.toString(calTime.get(Calendar.DAY_OF_MONTH)) + "/" +
                              Integer.toString(calTime.get(Calendar.YEAR));
                      toInsert += String.format(this.root.getResources().getString(R.string.brave_ui_expires_on),
                              date);
                  }
                  else {
                      toInsert += this.root.getResources().getString(R.string.brave_ui_ads_earnings);
                  }

                  adapter.add(BraveRewardsHelper.spannedFromHtmlString(toInsert));
              }
              listView.setAdapter(adapter);
          } else {
            btGrants.setVisibility(View.GONE);
          }
        }
        walletDetailsReceived = true;
      } else if (BraveRewardsNativeWorker.LEDGER_ERROR == error_code) {   // No Internet connection
        String args[] = {};
        ShowNotification(REWARDS_NOTIFICATION_NO_INTERNET_ID, REWARDS_NOTIFICATION_NO_INTERNET, 0, args);
      } else {
            walletDetailsReceived = false;
      }

      if (former_walletDetailsReceived != walletDetailsReceived) {
          EnableWalletDetails(walletDetailsReceived);
      }
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

        View usd = root.findViewById(R.id.br_usd_wallet);
        if (enable) {
            BraveRewardsHelper.crossfade(null, usd, 0,1f, BraveRewardsHelper.CROSS_FADE_DURATION );
        }
        else {
            BraveRewardsHelper.crossfade(usd, null, View.INVISIBLE, 1f, BraveRewardsHelper.CROSS_FADE_DURATION);
        }
    }

    @Override
    public void onLargeIconReady(Bitmap icon){
        SetFavIcon(icon);
    }


    private void SetFavIcon(Bitmap bmp) {
        if (bmp != null){
            mActivity.runOnUiThread(
                new Runnable(){
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
        Tab currentActiveTab = BraveRewardsHelper.currentActiveTab();
        String url = currentActiveTab.getUrl();
        final String favicon_url = (publisherFavIconURL.isEmpty()) ? url : publisherFavIconURL;

        mIconFetcher.retrieveLargeIcon(favicon_url,this);

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
        String verified_text = "";
        TextView tvVerified = (TextView)root.findViewById(R.id.publisher_verified);
        if (thisObject.mBraveRewardsNativeWorker.GetPublisherVerified(currentTabId)) {
            verified_text = root.getResources().getString(R.string.brave_ui_verified_publisher);
        } else {
            verified_text = root.getResources().getString(R.string.brave_ui_not_verified_publisher);
            tv = (TextView)root.findViewById(R.id.publisher_not_verified);
            String verified_description =
                root.getResources().getString(R.string.brave_ui_not_verified_publisher_description);
            verified_description += "<br/><font color=#73CBFF>" +
              root.getResources().getString(R.string.learn_more) + ".</font>";
            Spanned toInsert = BraveRewardsHelper.spannedFromHtmlString(verified_description);
            tv.setText(toInsert);
            tv.setVisibility(View.VISIBLE);
            tvVerified.setCompoundDrawablesWithIntrinsicBounds(R.drawable.bat_unverified, 0, 0, 0);
        }
        tvVerified.setText(verified_text);
        tvVerified.setVisibility(View.VISIBLE);

        tv = (TextView)root.findViewById(R.id.br_no_activities_yet);
        gl = (GridLayout)thisObject.root.findViewById(R.id.br_activities);
        if (tv != null && gl != null) {
          tv.setVisibility(View.GONE);
          gl.setVisibility(View.GONE);
        }
        thisObject.mBraveRewardsNativeWorker.GetRecurringDonations();

        mBraveRewardsNativeWorker.GetAutoContributeProps();
    }

    @Override
    public void OnGetCurrentBalanceReport(String[] report) {
        boolean no_activity = true;
        for (int i = 0; i < report.length; i++) {
          TextView tvTitle = null;
          TextView tv = null;
          TextView tvUSD = null;
          String text = "";
          String textUSD = "";

          double  probiDouble = BraveRewardsHelper.probiToDouble(report[i]);
          boolean hideControls = (probiDouble == 0);
          String value = Double.isNaN(probiDouble) ? ERROR_CONVERT_PROBI : String.format(Locale.getDefault(), "%.1f", probiDouble);

          String usdValue = ERROR_CONVERT_PROBI;
          if (! Double.isNaN(probiDouble)){
              double usdValueDouble = probiDouble * mBraveRewardsNativeWorker.GetWalletRate("USD");
              usdValue = String.format(Locale.getDefault(), "%.2f USD", usdValueDouble);
          }

          switch (i) {
            case BALANCE_REPORT_DEPOSITS:
              break;
            case BALANCE_REPORT_GRANTS:
              tvTitle = (TextView)root.findViewById(R.id.br_grants_claimed_title);
              tv = (TextView)root.findViewById(R.id.br_grants_claimed_bat);
              tvUSD = (TextView)root.findViewById(R.id.br_grants_claimed_usd);
              text = "<font color=#8E2995>" + value + "</font><font color=#000000> BAT</font>";
              textUSD = usdValue;
              break;
            case BALANCE_REPORT_EARNING_FROM_ADS:
              tvTitle = (TextView)root.findViewById(R.id.br_earnings_ads_title);
              tv = (TextView)root.findViewById(R.id.br_earnings_ads_bat);
              tvUSD = (TextView)root.findViewById(R.id.br_earnings_ads_usd);
              text = "<font color=#8E2995>" + value + "</font><font color=#000000> BAT</font>";
              textUSD = usdValue;
              break;
            case BALANCE_REPORT_AUTO_CONTRIBUTE:
              tvTitle = (TextView)root.findViewById(R.id.br_auto_contribute_title);
              tv = (TextView)root.findViewById(R.id.br_auto_contribute_bat);
              tvUSD = (TextView)root.findViewById(R.id.br_auto_contribute_usd);
              text = "<font color=#6537AD>" + value + "</font><font color=#000000> BAT</font>";
              textUSD = usdValue;
              break;
            case BALANCE_REPORT_RECURRING_DONATION:
              tvTitle = (TextView)root.findViewById(R.id.br_recurring_donation_title);
              tv = (TextView)root.findViewById(R.id.br_recurring_donation_bat);
              tvUSD = (TextView)root.findViewById(R.id.br_recurring_donation_usd);
              text = "<font color=#392DD1>" + value + "</font><font color=#000000> BAT</font>";
              textUSD = usdValue;
              break;
            case BALANCE_REPORT_ONE_TIME_DONATION:
              tvTitle = (TextView)root.findViewById(R.id.br_one_time_donation_title);
              tv = (TextView)root.findViewById(R.id.br_one_time_donation_bat);
              tvUSD = (TextView)root.findViewById(R.id.br_one_time_donation_usd);
              text = "<font color=#392DD1>" + value + "</font><font color=#000000> BAT</font>";
              textUSD = usdValue;
              break;
            case BALANCE_REPORT_TOTAL:
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
          if (!report[i].equals("0")) {
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
        ShowNotification(id, type, timestamp, args);
    }

    @Override
    public void OnNotificationDeleted(String id) {
        DismissNotification(id);
        mBraveRewardsNativeWorker.GetWalletProperties();
    }

    @Override
    public void OnIsWalletCreated(boolean created) {
        if (created) {
            walletInitialized = true;
            ShowWebSiteView(false);
            mBraveRewardsNativeWorker.FetchGrants();
        }
        else {
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
                R.string.brave_ui_reserved_amount_text), String.format(Locale.getDefault(), "%.2f", amount)) +
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
    }


    @Override
    public void OnGetAutoContributeProps() {
        mAutoContributeEnabled  = mBraveRewardsNativeWorker.IsAutoContributeEnabled();
        mBraveRewardsNativeWorker.GetRecurringDonations();
    }

    @Override
    public void OnGetReconcileStamp(long timestamp){}


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

        if (mAutoContributeEnabled || mPubInReccuredDonation){
            root.findViewById(R.id.ac_enabled_controls).setVisibility(View.VISIBLE);

            if (mAutoContributeEnabled){
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


    void UpdateRecurentDonationSpinner(double amount){
        int RequestedPosition = mTip_amount_spinner_data_adapter.getPosition ((int)amount);
        if (RequestedPosition < 0 || RequestedPosition >= mTip_amount_spinner_data_adapter.getCount() ){
            Log.e(TAG, "Requested position in Recurrent Donations Spinner doesn't exist");
            return;
        }

        int selectedItemPos = mTip_amount_spinner.getSelectedItemPosition();
        if (RequestedPosition != selectedItemPos){
            mTip_amount_spinner_auto_select = true; //spinner selection was changed programmatically
            mTip_amount_spinner.setSelection(RequestedPosition);
        }
    }
}

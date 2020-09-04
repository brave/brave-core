/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp.widget;

import android.content.Context;
import android.util.Pair;
import android.view.ContextThemeWrapper;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.PopupMenu;
import android.widget.TextView;

import androidx.appcompat.widget.AppCompatImageView;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.ntp.widget.NTPWidgetManager;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.ArrayList;
import java.util.List;

public class NTPWidgetAdapter extends PagerAdapter {
    private List<NTPWidgetItem> widgetList = new ArrayList<NTPWidgetItem>();
    private NTPWidgetListener ntpWidgetListener;
    private Context mContext = ContextUtils.getApplicationContext();
    private Profile mProfile = Profile.getLastUsedRegularProfile();

    public interface NTPWidgetListener {
        void onMenuEdit();

        void onMenuRemove(int position);

        void onBottomSheetDismiss();
    }

    public void setNTPWidgetListener(NTPWidgetListener ntpWidgetListener) {
        this.ntpWidgetListener = ntpWidgetListener;
    }

    @Override
    public int getItemPosition(Object object) {
        int index = widgetList.indexOf(object);
        if (index == -1)
            return POSITION_NONE;
        else
            return index;
    }

    @Override
    public Object instantiateItem(ViewGroup container, final int position) {
        NTPWidgetItem ntpWidgetItem = widgetList.get(position);
        View mainView = ntpWidgetItem.getWidgetView();
        if (mainView != null) {
            AppCompatImageView widgetMoreOption = mainView.findViewById(R.id.widget_more_option);
            if (widgetMoreOption != null) {
                widgetMoreOption.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        showPopupMenu(mContext, view, position);
                    }
                });
            }
            if (ntpWidgetItem.getWidgetType().equals(NTPWidgetManager.PREF_PRIVATE_STATS)) {
                updateBraveStats(mainView);
            }
            container.addView(mainView);
        }
        return mainView;
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object) {
        container.removeView((View) object);
    }

    @Override
    public int getCount() {
        return widgetList.size();
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
        return view == object;
    }

    public void setWidgetList(List<NTPWidgetItem> widgetList) {
        this.widgetList = widgetList;
    }

    public void removeWidgetItem(int position) {
        NTPWidgetItem ntpWidgetItem = widgetList.get(position);
        NTPWidgetManager.getInstance().setWidget(ntpWidgetItem.getWidgetType(), -1);
        widgetList.remove(position);
    }

    public void clearWidgets() {
        widgetList.clear();
    }

    private void updateBraveStats(View view) {
        TextView mAdsBlockedCountTextView =
                (TextView) view.findViewById(R.id.brave_stats_text_ads_count);
        TextView mDataSavedValueTextView =
                (TextView) view.findViewById(R.id.brave_stats_data_saved_value);
        TextView mEstTimeSavedCountTextView =
                (TextView) view.findViewById(R.id.brave_stats_text_time_count);

        long trackersBlockedCount =
                BravePrefServiceBridge.getInstance().getTrackersBlockedCount(mProfile);
        long adsBlockedCount = BravePrefServiceBridge.getInstance().getAdsBlockedCount(mProfile);
        long dataSaved = BravePrefServiceBridge.getInstance().getDataSaved(mProfile);
        long estimatedMillisecondsSaved =
                (trackersBlockedCount + adsBlockedCount) * BraveStatsUtil.MILLISECONDS_PER_ITEM;

        Pair<String, String> adsTrackersPair =
                BraveStatsUtil.getBraveStatsStringFormNumberPair(adsBlockedCount, false);
        Pair<String, String> dataSavedPair =
                BraveStatsUtil.getBraveStatsStringFormNumberPair(dataSaved, true);

        mAdsBlockedCountTextView.setText(
                String.format(mContext.getResources().getString(R.string.ntp_stat_text),
                        adsTrackersPair.first, adsTrackersPair.second));
        mDataSavedValueTextView.setText(
                String.format(mContext.getResources().getString(R.string.ntp_stat_text),
                        dataSavedPair.first, dataSavedPair.second));
        mEstTimeSavedCountTextView.setText(
                BraveStatsUtil.getBraveStatsStringFromTime(estimatedMillisecondsSaved / 1000));
    }

    private void showPopupMenu(Context context, View view, final int position) {
        // Creating the instance of PopupMenu
        PopupMenu popup = new PopupMenu(context, view);
        // Inflating the Popup using xml file
        popup.getMenuInflater().inflate(R.menu.ntp_widget_menu, popup.getMenu());
        // registering popup with OnMenuItemClickListener
        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                int id = item.getItemId();
                if (id == R.id.edit) {
                    ntpWidgetListener.onMenuEdit();
                } else if (id == R.id.remove) {
                    ntpWidgetListener.onMenuRemove(position);
                }
                return true;
            }
        });
        popup.show(); // showing popup menu
    }
}

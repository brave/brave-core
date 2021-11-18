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
import android.widget.LinearLayout;
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
import java.util.Locale;

public class NTPWidgetAdapter extends PagerAdapter {
    private static final String BTC = "BTC";
    private List<NTPWidgetItem> widgetList = new ArrayList<NTPWidgetItem>();
    private NTPWidgetListener ntpWidgetListener;
    private Context mContext = ContextUtils.getApplicationContext();
    private Profile mProfile = Profile.getLastUsedRegularProfile();

    public interface NTPWidgetListener {
        void onMenuEdit();

        void onMenuRemove(int position);
    }

    public void setNTPWidgetListener(NTPWidgetListener ntpWidgetListener) {
        this.ntpWidgetListener = ntpWidgetListener;
    }

    @Override
    public int getItemPosition(Object object) {
        return PagerAdapter.POSITION_NONE;
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
                BraveStatsUtil.updateBraveStatsLayout(mainView);
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
        notifyDataSetChanged();
    }

    private void showPopupMenu(Context context, View view, final int position) {
        PopupMenu popup = new PopupMenu(context, view);
        popup.getMenuInflater().inflate(R.menu.ntp_widget_menu, popup.getMenu());

        NTPWidgetItem ntpWidgetItem = widgetList.get(position);
        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem item) {
                int id = item.getItemId();
                if (id == R.id.edit) {
                    ntpWidgetListener.onMenuEdit();
                } else if (id == R.id.remove) {
                    NTPWidgetItem ntpWidgetItem = widgetList.get(position);
                    removeWidgetItem(position);
                }
                return true;
            }
        });
        popup.show();
    }
}

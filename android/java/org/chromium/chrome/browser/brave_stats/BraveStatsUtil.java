/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_stats;

import android.util.Pair;
import android.view.View;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_stats.BraveStatsBottomSheetDialogFragment;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;

public class BraveStatsUtil {
    public static final short MILLISECONDS_PER_ITEM = 50;
    /*
     * Gets string view of specific time in seconds for Brave stats
     */
    public static Pair<String, String> getBraveStatsStringFromTime(long seconds) {
        String result = "";
        String suffix = "";
        if (seconds > 24 * 60 * 60) {
            result = result + (seconds / (24 * 60 * 60));
            suffix = "d";
        } else if (seconds > 60 * 60) {
            result = result + (seconds / (60 * 60));
            suffix = "h";
        } else if (seconds > 60) {
            result = result + (seconds / 60);
            suffix = "m";
        } else {
            result = result + seconds;
            suffix = "s";
        }
        return new Pair<>(result, suffix);
    }

    public static Pair<String, String> getBraveStatsStringFormNumberPair(
            long number, boolean isBytes) {
        String result = "";
        String suffix = isBytes ? "KB" : "";
        long base = isBytes ? 1024L : 1000L;
        if (number >= base * base * base) {
            result = result + (number / (base * base * base));
            number = number % (base * base * base);
            result = result + "." + (number / (10L * base * base));
            suffix = isBytes ? "GB" : "B";
        } else if (number >= (10L * base * base) && number < (base * base * base)) {
            result = result + (number / (base * base));
            suffix = isBytes ? "MB" : "M";
        } else if (number >= (base * base) && number < (10L * base * base)) {
            result = result + (number / (base * base));
            number = number % (base * base);
            result = result + "." + (number / (100L * base));
            suffix = isBytes ? "MB" : "M";
        } else if (number >= (10L * base) && number < (base * base)) {
            result = result + (number / base);
            suffix = isBytes ? "KB" : "K";
        } else if (number >= base && number < (10L * base)) {
            result = result + (number / base);
            number = number % base;
            result = result + "." + (number / 100L);
            suffix = isBytes ? "KB" : "K";
        } else {
            result = result + number;
        }
        return new Pair<>(result, suffix);
    }

    public static void showBraveStats() {
        if (BraveActivity.getBraveActivity() != null) {
            BraveStatsBottomSheetDialogFragment braveStatsBottomSheetDialogFragment =
                    BraveStatsBottomSheetDialogFragment.newInstance();
            braveStatsBottomSheetDialogFragment.show(
                    BraveActivity.getBraveActivity().getSupportFragmentManager(),
                    "brave_stats_bottom_sheet_dialog_fragment");
        }
    }

    public static String getCalculatedDate(String dateFormat, int days) {
        Calendar cal = Calendar.getInstance();
        SimpleDateFormat s = new SimpleDateFormat(dateFormat, Locale.getDefault());
        cal.add(Calendar.DAY_OF_YEAR, days);
        return s.format(new Date(cal.getTimeInMillis()));
    }

    public static void updateBraveStatsLayout(View view) {
        Profile mProfile = Profile.getLastUsedRegularProfile();
        TextView mAdsBlockedCountTextView =
                (TextView) view.findViewById(R.id.brave_stats_text_ads_count);
        TextView mDataSavedValueTextView =
                (TextView) view.findViewById(R.id.brave_stats_data_saved_value);
        TextView mEstTimeSavedCountTextView =
                (TextView) view.findViewById(R.id.brave_stats_text_time_count);
        TextView mAdsBlockedCountTextTextView =
                (TextView) view.findViewById(R.id.brave_stats_text_ads_count_text);
        TextView mDataSavedValueTextTextView =
                (TextView) view.findViewById(R.id.brave_stats_data_saved_value_text);
        TextView mEstTimeSavedCountTextTextView =
                (TextView) view.findViewById(R.id.brave_stats_text_time_count_text);

        long trackersBlockedCount =
                BravePrefServiceBridge.getInstance().getTrackersBlockedCount(mProfile);
        long adsBlockedCount = BravePrefServiceBridge.getInstance().getAdsBlockedCount(mProfile);
        long dataSaved = BravePrefServiceBridge.getInstance().getDataSaved(mProfile);
        long estimatedMillisecondsSaved =
                (trackersBlockedCount + adsBlockedCount) * MILLISECONDS_PER_ITEM;

        Pair<String, String> adsTrackersPair =
                getBraveStatsStringFormNumberPair(adsBlockedCount, false);
        Pair<String, String> dataSavedPair = getBraveStatsStringFormNumberPair(dataSaved, true);
        Pair<String, String> timeSavedPair =
                getBraveStatsStringFromTime(estimatedMillisecondsSaved / 1000);

        mAdsBlockedCountTextView.setText(adsTrackersPair.first);
        mDataSavedValueTextView.setText(dataSavedPair.first);
        mEstTimeSavedCountTextView.setText(timeSavedPair.first);
        mAdsBlockedCountTextTextView.setText(adsTrackersPair.second);
        mDataSavedValueTextTextView.setText(dataSavedPair.second);
        mEstTimeSavedCountTextTextView.setText(timeSavedPair.second);
    }
}
/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_stats;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.MediaStore;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_stats.BraveStatsBottomSheetDialogFragment;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.shields.BraveShieldsUtils;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class BraveStatsUtil {
    public static final short MILLISECONDS_PER_ITEM = 50;
    public static final int SHARE_STATS_WRITE_EXTERNAL_STORAGE_PERM = 3867;
    public static final String TAG = "BraveStatsUtil";
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

        List<Pair<String, String>> statsPairs = getStatsPairs();

        mAdsBlockedCountTextView.setText(statsPairs.get(0).first);
        mDataSavedValueTextView.setText(statsPairs.get(1).first);
        mEstTimeSavedCountTextView.setText(statsPairs.get(2).first);
        mAdsBlockedCountTextTextView.setText(statsPairs.get(0).second);
        mDataSavedValueTextTextView.setText(statsPairs.get(1).second);
        mEstTimeSavedCountTextTextView.setText(statsPairs.get(2).second);
    }

    public static void updateBraveShareStatsLayoutAndShare(View view) {
        TextView mAdsBlockedCountTextView = (TextView) view.findViewById(R.id.stats_trackers_no);
        TextView mDataSavedValueTextView = (TextView) view.findViewById(R.id.stats_saved_data_no);
        TextView mEstTimeSavedCountTextView = (TextView) view.findViewById(R.id.stats_timed_no);

        List<Pair<String, String>> statsPairs = getStatsPairs();
        String trackersString =
                String.format("%s %s", statsPairs.get(0).first, statsPairs.get(0).second);
        String dataSavedString =
                String.format("%s %s", statsPairs.get(1).first, statsPairs.get(1).second);
        String timeSavedString =
                String.format("%s %s", statsPairs.get(2).first, statsPairs.get(2).second);

        mAdsBlockedCountTextView.setText(trackersString);
        mDataSavedValueTextView.setText(dataSavedString);
        mEstTimeSavedCountTextView.setText(timeSavedString);
        shareStatsAction(view);
    }

    public static void shareStatsAction(View view) {
        try {
            Context context = ContextUtils.getApplicationContext();
            Bitmap bmp = convertToBitmap(view);
            String path = "";
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                path = MediaStore.Images.Media.insertImage(
                        context.getContentResolver(), bmp, "tempimage", null);
            } else {
                storeImage(bmp);
                path = getOutputMediaFile().getAbsolutePath();
            }

            Uri uri = Uri.parse(path);

            Intent sendIntent = new Intent();
            sendIntent.setAction(Intent.ACTION_SEND);
            sendIntent.putExtra(Intent.EXTRA_TEXT,
                    context.getResources().getString(R.string.brave_stats_share_text));
            sendIntent.putExtra(Intent.EXTRA_STREAM, uri);
            sendIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            sendIntent.setType("image/text");

            Intent shareIntent = Intent.createChooser(sendIntent, " ");
            shareIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(shareIntent);
        } catch (Exception e) {
            Log.e(TAG, "exception :" + e);
        }
    }

    private static void storeImage(Bitmap image) {
        File pictureFile = getOutputMediaFile();
        if (pictureFile == null) {
            Log.e(TAG, "Error creating media file, check storage permissions: ");
            return;
        }
        try {
            FileOutputStream fos = new FileOutputStream(pictureFile);
            image.compress(Bitmap.CompressFormat.PNG, 90, fos);
            fos.close();
        } catch (FileNotFoundException e) {
            Log.e(TAG, "File not found: " + e.getMessage());
        } catch (IOException e) {
            Log.e(TAG, "Error accessing file: " + e.getMessage());
        }
    }

    /** Create a File for saving an image or video */
    private static File getOutputMediaFile() {
        File mediaStorageDir = new File(Environment.getExternalStorageDirectory() + "/Android/data/"
                + ContextUtils.getApplicationContext().getPackageName() + "/Files");

        if (!mediaStorageDir.exists()) {
            if (!mediaStorageDir.mkdirs()) {
                return null;
            }
        }

        File mediaFile;
        String mImageName = "share_stats.jpg";
        mediaFile = new File(mediaStorageDir.getPath() + File.separator + mImageName);
        return mediaFile;
    }

    public static View getLayout(int layoutId) {
        Context context = ContextUtils.getApplicationContext();
        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View layout = inflater.inflate(layoutId, null);

        return layout;
    }

    private static Bitmap convertToBitmap(View view) {
        view.measure(View.MeasureSpec.UNSPECIFIED, View.MeasureSpec.UNSPECIFIED);
        int totalHeight = view.getMeasuredHeight();
        int totalWidth = view.getMeasuredWidth();

        Bitmap canvasBitmap = Bitmap.createBitmap(totalWidth, totalHeight, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(canvasBitmap);
        view.layout(0, 0, view.getMeasuredWidth(), view.getMeasuredHeight());
        view.draw(canvas);

        return canvasBitmap;
    }

    public static boolean hasWritePermission(Activity activity) {
        if (ActivityCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED) {
            return true;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            activity.requestPermissions(new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    SHARE_STATS_WRITE_EXTERNAL_STORAGE_PERM);
        }

        return false;
    }

    public static void shareStats(int layout) {
        View shareStatsLayout = getLayout(layout);
        updateBraveShareStatsLayoutAndShare(shareStatsLayout);
    }

    private static List<Pair<String, String>> getStatsPairs() {
        List<Pair<String, String>> statsPair = new ArrayList<>();
        Profile mProfile = Profile.getLastUsedRegularProfile();
        long trackersBlockedCount =
                BravePrefServiceBridge.getInstance().getTrackersBlockedCount(mProfile);
        long adsBlockedCount = BravePrefServiceBridge.getInstance().getAdsBlockedCount(mProfile);
        long adsTrackersBlockedCount = trackersBlockedCount + adsBlockedCount;
        long dataSaved = BravePrefServiceBridge.getInstance().getDataSaved(mProfile);
        long estimatedMillisecondsSaved =
                (trackersBlockedCount + adsBlockedCount) * MILLISECONDS_PER_ITEM;

        Pair<String, String> adsTrackersPair =
                getBraveStatsStringFormNumberPair(adsTrackersBlockedCount, false);
        Pair<String, String> dataSavedPair = getBraveStatsStringFormNumberPair(dataSaved, true);
        Pair<String, String> timeSavedPair =
                getBraveStatsStringFromTime(estimatedMillisecondsSaved / 1000);
        statsPair.add(adsTrackersPair);
        statsPair.add(dataSavedPair);
        statsPair.add(timeSavedPair);

        return statsPair;
    }

    public static Pair<String, String> getAdsTrackersBlocked() {
        Profile mProfile = Profile.getLastUsedRegularProfile();
        long trackersBlockedCount =
                BravePrefServiceBridge.getInstance().getTrackersBlockedCount(mProfile);
        long adsBlockedCount = BravePrefServiceBridge.getInstance().getAdsBlockedCount(mProfile);
        long adsTrackersBlockedCount = trackersBlockedCount + adsBlockedCount;

        return getBraveStatsStringFormNumberPair(adsTrackersBlockedCount, false);
    }
}

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.local_database;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.util.Pair;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.util.ArrayList;
import java.util.List;

import org.chromium.base.Log;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ntp_background_images.model.TopSite;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;

public class DatabaseHelper extends SQLiteOpenHelper {

    private static volatile DatabaseHelper mInstance;

    // Database Version
    private static final int DATABASE_VERSION = 2;

    // Database Name
    private static final String DATABASE_NAME = "brave_db";

    public static DatabaseHelper getInstance() {
        synchronized (DatabaseHelper.class) {
            if (mInstance == null) {
                Context context = ContextUtils.getApplicationContext();
                mInstance = new DatabaseHelper(context);
            }
        }
        return mInstance;
    }

    public DatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    // Creating Tables
    @Override
    public void onCreate(SQLiteDatabase db) {

        // create notes table
        db.execSQL(TopSiteTable.CREATE_TABLE);
        db.execSQL(BraveStatsTable.CREATE_TABLE);
        db.execSQL(SavedBandwidthTable.CREATE_TABLE);
    }

    // Upgrading database
    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        onCreate(db);
    }

    private boolean isTopSiteAlreadyAdded(String destinationUrl) {
        SQLiteDatabase sqldb = this.getReadableDatabase();
        String query = "Select * from " + TopSiteTable.TABLE_NAME + " where " + TopSiteTable.COLUMN_DESTINATION_URL + " =?";
        Cursor cursor = sqldb.rawQuery(query, new String[] {destinationUrl});
        if (cursor.getCount() <= 0) {
            cursor.close();
            return false;
        }
        cursor.close();
        return true;
    }

    public void insertTopSite(TopSite topSite) {
        if (!isTopSiteAlreadyAdded(topSite.getDestinationUrl()) && !NTPUtil.isInRemovedTopSite(topSite.getDestinationUrl())) {
            // get writable database as we want to write data
            SQLiteDatabase db = this.getWritableDatabase();

            ContentValues values = new ContentValues();
            values.put(TopSiteTable.COLUMN_NAME, topSite.getName());
            values.put(TopSiteTable.COLUMN_DESTINATION_URL, topSite.getDestinationUrl());
            values.put(TopSiteTable.COLUMN_BACKGROUND_COLOR, topSite.getBackgroundColor());
            values.put(TopSiteTable.COLUMN_IMAGE_PATH, topSite.getImagePath());

            // insert row
            db.insert(TopSiteTable.TABLE_NAME, null, values);
        }
    }

    public List<TopSiteTable> getAllTopSites() {
        List<TopSiteTable> topSites = new ArrayList<>();

        // Select All Query
        String selectQuery = "SELECT  * FROM " + TopSiteTable.TABLE_NAME;

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);

        // looping through all rows and adding to list
        if (cursor.moveToFirst()) {
            do {
                TopSiteTable topSite = new TopSiteTable(
                    cursor.getString(cursor.getColumnIndex(TopSiteTable.COLUMN_NAME)),
                    cursor.getString(cursor.getColumnIndex(TopSiteTable.COLUMN_DESTINATION_URL)),
                    cursor.getString(cursor.getColumnIndex(TopSiteTable.COLUMN_BACKGROUND_COLOR)),
                    cursor.getString(cursor.getColumnIndex(TopSiteTable.COLUMN_IMAGE_PATH)));

                topSites.add(topSite);
            } while (cursor.moveToNext());
        }

        cursor.close();

        return topSites;
    }

    public int getTopSitesCount() {
        String countQuery = "SELECT  * FROM " + TopSiteTable.TABLE_NAME;
        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(countQuery, null);

        int count = cursor.getCount();
        cursor.close();

        // return count
        return count;
    }

    public void deleteTopSite(String destinationUrl) {
        SQLiteDatabase db = this.getWritableDatabase();
        db.delete(TopSiteTable.TABLE_NAME, TopSiteTable.COLUMN_DESTINATION_URL + " = ?",
                  new String[] {destinationUrl});
    }

    public long insertStats(BraveStatsTable braveStat) {
        // if (!isAdsTrackerAlreadyAdded(braveStat)) {
        SQLiteDatabase db = this.getWritableDatabase();

        ContentValues values = new ContentValues();
        values.put(BraveStatsTable.COLUMN_URL, braveStat.getUrl());
        values.put(BraveStatsTable.COLUMN_DOMAIN, braveStat.getDomain());
        values.put(BraveStatsTable.COLUMN_STAT_TYPE, braveStat.getStatType());
        values.put(BraveStatsTable.COLUMN_STAT_SITE, braveStat.getStatSite());
        values.put(BraveStatsTable.COLUMN_STAT_SITE_DOMAIN, braveStat.getStatSiteDomain());
        values.put(BraveStatsTable.COLUMN_TIMESTAMP, braveStat.getTimestamp());

        return  db.insert(BraveStatsTable.TABLE_NAME, null, values);
        // }
        // return -1;
    }

    private boolean isAdsTrackerAlreadyAdded(BraveStatsTable braveStat) {

        String sql = "SELECT * FROM "
                     + BraveStatsTable.TABLE_NAME
                     + " WHERE " + BraveStatsTable.COLUMN_STAT_SITE + " = '" + braveStat.getStatSite() + "'"
                     + " AND " + BraveStatsTable.COLUMN_URL + " = '" + braveStat.getUrl() + "'";

        Log.e("NTP", sql);

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(sql, null);

        int count = cursor.getCount();
        cursor.close();

        return count > 0;
    }

    public List<BraveStatsTable> getAllStats() {
        List<BraveStatsTable> braveStats = new ArrayList<>();

        // Select All Query
        String selectQuery = "SELECT  * FROM "
                             + BraveStatsTable.TABLE_NAME;

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);

        // looping through all rows and adding to list
        if (cursor.moveToFirst()) {
            do {
                BraveStatsTable braveStat = new BraveStatsTable(
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_URL)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_DOMAIN)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_STAT_TYPE)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_STAT_SITE)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_STAT_SITE_DOMAIN)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_TIMESTAMP)));

                braveStats.add(braveStat);
            } while (cursor.moveToNext());
        }

        cursor.close();

        return braveStats;
    }

    public List<BraveStatsTable> getAllStatsWithDate(String thresholdTime, String currentTime) {
        List<BraveStatsTable> braveStats = new ArrayList<>();
        // Select All Query
        String selectQuery = "SELECT  * FROM "
                             + BraveStatsTable.TABLE_NAME
                             + " WHERE " + BraveStatsTable.COLUMN_TIMESTAMP
                             + " BETWEEN date('" + thresholdTime + "') AND date('" + currentTime + "')";

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);

        // looping through all rows and adding to list
        if (cursor.moveToFirst()) {
            do {
                BraveStatsTable braveStat = new BraveStatsTable(
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_URL)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_DOMAIN)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_STAT_TYPE)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_STAT_SITE)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_STAT_SITE_DOMAIN)),
                    cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_TIMESTAMP)));

                braveStats.add(braveStat);
            } while (cursor.moveToNext());
        }

        cursor.close();

        return braveStats;
    }

    public List<Pair<String, Integer>> getStatsWithDate(String thresholdTime, String currentTime) {
        List<Pair<String, Integer>> braveStats = new ArrayList<>();

        String selectQuery = "SELECT  " + BraveStatsTable.COLUMN_DOMAIN + ", " + BraveStatsTable.COLUMN_TIMESTAMP + " , COUNT(*) as stat_count FROM "
                             + BraveStatsTable.TABLE_NAME
                             + " WHERE " + BraveStatsTable.COLUMN_TIMESTAMP
                             + " BETWEEN date('" + thresholdTime + "') AND date('" + currentTime + "')"
                             + " GROUP BY " + BraveStatsTable.COLUMN_DOMAIN
                             + " ORDER BY stat_count DESC";

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);

        if (cursor.moveToFirst()) {
            do {
                Pair<String, Integer> statPair = new Pair<>(cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_DOMAIN)), cursor.getInt(cursor.getColumnIndex("stat_count")));
                braveStats.add(statPair);
            } while (cursor.moveToNext());
        }

        cursor.close();

        return braveStats;
    }

    public List<Pair<String, Integer>> getSitesWithDate(String thresholdTime, String currentTime) {
        List<Pair<String, Integer>> braveStats = new ArrayList<>();
        // Select All Query
        String selectQuery = "SELECT  " + BraveStatsTable.COLUMN_STAT_SITE_DOMAIN + ", COUNT(*) as site_count FROM "
                             + BraveStatsTable.TABLE_NAME
                             + " WHERE " + BraveStatsTable.COLUMN_TIMESTAMP
                             + " BETWEEN date('" + thresholdTime + "') AND date('" + currentTime + "')"
                             + " GROUP BY " + BraveStatsTable.COLUMN_STAT_SITE_DOMAIN
                             + " ORDER BY site_count DESC";

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);

        // looping through all rows and adding to list
        if (cursor.moveToFirst()) {
            do {
                Pair<String, Integer> statPair = new Pair<>(cursor.getString(cursor.getColumnIndex(BraveStatsTable.COLUMN_STAT_SITE_DOMAIN)), cursor.getInt(cursor.getColumnIndex("site_count")));
                braveStats.add(statPair);
            } while (cursor.moveToNext());
        }

        cursor.close();

        return braveStats;
    }

    public void clearStatsTable() {
        String selectQuery = "DELETE FROM " + BraveStatsTable.TABLE_NAME;
        SQLiteDatabase db = this.getWritableDatabase();
        db.execSQL(selectQuery);
    }

    public long insertSavedBandwidth(SavedBandwidthTable savedBandwidthTable) {
        // get writable database as we want to write data
        SQLiteDatabase db = this.getWritableDatabase();

        ContentValues values = new ContentValues();
        values.put(SavedBandwidthTable.COLUMN_SAVED_BANDWIDTH, savedBandwidthTable.getSavedBandwidth());
        values.put(SavedBandwidthTable.COLUMN_TIMESTAMP, savedBandwidthTable.getTimestamp());

        // insert row
        return db.insert(SavedBandwidthTable.TABLE_NAME, null, values);
    }

    public long getTotalSavedBandwidthWithDate(String thresholdTime, String currentTime) {
        long sum = 0;
        String selectQuery = "SELECT  SUM(" + SavedBandwidthTable.COLUMN_SAVED_BANDWIDTH + ") as total FROM "
                             + SavedBandwidthTable.TABLE_NAME
                             + " WHERE " + BraveStatsTable.COLUMN_TIMESTAMP
                             + " BETWEEN date('" + thresholdTime + "') AND date('" + currentTime + "')";

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);

        if (cursor.moveToFirst())
            sum = cursor.getLong(cursor.getColumnIndex("total"));

        cursor.close();
        return sum;
    }

    public long getTotalSavedBandwidth() {
        long sum = 0;
        String selectQuery = "SELECT  SUM(" + SavedBandwidthTable.COLUMN_SAVED_BANDWIDTH + ") as total FROM "
                             + SavedBandwidthTable.TABLE_NAME;

        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);

        if (cursor.moveToFirst())
            sum = cursor.getLong(cursor.getColumnIndex("total"));

        cursor.close();
        return sum;
    }

    public void clearSavedBandwidthTable() {
        String selectQuery = "DELETE FROM " + SavedBandwidthTable.TABLE_NAME;
        SQLiteDatabase db = this.getWritableDatabase();
        db.execSQL(selectQuery);
    }
}
/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.local_database;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.util.ArrayList;
import java.util.List;

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
    	if (mInstance == null) {
    		Context context = ContextUtils.getApplicationContext();
    		mInstance = new DatabaseHelper(context);
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
	        if(cursor.getCount() <= 0){
	            cursor.close();
	            return false;
	        }
	    cursor.close();
	    return true;
    }

    public void insertTopSite(TopSite topSite) {
    	if(!isTopSiteAlreadyAdded(topSite.getDestinationUrl()) && !NTPUtil.isInRemovedTopSite(topSite.getDestinationUrl())) {
    		// get writable database as we want to write data
	        SQLiteDatabase db = this.getWritableDatabase();

	        ContentValues values = new ContentValues();
	        values.put(TopSiteTable.COLUMN_NAME, topSite.getName());
	        values.put(TopSiteTable.COLUMN_DESTINATION_URL, topSite.getDestinationUrl());
	        values.put(TopSiteTable.COLUMN_BACKGROUND_COLOR, topSite.getBackgroundColor());
	        values.put(TopSiteTable.COLUMN_IMAGE_PATH, topSite.getImagePath());

	        // insert row
	        db.insert(TopSiteTable.TABLE_NAME, null, values);

	        // close db connection
	        db.close();
    	}
    }

    public List<TopSiteTable> getAllTopSites() {
        List<TopSiteTable> topSites = new ArrayList<>();

        // Select All Query
        String selectQuery = "SELECT  * FROM " + TopSiteTable.TABLE_NAME;

        SQLiteDatabase db = this.getWritableDatabase();
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

        // close db connection
        db.close();

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
                new String[]{destinationUrl});
        db.close();
    }
}
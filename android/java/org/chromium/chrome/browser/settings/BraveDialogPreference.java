/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;

import androidx.preference.DialogPreference;
import org.chromium.chrome.R;

public class BraveDialogPreference extends DialogPreference {
    private CharSequence[] mDialogEntries;
    private String mDialogSubs;
    private int mCheckedIndex;

    public void setDialogEntries(CharSequence[] mDialogEntries) {
        this.mDialogEntries = mDialogEntries;
    }

    public BraveDialogPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        setDialogLayoutResource(R.layout.brave_dialog_preference);
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.brave_list_preference);

        // Sets dialog entries
        CharSequence[] entries = a.getTextArray(R.styleable.brave_list_preference_dialog_entries);
        if (entries != null) {
            this.setDialogEntries(entries);
        }

        // Sets dialog subtitle
        String subtitle = a.getString(R.styleable.brave_list_preference_dialog_subtitle);
        if (subtitle != null) {
            this.setDialogSubtitle(subtitle);
        }

        // Sets default checked index
        int defaultChecked = a.getInt(R.styleable.brave_list_preference_dialog_default_index, 1);
        this.setCheckedIndex(defaultChecked);

        a.recycle();
    }

    public String getDialogSubtitle() {
        return mDialogSubs;
    }

    public void setDialogSubtitle(String mDialogSubs) {
        this.mDialogSubs = mDialogSubs;
    }

    public CharSequence[] getDialogEntries() {
        return mDialogEntries;
    }

    public int getCheckedIndex() {
        return mCheckedIndex;
    }

    public void setCheckedIndex(int mCheckedIndex) {
        this.mCheckedIndex = mCheckedIndex;
    }
}

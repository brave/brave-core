/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RadioGroup;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.ai_chat.mojom.ModelWithSubtitle;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescriptionLayout;

import java.util.ArrayList;
import java.util.List;

public class BraveLeoRadioButtonGroupDefaultModelPreference extends Preference
        implements RadioGroup.OnCheckedChangeListener {
    /** This is a delegate that is implemented in BraveLeoDefaultModelPreferences */
    public interface RadioButtonsDelegate {
        default void fetchModels() {}

        default void setDefaultModel(String key) {}
    }

    private RadioButtonWithDescriptionLayout mGroup;
    private ModelWithSubtitle[] mModels;
    private String mDefaultModelKey;
    private final Context mContext;
    RadioButtonsDelegate mDelegate;

    public BraveLeoRadioButtonGroupDefaultModelPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        setLayoutResource(R.xml.brave_leo_radio_button_group_default_model_preference);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        mGroup = (RadioButtonWithDescriptionLayout) holder.findViewById(R.id.radio_button_layout);
        assert mGroup != null;
        mGroup.setOnCheckedChangeListener(this);
        assert mDelegate != null;
        mDelegate.fetchModels();
    }

    public void initialize(RadioButtonsDelegate delegate) {
        mDelegate = delegate;
    }

    public void initializeModels(ModelWithSubtitle[] models, String defaultModel) {
        mModels = models;
        mDefaultModelKey = defaultModel;
        if (mModels != null) {
            List<RadioButtonWithDescription> buttons = new ArrayList<RadioButtonWithDescription>();
            for (int i = 0; i < mModels.length; i++) {
                buttons.add(
                        createRadioButtonWithDescription(
                                mModels[i].model.displayName,
                                mModels[i].subtitle,
                                mModels[i].model.key,
                                mModels[i].model.key.equals(mDefaultModelKey),
                                i));
            }
            mGroup.addButtons(buttons);
        }
    }

    private RadioButtonWithDescription createRadioButtonWithDescription(
            String primary, String description, Object tag, boolean checked, int id) {
        RadioButtonWithDescription b = new RadioButtonWithDescription(mContext, null);
        b.setPrimaryText(primary);
        b.setDescriptionText(description);
        b.setTag(tag);
        b.setId(id);
        b.setChecked(checked);
        return b;
    }

    @Override
    public void onCheckedChanged(RadioGroup radioGroup, int i) {
        mDefaultModelKey = mModels[i].model.key;
        mDelegate.setDefaultModel(mModels[i].model.key);
    }
}

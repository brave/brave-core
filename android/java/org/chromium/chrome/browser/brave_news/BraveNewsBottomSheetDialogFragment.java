/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_news.mojom.BraveNewsController;
import org.chromium.brave_news.mojom.UserEnabled;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.util.TabUtils;

public class BraveNewsBottomSheetDialogFragment extends BottomSheetDialogFragment {
    private String mUrl;
    private String mPublisherId;
    private String mPublisherName;
    private BraveNewsController mBraveNewsController;
    private Context mContext;

    public static BraveNewsBottomSheetDialogFragment newInstance() {
        return new BraveNewsBottomSheetDialogFragment();
    }

    public void setController(BraveNewsController controller) {
        mBraveNewsController = controller;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(STYLE_NORMAL, R.style.AppBottomSheetDialogTheme);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        if (getArguments() != null) {
            mUrl = getArguments().getString("url");
            mPublisherId = getArguments().getString("publisherId");
            mPublisherName = getArguments().getString("publisherName");
        }
        View view = inflater.inflate(R.layout.brave_news_card_menu, container, false);
        return view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mContext = context;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mContext = null;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        ((BottomSheetDialog) getDialog())
                .getBehavior()
                .setState(BottomSheetBehavior.STATE_EXPANDED);
        TextView title = view.findViewById(R.id.item_menu_title_text);
        Button newTab = view.findViewById(R.id.new_tab);
        Button privateTab = view.findViewById(R.id.new_private_tab);
        Button disable = view.findViewById(R.id.disable_content);
        Button share = view.findViewById(R.id.share_content);

        title.setText(mUrl);
        StringBuilder disableText = new StringBuilder();
        disableText.append(
                getResources().getString(R.string.brave_news_disable_content, mPublisherName));
        disable.setText(disableText);

        newTab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TabUtils.openUrlInNewTabInBackground(false, mUrl);
                dismiss();
            }
        });

        privateTab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TabUtils.openUrlInNewTab(true, mUrl);
                dismiss();
            }
        });

        disable.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                PostTask.postTask(TaskTraits.BEST_EFFORT, () -> {
                    if (mBraveNewsController != null) {
                        // Removes the news source from the fetch list by setting a
                        // UserEnabled.DISABLED prop for the publisher in question
                        SharedPreferencesManager.getInstance().writeBoolean(
                                BravePreferenceKeys.BRAVE_NEWS_CHANGE_SOURCE, true);
                        mBraveNewsController.setPublisherPref(mPublisherId, UserEnabled.DISABLED);
                        BraveNewsUtils.disableFollowingPublisherList(mPublisherId);
                        BraveNewsUtils.setFollowingPublisherList();
                    }
                });
                dismiss();
                Toast.makeText(mContext,
                             getResources().getString(
                                     R.string.brave_news_disabled_content, mPublisherName),
                             Toast.LENGTH_SHORT)
                        .show();
            }
        });

        share.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent share = new Intent(android.content.Intent.ACTION_SEND);
                share.setType("text/plain");
                share.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                share.putExtra(Intent.EXTRA_TEXT, mUrl);
                startActivity(Intent.createChooser(share, mUrl));
            }
        });
    }
}

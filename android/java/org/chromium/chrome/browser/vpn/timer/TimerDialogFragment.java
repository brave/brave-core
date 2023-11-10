/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.timer;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.chrome.R;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class TimerDialogFragment extends BottomSheetDialogFragment {
    public static final String TAG = TimerDialogFragment.class.getName();

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_timer_dialog_list_dialog, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        final RecyclerView recyclerView = (RecyclerView) view.findViewById(R.id.timer_action_list);
        recyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        List<TimerItemModel> timerItemModels = new ArrayList<>(Arrays.asList(
                new TimerItemModel(requireContext().getString(R.string.pause_for_15_minutes),
                        R.drawable.ic_pause_filled),
                new TimerItemModel(requireContext().getString(R.string.pause_for_1_hour),
                        R.drawable.ic_pause_filled),
                new TimerItemModel(
                        requireContext().getString(R.string.disable), R.drawable.ic_disabled)));
        recyclerView.setAdapter(new TimerItemAdapter(timerItemModels));

        View cancelView = view.findViewById(R.id.cancel_view);
        TextView timerActionText = cancelView.findViewById(R.id.timer_action_text);
        ImageView timerActionImage = cancelView.findViewById(R.id.timer_action_image);
        TimerItemModel timerItemModel = new TimerItemModel(
                requireContext().getString(android.R.string.cancel), R.drawable.ic_cancel_filled);
        timerActionText.setText(timerItemModel.getActionText());
        timerActionImage.setImageResource(timerItemModel.getActionImage());
    }
}

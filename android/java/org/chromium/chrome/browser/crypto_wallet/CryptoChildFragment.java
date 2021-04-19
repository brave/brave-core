/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import android.annotation.SuppressLint;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;

// import com.github.mikephil.charting.charts.LineChart;
// import com.github.mikephil.charting.components.LimitLine;
// import com.github.mikephil.charting.components.XAxis;
// import com.github.mikephil.charting.components.YAxis;
// import com.github.mikephil.charting.data.Entry;
// import com.github.mikephil.charting.data.LineData;
// import com.github.mikephil.charting.data.LineDataSet;
// import com.github.mikephil.charting.interfaces.datasets.ILineDataSet;

import java.util.ArrayList;

public class CryptoChildFragment extends Fragment {
    // private LineChart mChart;

    public static CryptoChildFragment newInstance() {
        return new CryptoChildFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_crypto_child, container, false);
    }

    @SuppressLint("SetTextI18n")
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        assert getActivity() != null;

        // mChart = getActivity().findViewById(R.id.chart);
        // mChart.setTouchEnabled(true);
        // mChart.setPinchZoom(true);
        // mChart.getAxisLeft().setDrawAxisLine(false);
        // mChart.getXAxis().setDrawAxisLine(false);

        // mChart.getDescription().setEnabled(false);
        // mChart.setDrawGridBackground(false);

        // mChart.getXAxis().setDrawGridLines(false);
        // mChart.getAxisLeft().setDrawGridLines(false);
        // mChart.getAxisRight().setDrawGridLines(false);
        // mChart.getAxisRight().setDrawLimitLinesBehindData(false);
        // mChart.getAxisLeft().setDrawLabels(false);
        // mChart.getAxisRight().setDrawLabels(false);
        // mChart.getXAxis().setDrawLabels(false);
        // mChart.getXAxis().setDrawLimitLinesBehindData(false);
        // mChart.getLegend().setEnabled(false);

        // renderData();

        setUpCoinList(view);
    }

    private void setUpCoinList(View view) {
        RecyclerView rvContacts = (RecyclerView) view.findViewById(R.id.rvCoins);
        // Create adapter passing in the sample user data
        WalletCoinAdapter walletCoinAdapter = new WalletCoinAdapter();
        // Attach the adapter to the recyclerview to populate items
        rvContacts.setAdapter(walletCoinAdapter);
        // Set layout manager to position the items
        rvContacts.setLayoutManager(new LinearLayoutManager(getActivity()));

    }

    // public void renderData() {
    //     LimitLine llXAxis = new LimitLine(10f, "Index 10");
    //     llXAxis.setLineWidth(4f);
    //     llXAxis.setTextSize(10f);

    //     XAxis xAxis = mChart.getXAxis();
    //     xAxis.setAxisMaximum(10f);
    //     xAxis.setAxisMinimum(0f);

    //     YAxis leftAxis = mChart.getAxisLeft();
    //     leftAxis.removeAllLimitLines();
    //     leftAxis.setAxisMaximum(350f);
    //     leftAxis.setAxisMinimum(0f);

    //     mChart.getAxisRight().setEnabled(false);
    //     setData();
    // }

    // private void setData() {
    //     assert getActivity() != null;
    //     ArrayList<Entry> values = new ArrayList<>();
    //     values.add(new Entry(1, 50));
    //     values.add(new Entry(2, 100));
    //     values.add(new Entry(3, 80));
    //     values.add(new Entry(4, 120));
    //     values.add(new Entry(5, 110));
    //     values.add(new Entry(7, 150));
    //     values.add(new Entry(8, 250));
    //     values.add(new Entry(9, 190));

    //     LineDataSet set1;
    //     if (mChart.getData() != null &&
    //             mChart.getData().getDataSetCount() > 0) {
    //         set1 = (LineDataSet) mChart.getData().getDataSetByIndex(0);
    //         set1.setValues(values);
    //         mChart.getData().notifyDataChanged();
    //         mChart.notifyDataSetChanged();
    //     } else {
    //         set1 = new LineDataSet(values, "Sample Data");
    //         set1.setColor(Color.DKGRAY);
    //         set1.setCircleColor(Color.DKGRAY);

    //         Drawable drawable = ContextCompat.getDrawable(getActivity(), R.drawable.fade_blue);
    //         set1.setFillDrawable(drawable);
    //         ArrayList<ILineDataSet> dataSets = new ArrayList<>();
    //         dataSets.add(set1);
    //         LineData data = new LineData(dataSets);
    //         mChart.setData(data);
    //     }
    // }
}

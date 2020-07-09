/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_stats;

import android.util.Pair;

import org.chromium.chrome.browser.BraveActivity;
import org.chromium.chrome.browser.brave_stats.BraveStatsBottomSheetDialogFragment;

public class BraveStatsUtil {
	/*
	* Gets string view of specific number for Brave stats
	*/
	public static String getBraveStatsStringFormNumber(long number, boolean isBytes) {
		String result = "";
		String suffix = "";
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
		result = result + suffix;
		return result;
	}

	/*
	* Gets string view of specific time in seconds for Brave stats
	*/
	public static String getBraveStatsStringFromTime(long seconds) {
		String result = "";
		if (seconds > 24 * 60 * 60) {
			result = result + (seconds / (24 * 60 * 60)) + "d";
		} else if (seconds > 60 * 60) {
			result = result + (seconds / (60 * 60)) + "h";
		} else if (seconds > 60) {
			result = result + (seconds / 60) + "m";
		} else {
			result = result + seconds + "s";
		}
		return result;
	}

	public static Pair<String, String> getBraveStatsStringFormNumberPair(long number, boolean isBytes) {
		String result = "";
		String suffix = "";
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
		// result = result + suffix;
		return new Pair<>(result, suffix);
	}

	/*
	* Gets string view of specific time in seconds for Brave stats
	*/
	public static Pair<String, String> getBraveStatsStringFromTimePair(long seconds) {
		String result = "";
		String suffix = "";
		if (seconds > 24 * 60 * 60) {
			result = result + (seconds / (24 * 60 * 60));
			suffix = "Days";
		} else if (seconds > 60 * 60) {
			result = result + (seconds / (60 * 60));
			suffix = "Hours";
		} else if (seconds > 60) {
			result = result + (seconds / 60);
			suffix = "Minutes";
		} else {
			result = result + seconds;
			suffix = "Seconds";
		}
		return new Pair<>(result, suffix);
	}

	public static void showBraveStats() {
		if (BraveActivity.getBraveActivity() != null) {
			BraveStatsBottomSheetDialogFragment braveStatsBottomSheetDialogFragment = BraveStatsBottomSheetDialogFragment.newInstance();
			braveStatsBottomSheetDialogFragment.show(BraveActivity.getBraveActivity().getSupportFragmentManager(), "brave_stats_bottom_sheet_dialog_fragment");
			braveStatsBottomSheetDialogFragment.setCancelable(false);
		}
	}
}
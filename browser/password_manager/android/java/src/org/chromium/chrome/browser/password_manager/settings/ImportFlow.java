/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.password_manager.settings;

import android.app.Activity;
import android.content.Intent;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Bundle;

import androidx.annotation.IntDef;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.FragmentManager;

import org.chromium.base.task.AsyncTask;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.password_manager.R;
import org.chromium.chrome.browser.profiles.Profile;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * This class allows to trigger and complete the UX flow for importing passwords from a CSV file. A
 * Fragment can use it to display the flow UI over the fragment.
 */
@NullMarked
public class ImportFlow {
    @IntDef({
        ImportState.INACTIVE,
        ImportState.REQUESTED,
        ImportState.IN_PROGRESS,
        ImportState.FINISHED
    })
    @Retention(RetentionPolicy.SOURCE)
    private @interface ImportState {
        /**
         * INACTIVE: there is no currently running import. Either the user did not request one, or
         * the last one completed.
         */
        int INACTIVE = 0;

        /** REQUESTED: the user requested the import by clicking the preference. */
        int REQUESTED = 1;

        /** IN_PROGRESS: the user selected a file and the import is being processed. */
        int IN_PROGRESS = 2;

        /** FINISHED: import has successfully finished or completed with errors. */
        int FINISHED = 3;
    }

    /** Describes at which state the password import flow is. */
    @ImportState private int mImportState;

    /** The key for saving {@link #mImportState} to instance bundle. */
    private static final String SAVED_STATE_IMPORT_STATE = "saved-state-import-state";

    /** Values of the histogram recording password import related events. */
    @IntDef({
        PasswordImportEvent.IMPORT_OPTION_SELECTED,
        PasswordImportEvent.IMPORT_DISMISSED,
        PasswordImportEvent.IMPORT_STARTED,
        PasswordImportEvent.IMPORT_COMPLETED,
        PasswordImportEvent.COUNT
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface PasswordImportEvent {
        int IMPORT_OPTION_SELECTED = 0;
        int IMPORT_DISMISSED = 1;
        int IMPORT_STARTED = 2;
        int IMPORT_COMPLETED = 3;
        int COUNT = 4;
    }

    /** Delegate interface to access the hosting fragment/activity */
    public interface Delegate {
        Activity getActivity();

        FragmentManager getFragmentManager();

        Profile getProfile();

        void runCreateFilePickerIntent(Intent intent);
    }

    private @Nullable Delegate mDelegate;

    /** Constructor */
    public ImportFlow() {
        mImportState = ImportState.INACTIVE;
    }

    /**
     * Sets up the import flow with the given delegate.
     *
     * @param savedInstanceState Bundle containing saved state
     * @param delegate The delegate to use for accessing activity/fragment functionality
     */
    public void onCreate(@Nullable Bundle savedInstanceState, Delegate delegate) {
        mDelegate = delegate;

        if (savedInstanceState == null) return;

        if (savedInstanceState.containsKey(SAVED_STATE_IMPORT_STATE)) {
            mImportState = savedInstanceState.getInt(SAVED_STATE_IMPORT_STATE);
        }
    }

    /** Starts the password import flow by showing a file picker. */
    public void startImporting() {
        if (mDelegate == null) return;

        mImportState = ImportState.REQUESTED;

        // Create file picker intent for CSV files
        Intent chooseFile = new Intent(Intent.ACTION_GET_CONTENT);
        chooseFile.setType("text/*");
        chooseFile.addCategory(Intent.CATEGORY_OPENABLE);

        // Add MIME types for CSV files
        String[] mimeTypes = {"text/csv", "text/comma-separated-values", "application/csv"};
        chooseFile.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);

        chooseFile =
                Intent.createChooser(
                        chooseFile,
                        mDelegate
                                .getActivity()
                                .getResources()
                                .getString(R.string.password_manager_ui_select_file));

        mDelegate.runCreateFilePickerIntent(chooseFile);
    }

    /**
     * Processes the selected CSV file and imports passwords.
     *
     * @param data The intent data from the file picker
     */
    public void processImportFile(Intent data) {
        if (data == null || data.getData() == null || mDelegate == null) {
            mImportState = ImportState.INACTIVE;
            return;
        }

        mImportState = ImportState.IN_PROGRESS;
        Uri fileUri = data.getData();

        // Read and process the CSV file in a background task
        new ImportPasswordsTask(fileUri).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    /**
     * Saves the import state to the instance bundle.
     *
     * @param outState Bundle to save state to
     */
    public void onSaveInstanceState(Bundle outState) {
        outState.putInt(SAVED_STATE_IMPORT_STATE, mImportState);
    }

    /**
     * Returns whether the import flow is currently active.
     *
     * @return true if import is in progress
     */
    public boolean isActive() {
        return mImportState == ImportState.IN_PROGRESS;
    }

    private static class CSVReadResult {
        public final @Nullable String csv;
        public final @Nullable Exception err;

        public CSVReadResult(@Nullable String csv, @Nullable Exception err) {
            this.csv = csv;
            this.err = err;
        }
    }

    /** AsyncTask to handle CSV file reading and password import in the background */
    private class ImportPasswordsTask extends AsyncTask<CSVReadResult> {
        private final Uri mFileUri;

        public ImportPasswordsTask(Uri fileUri) {
            mFileUri = fileUri;
        }

        @Override
        protected CSVReadResult doInBackground() {
            if (mFileUri == null || mDelegate == null) {
                return new CSVReadResult("", null);
            }

            try (InputStream inputStream =
                    mDelegate.getActivity().getContentResolver().openInputStream(mFileUri)) {
                if (inputStream == null) {
                    return new CSVReadResult(null, new FileNotFoundException(mFileUri.getPath()));
                }

                try (InputStreamReader iStream = new InputStreamReader(inputStream);
                        BufferedReader reader = new BufferedReader(iStream)) {
                    StringBuilder csvContent = new StringBuilder();
                    String line;

                    while ((line = reader.readLine()) != null) {
                        csvContent.append(line).append("\n");
                    }

                    return new CSVReadResult(csvContent.toString(), null);
                }
            } catch (Exception e) {
                return new CSVReadResult(null, e);
            }
        }

        @Override
        protected void onPostExecute(CSVReadResult csvResult) {
            if (mDelegate == null) {
                return;
            }
            if (csvResult.err == null && csvResult.csv != null && !csvResult.csv.trim().isEmpty()) {
                importPasswordsFromCsvContent(csvResult.csv);
            } else {
                showImportErrorDialog(
                        mDelegate
                                .getActivity()
                                .getResources()
                                .getString(
                                        R.string.password_settings_import_file_read_error,
                                        csvResult.err != null ? csvResult.err.getMessage() : ""));
            }
        }
    }

    // Tied to chromium/components/password_manager/core/browser/import/import_results.h
    private static enum ImportStatusResult {
        NONE(0, R.string.password_manager_ui_import_error_unknown),
        UNKNOWN_ERROR(1, R.string.password_manager_ui_import_error_unknown),
        SUCCESS(2, R.string.password_manager_ui_import_success_title),
        IO_ERROR(3, R.string.password_manager_ui_import_error_unknown),
        BAD_FORMAT(4, R.string.password_manager_ui_import_error_bad_format),
        DISMISSED(5, R.string.password_manager_ui_import_error_unknown),
        MAX_FILE_SIZE(6, R.string.password_manager_ui_import_file_size_exceeded),
        IMPORT_ALREADY_ACTIVE(7, R.string.password_manager_ui_import_already_active),
        NUM_PASSWORDS_EXCEEDED(8, R.plurals.password_manager_ui_import_error_limit_exceeded),
        CONFLICTS(9, R.string.password_manager_ui_import_conflict_device);

        private final int mValue;
        private final int mLocId;

        private ImportStatusResult(int val, int locId) {
            mValue = val;
            mLocId = locId;
        }

        public int value() {
            return mValue;
        }

        public int locStrId() {
            return mLocId;
        }

        public static ImportStatusResult fromInt(int id) {
            for (ImportStatusResult elem : ImportStatusResult.values()) {
                if (elem.value() == id) return elem;
            }
            return ImportStatusResult.NONE;
        }
    }

    /**
     * Processes the CSV content and imports passwords using the PasswordManagerHandler.
     *
     * @param csvContent The CSV content to process
     */
    private void importPasswordsFromCsvContent(String csvContent) {
        if (mDelegate == null) {
            return;
        }
        // Redeclared to avoid Nullable warning in capturing lambda.
        Delegate mDelegate = this.mDelegate;
        try {
            PasswordManagerHandler handler =
                    PasswordManagerHandlerProvider.getForProfile(mDelegate.getProfile())
                            .getPasswordManagerHandler();

            if (handler != null) {
                // Call the native import method (to be implemented)
                handler.importPasswordsFromCsv(
                        csvContent,
                        (count) -> {
                            // Success callback
                            mImportState = ImportState.FINISHED;
                            showImportSuccessDialog(count);
                        },
                        (errorId) -> {
                            // Error callback
                            ImportStatusResult result = ImportStatusResult.fromInt(errorId);
                            var res = mDelegate.getActivity().getResources();
                            mImportState = ImportState.FINISHED;
                            showImportErrorDialog(
                                    result == ImportStatusResult.NUM_PASSWORDS_EXCEEDED
                                            ? res.getQuantityString(
                                                    result.locStrId(),
                                                    handler.getMaxPasswordsPerCsvFile(),
                                                    handler.getMaxPasswordsPerCsvFile())
                                            : res.getString(result.locStrId()));
                        });
            } else {
                showImportErrorDialog(
                        mDelegate
                                .getActivity()
                                .getResources()
                                .getString(R.string.password_settings_manager_not_available));
            }
        } catch (Exception e) {
            // We can re-use the export localisation string since it doesn't mention exporting at
            // all.
            showImportErrorDialog(
                    mDelegate
                            .getActivity()
                            .getResources()
                            .getString(
                                    R.string.password_settings_export_error_details,
                                    e.getMessage()));
        }
    }

    /**
     * Shows a success dialog after successful import.
     *
     * @param count Number of passwords imported
     */
    private void showImportSuccessDialog(int count) {
        if (mDelegate == null) {
            return;
        }

        Resources res = mDelegate.getActivity().getResources();

        new AlertDialog.Builder(mDelegate.getActivity())
                .setTitle(res.getString(R.string.password_manager_ui_import_success_title))
                .setMessage(
                        res.getQuantityString(
                                R.plurals.password_settings_import_file_success_count,
                                count,
                                count))
                .setPositiveButton(
                        res.getString(R.string.ok),
                        (dialog, which) -> {
                            mImportState = ImportState.INACTIVE;
                            dialog.dismiss();
                        })
                .show();
    }

    /**
     * Shows an error dialog when import fails.
     *
     * @param errorMessage The error message to display. Caller is required to do any localisation.
     */
    private void showImportErrorDialog(String errorMessage) {
        if (mDelegate == null) {
            return;
        }

        Resources res = mDelegate.getActivity().getResources();

        new AlertDialog.Builder(mDelegate.getActivity())
                .setTitle(res.getString(R.string.password_settings_import_file_error_title))
                .setMessage(errorMessage)
                .setPositiveButton(
                        res.getString(R.string.ok),
                        (dialog, which) -> {
                            mImportState = ImportState.INACTIVE;
                            dialog.dismiss();
                        })
                .show();
    }
}

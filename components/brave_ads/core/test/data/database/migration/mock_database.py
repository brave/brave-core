# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This script is used to mock a SQLite database.

from datetime import datetime, timedelta
import argparse
import os
import secrets
import sqlite3
import string
import sys

DEFAULT_TEXT_COLUMN_LENGTH = 32
DEFAULT_LONGVARCHAR_COLUMN_LENGTH = 64

MILLISECONDS_IN_SECOND = 1000

# TODO(https://github.com/brave/brave-browser/issues/40017): Add foreign key
# support.


def generate_mock_string(length):
    characters = string.ascii_letters + string.digits
    random_string = ''.join(secrets.choice(characters) for _ in range(length))
    return random_string


def generate_mock_blob():
    return secrets.token_bytes(128)


def generate_mock_chrome_webkit_timestamp():
    start_date = datetime(1985, 10, 26)
    end_date = datetime(2015, 10, 21)  # If my calculations are correct...

    duration = end_date - start_date
    duration_in_seconds = duration.total_seconds()
    random_duration = secrets.randbelow(int(duration_in_seconds))

    random_date = start_date + timedelta(seconds=random_duration)

    chrome_webkit_epoch = datetime(
        1601, 1, 1)  # To be, or not to be, that is the question.
    chrome_webkit_delta = random_date - chrome_webkit_epoch
    chrome_webkit_delta_in_seconds = int(chrome_webkit_delta.total_seconds())

    return chrome_webkit_delta_in_seconds * MILLISECONDS_IN_SECOND


def generate_mock_column_with_random_test_data(connection, table_name,
                                               column_name, column_type):
    if column_type in ('INTEGER', 'INT', 'NUMERIC'):
        mock_column = secrets.randbelow(100)
    elif column_type == 'TEXT':
        mock_column = generate_mock_string(DEFAULT_TEXT_COLUMN_LENGTH)
    elif column_type == 'LONGVARCHAR':
        longvarchar_length = get_longvarchar_length(connection, table_name,
                                                    column_name)
        string_length = longvarchar_length[
            0] if longvarchar_length else DEFAULT_LONGVARCHAR_COLUMN_LENGTH
        mock_column = generate_mock_string(string_length)
    elif column_type in ('REAL', 'DOUBLE'):
        mock_column = round(secrets.SystemRandom().uniform(0.0, 1.0), 1)
    elif column_type == 'BLOB':
        mock_column = generate_mock_blob()
    elif column_type in ('DATE', 'TIMESTAMP'):
        mock_column = generate_mock_chrome_webkit_timestamp()
    else:
        sys.exit(f"ERROR: Unsupported column type {column_type}")

    return mock_column


def generate_mock_column_with_fixed_test_data(column_type):
    if column_type in ('INTEGER', 'INT', 'NUMERIC'):
        mock_column = 0
    elif column_type in ('TEXT', 'LONGVARCHAR'):
        mock_column = ""
    elif column_type in ('REAL', 'DOUBLE'):
        mock_column = 0.0
    elif column_type == 'BLOB':
        mock_column = b""
    elif column_type in ('DATE', 'TIMESTAMP'):
        mock_column = 0
    else:
        sys.exit(f"ERROR: Unsupported column type {column_type}")

    return mock_column


def generate_mock_columns(connection, table_name, should_mock_column_nulls,
                          should_use_random_test_data):
    auto_increment_column_names = get_auto_increment_column_names(
        connection, table_name)

    mock_columns = []

    table_info = get_table_info(connection, table_name)
    for column_name, column_type, column_notnull, column_pk in zip(
            column_names(table_info), column_types(table_info),
            column_notnulls(table_info), column_pks(table_info)):
        if column_name in auto_increment_column_names:
            # Do not mock auto-increment columns.
            continue

        mock_column = None

        if column_pk == 1:
            # Always mock the column if the column is a primary key.
            mock_column = (
                generate_mock_column_with_random_test_data(
                    connection, table_name, column_name, column_type)
                if is_column_unique(connection, table_name, column_name)
                or should_use_random_test_data else
                generate_mock_column_with_fixed_test_data(column_type))
        elif column_notnull == 1:
            # Always mock the column if the column has a NOT NULL constraint.
            mock_column = (
                generate_mock_column_with_random_test_data(
                    connection, table_name, column_name, column_type)
                if is_column_unique(connection, table_name, column_name)
                or should_use_random_test_data else
                generate_mock_column_with_fixed_test_data(column_type))
        else:
            # Only mock the column if we should mock NULL columns.
            if should_mock_column_nulls:
                mock_column = (
                    generate_mock_column_with_random_test_data(
                        connection, table_name, column_name, column_type)
                    if should_use_random_test_data else
                    generate_mock_column_with_fixed_test_data(column_type))

        mock_columns.append(mock_column)

    return mock_columns


def create_connection(database):
    connection = None

    try:
        connection = sqlite3.connect(database)
    except sqlite3.Error as e:
        print(f"ERROR: Failed to connect to database {e}")
        raise

    return connection


def close_connection(connection):
    if connection:
        connection.close()


def vacuum(connection):
    connection.execute("VACUUM")


def get_table_names(connection):
    connection_cursor = connection.cursor()

    connection_cursor.execute('''
        SELECT
          name
        FROM
          sqlite_master
        WHERE type = 'table'
          AND name NOT LIKE 'sqlite_%'
          AND name NOT LIKE 'meta';
        ''')
    return [row[0] for row in connection_cursor.fetchall()]


def get_table_info(connection, table_name):
    connection_cursor = connection.cursor()
    connection_cursor.execute(f"PRAGMA table_info({table_name});")
    return connection_cursor.fetchall()


# Extract column ids.
def column_cids(table_info):
    return [column[0] for column in table_info]


# Extract column names.
def column_names(table_info):
    return [column[1] for column in table_info]


# Extract column types.
def column_types(table_info):
    return [column[2] for column in table_info]


# Extract column NOT NULL constraints.
def column_notnulls(table_info):
    return [column[3] for column in table_info]


# Extract column DEFAULT values.
def column_dflt_values(table_info):
    return [column[4] for column in table_info]


# Extract column PRIMARY KEY constraints.
def column_pks(table_info):
    return [column[5] for column in table_info]


def is_column_unique(connection, table_name, column_name):
    connection_cursor = connection.cursor()

    connection_cursor.execute(f"PRAGMA index_list({table_name});")
    indexes = connection_cursor.fetchall()

    for index in indexes:
        index_name = index[1]
        is_unique = index[2]

        if is_unique:
            connection_cursor.execute(f"PRAGMA index_info({index_name});")
            index_info = connection_cursor.fetchall()

            for column in index_info:
                if column[2] == column_name:
                    return True

    return False


def get_auto_increment_column_names(connection, table_name):
    connection_cursor = connection.cursor()

    auto_increment_column_names = []

    table_info = get_table_info(connection, table_name)
    for column_name, column_pk in zip(column_names(table_info),
                                      column_pks(table_info)):
        if column_pk == 0:
            continue

        connection_cursor.execute(f"""
            SELECT
              sql
            FROM
              sqlite_master
            WHERE type='table'
              AND name='{table_name}';
            """)
        create_table_sql = connection_cursor.fetchone()[0]
        if 'AUTOINCREMENT' in create_table_sql:
            auto_increment_column_names.append(column_name)

    return auto_increment_column_names


def get_longvarchar_length(connection, table_name, column_name):
    connection_cursor = connection.cursor()
    connection_cursor.execute(
        f"SELECT LENGTH({column_name}) FROM {table_name};")
    return connection_cursor.fetchall()


def delete_table_rows(connection, table_name):
    connection_cursor = connection.cursor()

    try:
        connection_cursor.execute(f"DELETE FROM {table_name}")
    except Exception as e:
        print(f"ERROR: Failed to delete {table_name} table rows {e}")
        raise


def delete_table_auto_increment_counter(connection, table_name):
    connection_cursor = connection.cursor()

    try:
        connection_cursor.execute(
            f"DELETE FROM sqlite_sequence WHERE name='{table_name}'")
    except Exception:
        # The `sqlite_sequence` table will not exist if the table lacks an
        # auto-increment column.
        pass


def insert_mock_table_row(connection, table_name, columns):
    auto_increment_column_names = get_auto_increment_column_names(
        connection, table_name)

    table_info = get_table_info(connection, table_name)
    filtered_column_names = [
        column_name for column_name in column_names(table_info)
        if column_name not in auto_increment_column_names
    ]

    comma_separated_filtered_column_names = ', '.join(filtered_column_names)

    bind_columns = ", ".join(["?"] * len(columns))

    connection_cursor = connection.cursor()
    try:
        connection_cursor.execute(
            f'''
            INSERT INTO {table_name} (
              {comma_separated_filtered_column_names}
            ) VALUES ({bind_columns})
            ''', columns)
    except sqlite3.IntegrityError as e:
        print(f"ERROR: Failed to insert {columns} into {table_name}: {e}")
        raise


def insert_mock_table_rows(connection, table_name):
    # Generate and insert a table row with all columns populated, including
    # those that can be NULL with random test data.
    mock_columns = generate_mock_columns(connection,
                                         table_name,
                                         should_mock_column_nulls=True,
                                         should_use_random_test_data=True)
    insert_mock_table_row(connection, table_name, mock_columns)

    # Generate and insert a table row with all columns populated, including
    # those that can be NULL with fixed test data.
    mock_columns = generate_mock_columns(connection,
                                         table_name,
                                         should_mock_column_nulls=True,
                                         should_use_random_test_data=False)
    insert_mock_table_row(connection, table_name, mock_columns)

    # Generate and insert a table row with only non-NULL columns populated with
    # random test data.
    mock_columns = generate_mock_columns(connection,
                                         table_name,
                                         should_mock_column_nulls=False,
                                         should_use_random_test_data=True)
    insert_mock_table_row(connection, table_name, mock_columns)

    # Generate and insert a table row with only non-NULL columns populated with
    # fixed test data.
    mock_columns = generate_mock_columns(connection,
                                         table_name,
                                         should_mock_column_nulls=False,
                                         should_use_random_test_data=False)
    insert_mock_table_row(connection, table_name, mock_columns)


def mock_table(connection, table_name):
    print(f"Deleting {table_name} table rows")
    delete_table_rows(connection, table_name)

    print(f"Deleting {table_name} table auto-increment counter")
    delete_table_auto_increment_counter(connection, table_name)

    print(f"Mocking {table_name} table test data")
    insert_mock_table_rows(connection, table_name)


def mock_tables(connection):
    connection.execute("PRAGMA foreign_keys = OFF")

    table_names = get_table_names(connection)
    for table_name in table_names:
        mock_table(connection, table_name)

    connection.execute("PRAGMA foreign_keys = ON")

    connection.commit()


def mock_database(database):
    if not os.path.exists(database):
        sys.exit(f"ERROR: {database} does not exist")

    connection = create_connection(database)
    if not connection:
        sys.exit("ERROR: Failed to connect to database")

    print("Mocking database migration test data")

    mock_tables(connection)
    vacuum(connection)
    close_connection(connection)

    print("Mocked database migration test data")


def main():
    argument_parser = argparse.ArgumentParser(
        description="Mock database migration test data.")
    argument_parser.add_argument('database', help="The SQLite database")
    args = argument_parser.parse_args()

    mock_database(args.database)


if __name__ == "__main__":
    main()

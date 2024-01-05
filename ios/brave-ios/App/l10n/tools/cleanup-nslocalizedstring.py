#!/usr/bin/python
#
# Clean up NSLocalizedString usage excluding files within "blacklisted_parent_directories"
# Run ./l10n/cleanup-nslocalizedstring.py from project root folder

import os
import re
import sys
import json

blacklisted_parent_directories = [
    "ThirdParty", "Carthage", "fastlane", "L10nSnapshotTests", "l10n"
]
frameworks = ["BraveShared", "BraveWallet", "Data", "Shared", "Storage"]


def pascal_case(string):
    # Convert full stops, hyphens and underscores to spaces so that words are correctly pascal cased
    string = string.replace(".", " ")
    string = string.replace("-", " ")
    string = string.replace("_", " ")

    # Convert first letter of each word to uppercase
    string = re.sub(r'(^|\s)(\S)',
                    lambda match: match.group(1) + match.group(2).upper(),
                    string)

    # Strip punctuation
    string = re.sub(r'[^\w\s]', '', string)

    # Strip spaces
    string = string.replace(" ", "")

    return string


def replacement_string(key, table_name, value, comment, file):
    if key in keyDict:
        if key in duplicate:
            duplicate[key].append(file)
        else:
            duplicate[key] = [file, keyDict[key]]
    else:
        keyDict[key] = file
    content = 'NSLocalizedString("' + pascal_case(key) + '"'

    if table_name:
        content += ', tableName: "' + table_name + '"'

    content += ', value: "' + value + '"'

    content += ', comment: "' + comment + '")'

    return content


def parent_directory(path):
    norm_path = os.path.normpath(path)
    path_components = norm_path.split(os.sep)
    directory = path_components[0]
    return directory


def should_skip_path(path):
    directory = parent_directory(path)
    if directory in blacklisted_parent_directories:
        return True

    return False


keyDict = {}
duplicate = {}

for path, directories, files in os.walk("."):
    for file in files:
        if should_skip_path(path):
            continue

        if file.endswith(".swift"):
            table_name = ""

            directory = parent_directory(path)
            if directory in frameworks:
                table_name = directory
                print("Processing " + path + "/" + file + " [Framework: " +
                      directory + "]")
            else:
                print("Processing " + path + "/" + file)

            quoted_string_pattern = r'((?<![\\])[\'"])((?:.(?!(?<![\\])\1))*.?)\1'

            key_pattern = ' *' + quoted_string_pattern + ' *'
            table_name_pattern = ',[ \t]*(.?)[ \t]*tableName:[ \t]*(.?)[ \t]*' + quoted_string_pattern + '[ \t]*'
            value_pattern = ',[ \t]*(.?)[ \t]*value:[ \t]*(.?)[ \t]*' + quoted_string_pattern + '[ \t]*'
            comment_pattern = ',[ \t]*(.?)[ \t]*comment:[ \t]*(.?)[ \t]*' + quoted_string_pattern + '[ \t]*'

            flags = re.MULTILINE | re.DOTALL

            with open(os.path.join(path, file), 'r') as source:
                content = source.read()

                # Where K = key
                #       T = tableName
                #       V = value
                #       C = comment

                # KTVC
                pattern = 'NSLocalizedString\(' + key_pattern + table_name_pattern + value_pattern + comment_pattern + '\)'
                replacement_pattern = lambda match: replacement_string(
                    match.group(2), table_name, match.group(10), match.group(
                        14), file)
                content = re.sub(pattern,
                                 replacement_pattern,
                                 content,
                                 flags=flags)

                # KTV-
                pattern = 'NSLocalizedString\(' + key_pattern + table_name_pattern + value_pattern + '\)'
                replacement_pattern = lambda match: replacement_string(
                    match.group(2), table_name, match.group(6), "", file)
                content = re.sub(pattern,
                                 replacement_pattern,
                                 content,
                                 flags=flags)

                # KT-C
                pattern = 'NSLocalizedString\(' + key_pattern + table_name_pattern + comment_pattern + '\)'
                replacement_pattern = lambda match: replacement_string(
                    match.group(2), table_name, match.group(2), match.group(10),
                    file)
                content = re.sub(pattern,
                                 replacement_pattern,
                                 content,
                                 flags=flags)

                # KT--
                pattern = 'NSLocalizedString\(' + key_pattern + table_name_pattern + '\)'
                replacement_pattern = lambda match: replacement_string(
                    match.group(2), table_name, match.group(2), "", file)
                content = re.sub(pattern,
                                 replacement_pattern,
                                 content,
                                 flags=flags)

                # K-VC
                pattern = 'NSLocalizedString\(' + key_pattern + value_pattern + comment_pattern + '\)'
                replacement_pattern = lambda match: replacement_string(
                    match.group(2), table_name, match.group(6), match.group(10),
                    file)
                content = re.sub(pattern,
                                 replacement_pattern,
                                 content,
                                 flags=flags)

                # K-V-
                pattern = 'NSLocalizedString\(' + key_pattern + value_pattern + '\)'
                replacement_pattern = lambda match: replacement_string(
                    match.group(2), table_name, match.group(6), "", file)
                content = re.sub(pattern,
                                 replacement_pattern,
                                 content,
                                 flags=flags)

                # # K--C
                pattern = 'NSLocalizedString\(' + key_pattern + comment_pattern + '\)'
                replacement_pattern = lambda match: replacement_string(
                    match.group(2), table_name, match.group(2), match.group(6),
                    file)
                content = re.sub(pattern,
                                 replacement_pattern,
                                 content,
                                 flags=flags)

                # K---
                pattern = 'NSLocalizedString\(' + key_pattern + '\)'
                replacement_pattern = lambda match: replacement_string(
                    match.group(2), table_name, match.group(2), "", file)
                content = re.sub(pattern,
                                 replacement_pattern,
                                 content,
                                 flags=flags)

                with open(os.path.join(path, file), 'w') as source:
                    source.write(content)

os.system('sh ./l10n/LocalizedMatcher.sh')

array = []
with open('localizedStringLocations.txt', 'r') as f:
    content = f.read().splitlines()
    for line in content:
        array.append([x.strip() for x in line.split(':', 2)])

sys.stdout = open("localizedStringLocations.txt", "w")
print(json.dumps(array, indent=1))
sys.stdout.close()

duplicateList = []
for key, value in duplicate.iteritems():
    temp = [key, value]
    duplicateList.append(temp)
sys.stdout = open("Duplicates.txt", "w")
print(json.dumps(duplicateList, indent=1))
sys.stdout.close()

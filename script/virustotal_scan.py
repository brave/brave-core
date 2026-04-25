import os
import sys
import argparse
import hashlib
import logging
import json

from time import sleep

from virustotal3.core import Files as Virustotal

from requests.exceptions import ConnectionError


logging.basicConfig()
LOGGER = logging.getLogger(__name__)
LOGGER.setLevel(logging.INFO)

WAIT_TIME = 2  # seconds

RC_NOT_FOUND = 0
RC_OK = 1

RET_STR_CLEAN = 'CLEAN'
RET_STR_INFECTED = 'THREAT DETECTED'

SCAN_THREATS_FOUND = 255
SCAN_ERROR = 1
SCAN_FALSE_POSITIVES_FOUND = 0
SCAN_CLEAN = 0
SCAN_NOT_FOUND = -1


def sha256sum(filename):
    h = hashlib.sha256()
    b = bytearray(128*1024)
    mv = memoryview(b)
    with open(filename, 'rb', buffering=0) as f:
        for n in iter(lambda: f.readinto(mv), 0):
            h.update(mv[:n])
    return h.hexdigest()


def parse_response(response):
    if not response or 'data' not in response.keys():
        LOGGER.error(f"Error on VT request: {response}")
        return SCAN_ERROR

    # print(response)

    results = response['data']['attributes']['last_analysis_results']

    total = len(results)
    malicious = response['data']['attributes']['last_analysis_stats']['malicious']
    permalink = response['data']['links']['self'].replace(
        "api/v3/files", "gui/file")

    if malicious == 0:
        return SCAN_CLEAN
    elif malicious == 1:
        if results['Antiy-AVL']['result'] is not None or results['Zillya']['result'] is not None:
            LOGGER.warning(
                f"Known false-positive threats detected (Antiy-AVL or Zillya): {permalink}")
            return SCAN_FALSE_POSITIVES_FOUND
    elif malicious == 2:
        if results['Antiy-AVL']['result'] is not None and results['Zillya']['result'] is not None:
            LOGGER.warning(
                f"Known false-positive threats detected (Antiy-AVL and Zillya): {permalink}")
            return SCAN_FALSE_POSITIVES_FOUND

    LOGGER.warning(f"Threats detected: {permalink}")
    return SCAN_THREATS_FOUND


def check_report(vt, file_hash, abort_on_conn_err=True):
    errstr = None

    try:
        response = vt.info_file(file_hash)
    except ConnectionError as e:
        err_str = str(e)
        LOGGER.error(f"Connection error to VT: {err_str}.")
        if abort_on_conn_err:
            sys.exit(SCAN_ERROR)
        else:
            return SCAN_ERROR
    except Exception as e:
        errstr = str(e)

    if errstr:
        try:
            err = json.loads(errstr)
            if err["error"]["code"] == "NotFoundError":
                return SCAN_NOT_FOUND
        except Exception as e:
            _errstr = str(e)
            LOGGER.error(f"Unhandled error: {errstr}")
            sys.exit(SCAN_ERROR)

    return response


def main():
    parser = argparse.ArgumentParser(
        description="Scan a single file in VirusTotal and waits until report is complete")
    parser.add_argument('file', help='File to be scanned')

    args = parser.parse_args()

    if 'VT_API_KEY' not in os.environ:
        LOGGER.error('VT_API_KEY environment variable not set.')
        sys.exit(SCAN_ERROR)

    LOGGER.debug('Initialzing VirusTotal API')
    vt_api_key = os.environ['VT_API_KEY']

    vt = Virustotal(vt_api_key)

    # Hash file
    LOGGER.info('Checking if report already exists via file hash.')
    file_hash = sha256sum(args.file)

    response = check_report(vt, file_hash)

    if response == SCAN_NOT_FOUND:
        LOGGER.info(
            'Report not found. Sending file to VirusTotal for scanning.')
        vt.upload(args.file)

        while (response == SCAN_NOT_FOUND or response == SCAN_ERROR):
            LOGGER.info(
                f"Scan still running, sleeping for {WAIT_TIME} seconds.")
            sleep(WAIT_TIME)
            response = check_report(vt, file_hash, abort_on_conn_err=False)

    ret = parse_response(response)

    ret_str = RET_STR_INFECTED if ret else RET_STR_CLEAN
    LOGGER.info(f"Scan finished. Status: {ret_str}.")
    sys.exit(ret)


if __name__ == "__main__":
    main()

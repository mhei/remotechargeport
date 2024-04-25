#!/bin/sh
#
# This helper received the following arguments:
# - the filename to use for the upload
# - the target URL to upload to
# - the file names to store into the tarball
#   (relative paths, i.e. only the filenames and
#   assumed to be placed inside UPLOAD_DIR)
#

DEFAULTS_FILE="/etc/default/systemaggregatorftpd"
UPLOAD_DIR="/srv/diagnostics-incoming"
WORK_DIR="/srv/diagnostics-outgoing"
CONNECTION_TIMEOUT="20"

FN="$1"
URL="$2"
shift 2

[ -f "$DEFAULTS_FILE" ] && . "$DEFAULTS_FILE"

# just in case it does not exist yet
mkdir -p "$WORK_DIR"

# tar the given files into a single archive
if ! tar -czf "$WORK_DIR/$FN" -C "$UPLOAD_DIR" "$@"; then
    # error, so cleanup and report it
    cd "$UPLOAD_DIR"
    rm -f "$@"
    echo "UploadFailure"
    exit 1
fi

curl --progress-bar --connect-timeout "$CONNECTION_TIMEOUT" -T "$WORK_DIR/$FN" "$URL"
curl_exit_code=$?
if [[ $curl_exit_code -eq 0 ]]; then
    echo "Uploaded"
elif [[ $curl_exit_code -eq 67 ]] || [[ $curl_exit_code -eq 35 ]] || [[ $curl_exit_code -eq 69 ]] ||
    [[ $curl_exit_code -eq 9 ]]; then
    echo "PermissionDenied"
elif [[ $curl_exit_code -eq 3 ]] || [[ $curl_exit_code -eq 6 ]] || [[ $curl_exit_code -eq 10 ]] ||
    [[ $curl_exit_code -eq 87 ]]; then
    echo "BadMessage"
elif [[ $curl_exit_code -eq 1 ]]; then
    echo "NotSupportedOperation"
else
    echo "UploadFailure"
fi

# final cleanup: only cleanup the tarball,
# the incoming uploads might be required for a retry, so keep them
rm "$WORK_DIR/$FN"

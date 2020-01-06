#!/bin/sh

test_description="rauc-hawkbit-updater binary tests"


: "${SHARNESS_TEST_SRCDIR:=.}"
. "$SHARNESS_TEST_SRCDIR/sharness.sh"

SHARNESS_BUILD_DIRECTORY=$SHARNESS_TEST_DIRECTORY/../build

test_expect_success "rauc-hawkbit-updater version" "
  rauc-hawkbit-updater -v
"

test_expect_success "rauc-hawkbit-updater invalid argument" "
  test_expect_code 1 rauc-hawkbit-updater --invalidarg
"

test_done

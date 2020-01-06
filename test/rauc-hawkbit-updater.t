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

test_expect_success "rauc-hawkbit-updater no config file" "
  test_expect_code 2 rauc-hawkbit-updater
"

test_expect_success "rauc-hawkbit-updater non-existing config file given" "
  test_expect_code 3 rauc-hawkbit-updater -c does-not-exist.conf
"

test_done

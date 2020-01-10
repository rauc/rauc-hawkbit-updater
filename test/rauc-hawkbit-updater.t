#!/bin/sh

test_description="rauc-hawkbit-updater binary tests"


: "${SHARNESS_TEST_SRCDIR:=.}"
. "$SHARNESS_TEST_SRCDIR/sharness.sh"

SHARNESS_BUILD_DIRECTORY=$SHARNESS_TEST_DIRECTORY/../build

VALID_TOKEN=bhVahL1Il1shie2aj2poojeChee6ahShu
INVALID_TOKEN=ahVahL1Il1shie2aj2poojeChee6ahShu

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

test_expect_success "rauc-hawkbit-updater register and check (valid token)" "
  $SHARNESS_TEST_DIRECTORY/create_test_target && \
  cp $SHARNESS_TEST_DIRECTORY/test-config.conf . && \
  sed -i s/TEST_TOKEN/$VALID_TOKEN/g test-config.conf && \
  rauc-hawkbit-updater -c test-config.conf -r
"

test_expect_success "rauc-hawkbit-updater register and check (invalid token)" "
  $SHARNESS_TEST_DIRECTORY/create_test_target && \
  cp $SHARNESS_TEST_DIRECTORY/test-config.conf . && \
  sed -i s/TEST_TOKEN/$INVALID_TOKEN/g test-config.conf && \
  test_must_fail rauc-hawkbit-updater -c $SHARNESS_TEST_DIRECTORY/test-config.conf -r
"

test_done

#include "unity.h"

#include "assertion.h"
#include "models.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_status_assertion_should_pass_when_status_matches(void)
{
    Response resp = {0};
    resp.status_code = 200;

    Assertion as = {0};
    as.type = ASSERT_STATUS_EQ;
    as.expected.type = VALUE_INT;
    as.expected.as.int_value = 200;

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_TRUE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_STATUS_EQ, result.type);

    TEST_ASSERT_EQUAL(VALUE_INT, result.expected.type);
    TEST_ASSERT_EQUAL_INT(200, result.expected.as.int_value);

    TEST_ASSERT_EQUAL(VALUE_INT, result.actual.type);
    TEST_ASSERT_EQUAL_INT(200, result.actual.as.int_value);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_status_assertion_should_fail_when_status_does_not_match(void)
{
    Response resp = {0};
    resp.status_code = 404;

    Assertion as = {0};
    as.type = ASSERT_STATUS_EQ;
    as.expected.type = VALUE_INT;
    as.expected.as.int_value = 200;

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_STATUS_EQ, result.type);

    TEST_ASSERT_EQUAL(VALUE_INT, result.expected.type);
    TEST_ASSERT_EQUAL_INT(200, result.expected.as.int_value);

    TEST_ASSERT_EQUAL(VALUE_INT, result.actual.type);
    TEST_ASSERT_EQUAL_INT(404, result.actual.as.int_value);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_status_assertion_should_fail_when_expected_type_is_not_int(void)
{
    Response resp = {0};
    resp.status_code = 200;

    Assertion as = {0};
    as.type = ASSERT_STATUS_EQ;
    as.expected.type = VALUE_STRING;
    as.expected.as.string_value = "200";

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(ASSERT_STATUS_EQ, result.type);

    TEST_ASSERT_EQUAL(VALUE_STRING, result.expected.type);
    TEST_ASSERT_EQUAL_STRING("200", result.expected.as.string_value);

    /*
        Expected type yanlış olduğu için actual set edilmemiş olmalı.
        {0} initialize edildiği için VALUE_NONE bekliyoruz.
    */
    TEST_ASSERT_EQUAL(VALUE_NONE, result.actual.type);

    TEST_ASSERT_NOT_NULL(result.message);
}

void test_eval_assertion_should_fail_with_null_response(void)
{
    Assertion as = {0};
    as.type = ASSERT_STATUS_EQ;
    as.expected.type = VALUE_INT;
    as.expected.as.int_value = 200;

    AssertionResult result = eval_assertion(NULL, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_NOT_NULL(result.message);
}

void test_eval_assertion_should_fail_with_null_assertion(void)
{
    Response resp = {0};
    resp.status_code = 200;

    AssertionResult result = eval_assertion(&resp, NULL);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_NOT_NULL(result.message);
}

void test_unknown_assertion_type_should_fail(void)
{
    Response resp = {0};
    resp.status_code = 200;

    Assertion as = {0};
    as.type = (AssertionType)999;
    as.expected.type = VALUE_INT;
    as.expected.as.int_value = 200;

    AssertionResult result = eval_assertion(&resp, &as);

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL((AssertionType)999, result.type);
    TEST_ASSERT_NOT_NULL(result.message);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_status_assertion_should_pass_when_status_matches);
    RUN_TEST(test_status_assertion_should_fail_when_status_does_not_match);
    RUN_TEST(test_status_assertion_should_fail_when_expected_type_is_not_int);
    RUN_TEST(test_eval_assertion_should_fail_with_null_response);
    RUN_TEST(test_eval_assertion_should_fail_with_null_assertion);
    RUN_TEST(test_unknown_assertion_type_should_fail);

    return UNITY_END();
}
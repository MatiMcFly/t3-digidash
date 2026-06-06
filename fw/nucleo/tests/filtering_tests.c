#include <stdint.h>

#include "filtering.h"
#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_filtering_moving_average_basic_progression(void)
{
    int16_t  ringbuf[4] = {0};
    uint16_t index      = 0;

    TEST_ASSERT_EQUAL_INT16(2, filtering_moving_average(ringbuf, 4, &index, 8));
    TEST_ASSERT_EQUAL_UINT16(1, index);

    TEST_ASSERT_EQUAL_INT16(5, filtering_moving_average(ringbuf, 4, &index, 12));
    TEST_ASSERT_EQUAL_UINT16(2, index);

    TEST_ASSERT_EQUAL_INT16(9, filtering_moving_average(ringbuf, 4, &index, 16));
    TEST_ASSERT_EQUAL_UINT16(3, index);

    TEST_ASSERT_EQUAL_INT16(14, filtering_moving_average(ringbuf, 4, &index, 20));
    TEST_ASSERT_EQUAL_UINT16(0, index);
}

void test_filtering_moving_average_wraps_ring_buffer(void)
{
    int16_t  ringbuf[4] = {8, 12, 16, 20};
    uint16_t index      = 0;

    TEST_ASSERT_EQUAL_INT16(18, filtering_moving_average(ringbuf, 4, &index, 24));
    TEST_ASSERT_EQUAL_UINT16(1, index);
}

void test_filtering_moving_average_zero_size_returns_zero(void)
{
    int16_t  ringbuf[1] = {123};
    uint16_t index      = 0;

    TEST_ASSERT_EQUAL_INT16(0, filtering_moving_average(ringbuf, 0, &index, 99));
    TEST_ASSERT_EQUAL_UINT16(0, index);
    TEST_ASSERT_EQUAL_INT16(123, ringbuf[0]);
}

void test_filtering_moving_average_invalid_index_resets_to_zero(void)
{
    int16_t  ringbuf[3] = {10, 20, 30};
    uint16_t index      = 5;

    TEST_ASSERT_EQUAL_INT16(20, filtering_moving_average(ringbuf, 3, &index, 99));
    TEST_ASSERT_EQUAL_UINT16(0, index);
    TEST_ASSERT_EQUAL_INT16(10, ringbuf[0]);
    TEST_ASSERT_EQUAL_INT16(20, ringbuf[1]);
    TEST_ASSERT_EQUAL_INT16(30, ringbuf[2]);
}

void test_filtering_debounce_changes_only_after_consistent_samples(void)
{
    int16_t  ringbuf[2]     = {0};
    uint16_t index          = 0;
    int16_t  previous_value = 0;

    TEST_ASSERT_EQUAL_INT16(0, filtering_debounce(ringbuf, 2, &index, 1, &previous_value));
    TEST_ASSERT_EQUAL_INT16(0, previous_value);

    TEST_ASSERT_EQUAL_INT16(1, filtering_debounce(ringbuf, 2, &index, 1, &previous_value));
    TEST_ASSERT_EQUAL_INT16(1, previous_value);

    TEST_ASSERT_EQUAL_INT16(1, filtering_debounce(ringbuf, 2, &index, 0, &previous_value));
    TEST_ASSERT_EQUAL_INT16(1, previous_value);

    TEST_ASSERT_EQUAL_INT16(0, filtering_debounce(ringbuf, 2, &index, 0, &previous_value));
    TEST_ASSERT_EQUAL_INT16(0, previous_value);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_filtering_moving_average_basic_progression);
    RUN_TEST(test_filtering_moving_average_wraps_ring_buffer);
    RUN_TEST(test_filtering_moving_average_zero_size_returns_zero);
    RUN_TEST(test_filtering_moving_average_invalid_index_resets_to_zero);
    RUN_TEST(test_filtering_debounce_changes_only_after_consistent_samples);
    return UNITY_END();
}

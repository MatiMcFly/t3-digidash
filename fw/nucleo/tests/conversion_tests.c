#include "conversion.h"
#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_conversion_motor_rpm(void)
{
    TEST_ASSERT_EQUAL_INT16(0, conversion_motor_rpm(0));
    TEST_ASSERT_EQUAL_INT16(1000, conversion_motor_rpm(4000));
    TEST_ASSERT_EQUAL_INT16(16383, conversion_motor_rpm(65535));
}

void test_conversion_battery_voltage(void)
{
    TEST_ASSERT_EQUAL_INT16(0, conversion_battery_voltage(0));
    TEST_ASSERT_EQUAL_INT16(914, conversion_battery_voltage(32768));
    TEST_ASSERT_EQUAL_INT16(1200, conversion_battery_voltage(43000));
    TEST_ASSERT_TRUE(conversion_battery_voltage(40000) > conversion_battery_voltage(20000));
}

void test_conversion_fuel_level(void)
{
    TEST_ASSERT_EQUAL_INT16(808, conversion_fuel_level(0));
    TEST_ASSERT_EQUAL_INT16(700, conversion_fuel_level(7080));
    TEST_ASSERT_EQUAL_INT16(100, conversion_fuel_level(29067));
    TEST_ASSERT_EQUAL_INT16(-79, conversion_fuel_level(32768));
    TEST_ASSERT_TRUE(conversion_fuel_level(40000) < conversion_fuel_level(20000));
}

void test_conversion_coolant_temperature(void)
{
    TEST_ASSERT_EQUAL_INT16(-2731, conversion_coolant_temperature(0));
    TEST_ASSERT_EQUAL_INT16(1500, conversion_coolant_temperature(3438));
    TEST_ASSERT_EQUAL_INT16(800, conversion_coolant_temperature(14923));
    TEST_ASSERT_EQUAL_INT16(200, conversion_coolant_temperature(45889));
    TEST_ASSERT_EQUAL_INT16(0, conversion_coolant_temperature(55752));
    TEST_ASSERT_EQUAL_INT16(-100, conversion_coolant_temperature(59241));
    TEST_ASSERT_TRUE(conversion_coolant_temperature(50000) < conversion_coolant_temperature(30000));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_conversion_motor_rpm);
    RUN_TEST(test_conversion_battery_voltage);
    RUN_TEST(test_conversion_fuel_level);
    RUN_TEST(test_conversion_coolant_temperature);
    return UNITY_END();
}

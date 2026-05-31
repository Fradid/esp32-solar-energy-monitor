#include <unity.h>
#include "SOCEstimator.h"
#include "../../include/constants.h"

void setUp(void) {
    soc.begin(50.0f);
}
void tearDown(void) {}

void test_initial_SOC(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01, 50.0, soc.getSOC());
}

void test_charge_increases_SOC(void) {
    soc.begin(50.0f);
    // Charge at 1A for 60 seconds = 60 As
    for (int i = 0; i < 60; i++) {
        soc.update(1.0f, 0.0f, 3.7f, 1.0f);
    }
    // dQ = 60 As * 0.95 = 57 As, dSOC = 57/11520*100 = 0.4948%
    TEST_ASSERT_FLOAT_WITHIN(0.1, 50.4948, soc.getSOC());
}

void test_discharge_decreases_SOC(void) {
    soc.begin(50.0f);
    for (int i = 0; i < 60; i++) {
        soc.update(0.0f, 1.0f, 3.7f, 1.0f);
    }
    // dQ = -60 As * 1.0 = -60 As, dSOC = -0.5208%
    TEST_ASSERT_FLOAT_WITHIN(0.1, 49.479, soc.getSOC());
}

void test_SOC_clamped_at_100(void) {
    soc.setSOC(99.9f);
    for (int i = 0; i < 100; i++) {
        soc.update(2.0f, 0.0f, 4.2f, 1.0f);
    }
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(100.0f, soc.getSOC());
}

void test_SOC_clamped_at_0(void) {
    soc.setSOC(0.1f);
    for (int i = 0; i < 100; i++) {
        soc.update(0.0f, 2.0f, 3.0f, 1.0f);
    }
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0f, soc.getSOC());
}

void test_set_SOC(void) {
    soc.setSOC(75.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 75.0, soc.getSOC());

    soc.setSOC(150.0f);  // out of range
    TEST_ASSERT_FLOAT_WITHIN(0.01, 100.0, soc.getSOC());

    soc.setSOC(-10.0f);  // negative
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, soc.getSOC());
}

void test_zero_current_no_change(void) {
    soc.begin(60.0f);
    for (int i = 0; i < 100; i++) {
        soc.update(0.0f, 0.0f, 3.7f, 1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.01, 60.0, soc.getSOC());
}

void test_charge_efficiency(void) {
    soc.begin(0.0f);
    // Charge with 1A for 10000 sec = 10000 As
    // dSOC = 10000 * 0.95 / 11520 * 100 = 82.47%
    for (int i = 0; i < 10000; i++) {
        soc.update(1.0f, 0.0f, 3.7f, 1.0f);
    }
    TEST_ASSERT_FLOAT_WITHIN(1.0, 82.47, soc.getSOC());
}

void test_full_cycle(void) {
    // Discharge from 100 then recharge to 100
    soc.begin(100.0f);
    // Discharge ~100% (need 11520 As at I=1A -> 11520 s)
    for (int i = 0; i < 11600; i++) {
        soc.update(0.0f, 1.0f, 3.6f, 1.0f);
    }
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0, soc.getSOC());

    // Recharge with 1A
    for (int i = 0; i < 12200; i++) {
        soc.update(1.0f, 0.0f, 3.9f, 1.0f);
    }
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(99.0, soc.getSOC());
}

void test_charge_then_discharge_returns_close(void) {
    soc.begin(50.0f);
    // Charge for 100 sec at 1A: 100 As * 0.95 = 95 As
    for (int i = 0; i < 100; i++) soc.update(1.0f, 0.0f, 3.7f, 1.0f);
    float after_charge = soc.getSOC();

    // Discharge same charge back: 100 As * 1.0 = 100 As (more than charged in)
    for (int i = 0; i < 100; i++) soc.update(0.0f, 1.0f, 3.7f, 1.0f);

    // Result should be < initial because charge efficiency < 1
    TEST_ASSERT_LESS_THAN_FLOAT(50.0, soc.getSOC());
    TEST_ASSERT_GREATER_THAN_FLOAT(49.9, soc.getSOC());
}

void test_net_current_zero(void) {
    soc.begin(50.0f);
    // Equal charge and discharge → small net change (efficiency loss)
    for (int i = 0; i < 100; i++) soc.update(1.0f, 1.0f, 3.7f, 1.0f);
    // I_net = 0, but η != 1 when calculated direction depends on sign
    // Test that result is bounded
    TEST_ASSERT_FLOAT_WITHIN(1.0, 50.0, soc.getSOC());
}

void test_charged_Ah_accumulator(void) {
    soc.begin(50.0f);
    soc.resetAccumulator();
    // Charge 1A for 3600s = 1 Ah
    for (int i = 0; i < 3600; i++) {
        soc.update(1.0f, 0.0f, 3.7f, 1.0f);
    }
    // With η=0.95: 0.95 Ah accumulated
    TEST_ASSERT_FLOAT_WITHIN(0.05, 0.95, soc.getChargedAh());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_SOC);
    RUN_TEST(test_charge_increases_SOC);
    RUN_TEST(test_discharge_decreases_SOC);
    RUN_TEST(test_SOC_clamped_at_100);
    RUN_TEST(test_SOC_clamped_at_0);
    RUN_TEST(test_set_SOC);
    RUN_TEST(test_zero_current_no_change);
    RUN_TEST(test_charge_efficiency);
    RUN_TEST(test_full_cycle);
    RUN_TEST(test_charge_then_discharge_returns_close);
    RUN_TEST(test_net_current_zero);
    RUN_TEST(test_charged_Ah_accumulator);
    return UNITY_END();
}

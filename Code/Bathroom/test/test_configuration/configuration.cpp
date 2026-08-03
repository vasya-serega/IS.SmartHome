// PlatformIO + Unity test suite for Configuration
//
// This runs ON the Pico W itself (not "native"), because Configuration
// depends directly on the RP2040 LittleFS implementation and cannot be
// meaningfully mocked without changing the class to accept an injected
// filesystem/file abstraction.
//
// Run with:   pio test -e test
// (add a "test" environment in platformio.ini that extends your board env
//  and excludes src/main.cpp via build_src_filter)

#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include "Configuration.h"
#include "Constants.h"

Configuration config;

void setUp(void)
{
    // Start from a clean filesystem state before every test so each test
    // is independent of what earlier tests left behind.
    LittleFS.begin();
    if (LittleFS.exists(ConfigurationFile))
    {
        LittleFS.remove(ConfigurationFile);
    }
    config = Configuration();
}

void tearDown(void)
{
    if (LittleFS.exists(ConfigurationFile))
    {
        LittleFS.remove(ConfigurationFile);
    }
}

// --- getHumidityThreshold / default state -----------------------------

void test_default_humidity_threshold_before_init(void)
{
    TEST_ASSERT_EQUAL_UINT8(DefaultHumidityThreshold, config.getHumidityThreshold());
}

// --- init() -------------------------------------------------------------

void test_init_creates_config_file_when_missing(void)
{
    TEST_ASSERT_FALSE(LittleFS.exists(ConfigurationFile));
    config.init();
    TEST_ASSERT_TRUE(LittleFS.exists(ConfigurationFile));
}

void test_init_creates_config_file_with_default_value(void)
{
    // The freshly-created file should contain the default threshold as
    // its first byte, written via the "w+" open in the creation branch.
    config.init();

    File f = LittleFS.open(ConfigurationFile, "r");
    uint8_t stored = 0;
    f.readBytes(reinterpret_cast<char *>(&stored), sizeof(stored));
    f.close();

    TEST_ASSERT_EQUAL_UINT8(DefaultHumidityThreshold, stored);
}

void test_init_loads_default_threshold_on_first_run(void)
{
    config.init();
    TEST_ASSERT_EQUAL_UINT8(DefaultHumidityThreshold, config.getHumidityThreshold());
}

void test_init_reads_previously_stored_threshold(void)
{
    // Pre-seed a config file with a custom value, simulating a value
    // written during a previous session/boot. The file already exists
    // here, so init()'s "r+" open should succeed regardless of the
    // first-run creation behavior tested above.
    File f = LittleFS.open(ConfigurationFile, "w");
    const uint8_t seeded = 42;
    f.write(seeded);
    f.close();

    Configuration fresh;
    fresh.init();

    TEST_ASSERT_EQUAL_UINT8(seeded, fresh.getHumidityThreshold());
}

// --- setHumidityThreshold() ----------------------------------------------

void test_set_humidity_threshold_updates_in_memory_value(void)
{
    config.init();
    config.setHumidityThreshold(75);
    TEST_ASSERT_EQUAL_UINT8(75, config.getHumidityThreshold());
}

void test_set_humidity_threshold_ignores_identical_value(void)
{
    // The getter alone can't prove the early-return path ran, since a
    // real write of the same value would leave the getter looking
    // identical too. Instead, confirm the file itself was never
    // rewritten, using its last-write timestamp as the signal.
    File f = LittleFS.open(ConfigurationFile, "w");
    f.write(DefaultHumidityThreshold);
    f.close();

    config.init();

    File before = LittleFS.open(ConfigurationFile, "r");
    time_t beforeWrite = before.getLastWrite();
    before.close();

    delay(1100); // cross a full second boundary so a real write would produce a different timestamp

    config.setHumidityThreshold(DefaultHumidityThreshold); // identical value -> should be a no-op

    File after = LittleFS.open(ConfigurationFile, "r");
    time_t afterWrite = after.getLastWrite();
    after.close();

    TEST_ASSERT_EQUAL_UINT8(DefaultHumidityThreshold, config.getHumidityThreshold());
    TEST_ASSERT_EQUAL_INT(beforeWrite, afterWrite);
}

void test_set_humidity_threshold_different_value_updates_file_timestamp(void)
{
    // Sanity check for the timestamp technique used above: an actual
    // write SHOULD change the file's last-write time. Without this,
    // a passing result above could just mean getLastWrite() never
    // changes on this platform/filesystem config.
    File f = LittleFS.open(ConfigurationFile, "w");
    f.write(DefaultHumidityThreshold);
    f.close();

    config.init();

    File before = LittleFS.open(ConfigurationFile, "r");
    time_t beforeWrite = before.getLastWrite();
    before.close();

    delay(1100);

    config.setHumidityThreshold(DefaultHumidityThreshold + 1); // different value -> should write

    File after = LittleFS.open(ConfigurationFile, "r");
    time_t afterWrite = after.getLastWrite();
    after.close();

    TEST_ASSERT_TRUE(afterWrite > beforeWrite);
}

void test_set_humidity_threshold_persists_across_reinit(void)
{
    // Pre-seed the file so init() opens an *existing* file. This isolates
    // the persistence behavior of setHumidityThreshold() (seek(0) + write
    // + close + reopen) from the separate first-run creation path in
    // init() covered by test_init_creates_config_file_when_missing.
    File f = LittleFS.open(ConfigurationFile, "w");
    f.write(DefaultHumidityThreshold);
    f.close();

    config.init();
    config.setHumidityThreshold(88);

    // Simulate a reboot: a brand-new instance reads the same config file.
    Configuration reloaded;
    reloaded.init();

    TEST_ASSERT_EQUAL_UINT8(88, reloaded.getHumidityThreshold());
}

void test_set_humidity_threshold_out_of_range_high_value(void)
{
    File f = LittleFS.open(ConfigurationFile, "w");
    f.write(DefaultHumidityThreshold);
    f.close();

    config.init();
    config.setHumidityThreshold(255);
    TEST_ASSERT_EQUAL_UINT8(255, config.getHumidityThreshold());
}

void test_set_humidity_threshold_zero_value(void)
{
    File f = LittleFS.open(ConfigurationFile, "w");
    f.write(DefaultHumidityThreshold);
    f.close();

    config.init();
    config.setHumidityThreshold(0);
    TEST_ASSERT_EQUAL_UINT8(0, config.getHumidityThreshold());
}

void test_set_humidity_threshold_file_remains_open_after_write(void)
{
    // setHumidityThreshold() closes and reopens _file after writing.
    // Verify the object is left usable for a subsequent update in the
    // same session (no need to call init() again).
    File f = LittleFS.open(ConfigurationFile, "w");
    f.write(DefaultHumidityThreshold);
    f.close();

    config.init();
    config.setHumidityThreshold(50);
    config.setHumidityThreshold(60); // second update, same session

    Configuration reloaded;
    reloaded.init();

    TEST_ASSERT_EQUAL_UINT8(60, reloaded.getHumidityThreshold());
}

// --- serverIpString() / serverIp -----------------------------------------

void test_server_ip_string_format(void)
{
    String expected = "http://192.168.198.102";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), Configuration::serverIpString().c_str());
}

void test_server_ip_bytes(void)
{
    TEST_ASSERT_EQUAL_UINT8(192, Configuration::serverIp[0]);
    TEST_ASSERT_EQUAL_UINT8(168, Configuration::serverIp[1]);
    TEST_ASSERT_EQUAL_UINT8(198, Configuration::serverIp[2]);
    TEST_ASSERT_EQUAL_UINT8(102, Configuration::serverIp[3]);
}

// --- Unity runner (Arduino-style setup/loop) -----------------------------

void setup()
{
    delay(2000); // give the board/serial time to settle before Unity output starts

    UNITY_BEGIN();

    RUN_TEST(test_default_humidity_threshold_before_init);
    RUN_TEST(test_init_creates_config_file_when_missing);
    RUN_TEST(test_init_creates_config_file_with_default_value);
    RUN_TEST(test_init_loads_default_threshold_on_first_run);
    RUN_TEST(test_init_reads_previously_stored_threshold);
    RUN_TEST(test_set_humidity_threshold_updates_in_memory_value);
    RUN_TEST(test_set_humidity_threshold_ignores_identical_value);
    RUN_TEST(test_set_humidity_threshold_different_value_updates_file_timestamp);
    RUN_TEST(test_set_humidity_threshold_persists_across_reinit);
    RUN_TEST(test_set_humidity_threshold_out_of_range_high_value);
    RUN_TEST(test_set_humidity_threshold_zero_value);
    RUN_TEST(test_set_humidity_threshold_file_remains_open_after_write);
    RUN_TEST(test_server_ip_string_format);
    RUN_TEST(test_server_ip_bytes);

    UNITY_END();

    Serial.flush();
    delay(17000);
    rp2040.reboot();
}

void loop()
{
    // nothing to do here — all work happens in setup()
}
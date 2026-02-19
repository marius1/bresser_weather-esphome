#include "bresser_weather.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace bresser_weather
    {

        static const char *const TAG = "bresser_weather";

        void BresserWeatherComponent::setup()
        {
            ESP_LOGI(TAG, "Setting up Bresser Weather Sensor Receiver");
            this->ws_.begin();
            ESP_LOGI(TAG, "Receiver initialized successfully");
        }

        void BresserWeatherComponent::loop()
        {
            // Clear all sensor data
            this->ws_.clearSlots();

            // Try to receive radio message (non-blocking)
            int decode_status = this->ws_.getMessage();

            if (decode_status == DECODE_OK)
            {
                // Use first sensor slot
                const int i = 0;

                // Check if filter is enabled and if sensor ID matches
                if (this->filter_enabled_ && this->ws_.sensor[i].sensor_id != this->filter_sensor_id_)
                {
                    ESP_LOGD(TAG, "Ignoring sensor ID %08X (filter: %08X)",
                             (unsigned int)this->ws_.sensor[i].sensor_id,
                             (unsigned int)this->filter_sensor_id_);
                    return;
                }

                // Check if this is a weather sensor
                if ((this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER0) ||
                    (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER1) ||
                    (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER3) ||
                    (this->ws_.sensor[i].s_type == SENSOR_TYPE_WEATHER8))
                {

                    // Format sensor ID once for reuse
                    char id_buf[16];
                    snprintf(id_buf, sizeof(id_buf), "%08X", (unsigned int)this->ws_.sensor[i].sensor_id);

                    // Publish sensor ID
                    if (this->sensor_id_sensor_ != nullptr)
                    {
                        this->sensor_id_sensor_->publish_state(id_buf);
                    }

                    // Publish RSSI
                    if (this->rssi_sensor_ != nullptr)
                    {
                        this->rssi_sensor_->publish_state(this->ws_.sensor[i].rssi);
                    }

                    // Publish battery status
                    // Note: In Home Assistant, device_class BATTERY uses inverted logic:
                    // ON = Battery Low, OFF = Battery OK
                    if (this->battery_sensor_ != nullptr)
                    {
                        this->battery_sensor_->publish_state(!this->ws_.sensor[i].battery_ok);
                    }

                    // Publish temperature
                    if (this->ws_.sensor[i].w.temp_ok && this->temperature_sensor_ != nullptr)
                    {
                        this->temperature_sensor_->publish_state(this->ws_.sensor[i].w.temp_c);
                    }

                    // Publish humidity
                    if (this->ws_.sensor[i].w.humidity_ok && this->humidity_sensor_ != nullptr)
                    {
                        this->humidity_sensor_->publish_state(this->ws_.sensor[i].w.humidity);
                    }

                    // Publish wind data
                    if (this->ws_.sensor[i].w.wind_ok)
                    {
                        if (this->wind_gust_sensor_ != nullptr)
                        {
                            this->wind_gust_sensor_->publish_state(this->ws_.sensor[i].w.wind_gust_meter_sec);
                        }
                        if (this->wind_speed_sensor_ != nullptr)
                        {
                            this->wind_speed_sensor_->publish_state(this->ws_.sensor[i].w.wind_avg_meter_sec);
                        }
                        if (this->wind_direction_sensor_ != nullptr)
                        {
                            this->wind_direction_sensor_->publish_state(this->ws_.sensor[i].w.wind_direction_deg);
                        }
                    }

                    // Publish rain
                    if (this->ws_.sensor[i].w.rain_ok && this->rain_sensor_ != nullptr)
                    {
                        this->rain_sensor_->publish_state(this->ws_.sensor[i].w.rain_mm);
                    }

                    // Publish UV index (7-in-1 specific)
                    if (this->ws_.sensor[i].w.uv_ok && this->uv_sensor_ != nullptr)
                    {
                        this->uv_sensor_->publish_state(this->ws_.sensor[i].w.uv);
                    }

                    // Publish light (7-in-1 specific)
                    if (this->ws_.sensor[i].w.light_ok && this->light_sensor_ != nullptr)
                    {
                        this->light_sensor_->publish_state(this->ws_.sensor[i].w.light_klx);
                    }

                    // Build WeatherData struct and fire on_value callbacks
                    WeatherData data{};
                    data.sensor_id = id_buf;
                    data.rssi = this->ws_.sensor[i].rssi;
                    data.battery_ok = this->ws_.sensor[i].battery_ok;
                    data.temperature = this->ws_.sensor[i].w.temp_ok ? this->ws_.sensor[i].w.temp_c : NAN;
                    data.temperature_ok = this->ws_.sensor[i].w.temp_ok;
                    data.humidity = this->ws_.sensor[i].w.humidity_ok ? this->ws_.sensor[i].w.humidity : NAN;
                    data.humidity_ok = this->ws_.sensor[i].w.humidity_ok;
                    data.wind_gust = this->ws_.sensor[i].w.wind_ok ? this->ws_.sensor[i].w.wind_gust_meter_sec : NAN;
                    data.wind_speed = this->ws_.sensor[i].w.wind_ok ? this->ws_.sensor[i].w.wind_avg_meter_sec : NAN;
                    data.wind_direction = this->ws_.sensor[i].w.wind_ok ? this->ws_.sensor[i].w.wind_direction_deg : NAN;
                    data.wind_ok = this->ws_.sensor[i].w.wind_ok;
                    data.rain = this->ws_.sensor[i].w.rain_ok ? this->ws_.sensor[i].w.rain_mm : NAN;
                    data.rain_ok = this->ws_.sensor[i].w.rain_ok;
                    data.uv = this->ws_.sensor[i].w.uv_ok ? this->ws_.sensor[i].w.uv : NAN;
                    data.uv_ok = this->ws_.sensor[i].w.uv_ok;
                    data.light = this->ws_.sensor[i].w.light_ok ? this->ws_.sensor[i].w.light_klx : NAN;
                    data.light_ok = this->ws_.sensor[i].w.light_ok;
                    this->data_callback_.call(data);

                    ESP_LOGD(TAG, "Data published: Temp=%.1f°C, Hum=%d%%, Wind=%.1f/%.1f m/s @ %.0f°, Rain=%.1fmm, UV=%.1f, Light=%.1fklx, RSSI=%.1fdBm, Battery=%s",
                             this->ws_.sensor[i].w.temp_c,
                             this->ws_.sensor[i].w.humidity,
                             this->ws_.sensor[i].w.wind_avg_meter_sec,
                             this->ws_.sensor[i].w.wind_gust_meter_sec,
                             this->ws_.sensor[i].w.wind_direction_deg,
                             this->ws_.sensor[i].w.rain_mm,
                             this->ws_.sensor[i].w.uv,
                             this->ws_.sensor[i].w.light_klx,
                             this->ws_.sensor[i].rssi,
                             this->ws_.sensor[i].battery_ok ? "OK" : "Low");
                }
            }

            delay(100);
        }

    } // namespace bresser_weather
} // namespace esphome

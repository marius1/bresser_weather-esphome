#pragma once

#include <cmath>
#include <string>

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

namespace esphome
{
    namespace bresser_weather
    {

        struct WeatherData
        {
            std::string sensor_id;
            float rssi;
            bool battery_ok;
            float temperature;
            bool temperature_ok;
            float humidity;
            bool humidity_ok;
            float wind_gust;
            float wind_speed;
            float wind_direction;
            bool wind_ok;
            float rain;
            bool rain_ok;
            float uv;
            bool uv_ok;
            float light;
            bool light_ok;
        };

        class BresserWeatherComponent : public Component
        {
        public:
            void setup() override;
            void loop() override;
            float get_setup_priority() const override { return setup_priority::DATA; }

            void set_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
            void set_humidity_sensor(sensor::Sensor *sensor) { humidity_sensor_ = sensor; }
            void set_wind_gust_sensor(sensor::Sensor *sensor) { wind_gust_sensor_ = sensor; }
            void set_wind_speed_sensor(sensor::Sensor *sensor) { wind_speed_sensor_ = sensor; }
            void set_wind_direction_sensor(sensor::Sensor *sensor) { wind_direction_sensor_ = sensor; }
            void set_rain_sensor(sensor::Sensor *sensor) { rain_sensor_ = sensor; }
            void set_uv_sensor(sensor::Sensor *sensor) { uv_sensor_ = sensor; }
            void set_light_sensor(sensor::Sensor *sensor) { light_sensor_ = sensor; }
            void set_rssi_sensor(sensor::Sensor *sensor) { rssi_sensor_ = sensor; }
            void set_battery_sensor(binary_sensor::BinarySensor *sensor) { battery_sensor_ = sensor; }
            void set_sensor_id_text_sensor(text_sensor::TextSensor *sensor) { sensor_id_sensor_ = sensor; }
            void set_filter_sensor_id(uint32_t filter_id)
            {
                filter_sensor_id_ = filter_id;
                filter_enabled_ = true;
            }

            void add_on_value_callback(std::function<void(const WeatherData &)> callback)
            {
                this->data_callback_.add(std::move(callback));
            }

        protected:
            WeatherSensor ws_;
            uint32_t filter_sensor_id_{0};
            bool filter_enabled_{false};

            sensor::Sensor *temperature_sensor_{nullptr};
            sensor::Sensor *humidity_sensor_{nullptr};
            sensor::Sensor *wind_gust_sensor_{nullptr};
            sensor::Sensor *wind_speed_sensor_{nullptr};
            sensor::Sensor *wind_direction_sensor_{nullptr};
            sensor::Sensor *rain_sensor_{nullptr};
            sensor::Sensor *uv_sensor_{nullptr};
            sensor::Sensor *light_sensor_{nullptr};
            sensor::Sensor *rssi_sensor_{nullptr};
            binary_sensor::BinarySensor *battery_sensor_{nullptr};
            text_sensor::TextSensor *sensor_id_sensor_{nullptr};

            CallbackManager<void(const WeatherData &)> data_callback_;
        };

        class WeatherDataTrigger : public Trigger<WeatherData>
        {
        public:
            explicit WeatherDataTrigger(BresserWeatherComponent *parent)
            {
                parent->add_on_value_callback([this](const WeatherData &data)
                                              { this->trigger(data); });
            }
        };

    } // namespace bresser_weather
} // namespace esphome

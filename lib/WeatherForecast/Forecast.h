#pragma once

#include <nlohmann/json.hpp>

#include <vector>
#include <string>
#include <map>
#include <iostream>

class Forecast final {
private:
    int forecastDays_;
    std::vector<std::string> cities_;
    std::vector<std::pair<double, double>> city_coords_;
    double updateRate_;
    const std::string api_token_ = "Z9up6WD6qdQs2IDatlsYsw==1VUYYKvgIBSGLPiW";
    std::string text_;

    void readConfig();
    void getCoords();
    nlohmann::json getOneCityForecast(double latitude, double longitude);
    std::map<std::string, double> parseCurrentWeather(const nlohmann::json& weather_data);
    std::vector<std::map<std::string, double>> parseLongDayWeather(const nlohmann::json& weather_data, int day);
    std::map<std::string, double> parseShortDayWeather(const nlohmann::json& weather_data, int day);
public:
    void start();
};

std::string getWindDirection(double numWindDirection);

int kbhit();


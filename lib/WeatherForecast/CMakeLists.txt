add_library(WeatherForecast Forecast.cpp Forecast.h)
target_link_libraries(WeatherForecast PRIVATE nlohmann_json::nlohmann_json cpr::cpr pdcurses)
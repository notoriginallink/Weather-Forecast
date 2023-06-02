#include "Forecast.h"

#include <cpr/cpr.h>
#include <curses.h>

#include <cstdlib>
#include <fstream>
#include <chrono>

std::map<int, std::string> weathercode = {
        {0, "Clear sky"},
        {1, "Mainly clear"},
        {2, "Partly cloudy"},
        {3, "Overcast"},
        {45, "Foggy"},
        {48, "Deposing rime Fog"},
        {51, "Light drizzle"},
        {53, "moderate drizzle"},
        {55, "Dense drizzle"},
        {56, "Light freez. drizzle"},
        {57, "Dense freez. drizzle"},
        {61, "Slight rain"},
        {63, "Moderate rain"},
        {65, "Heavy rain"},
        {66, "Light freezing rain"},
        {67, "Heavy freezing rain"},
        {71, "Slight snow fall"},
        {73, "Moderate snowfall"},
        {75, "Heavy snowfall"},
        {77, "Snow grains"},
        {80, "Slight rain showers"},
        {81, "Mod. rain showers"},
        {82, "Heavy rain showers"},
        {85, "Slight snow showers"},
        {86, "Heavy snow showers"},
        {95, "Slight thunderstorm"}
};

std::vector<std::string> months{"January", "February", "March", "April",
                                "May", "June", "July", "August", "September",
                                "October", "November", "December"};

std::map<int, std::vector<std::string>> pictures = {
        {0, {"     \\   /     ", "      .-.      ", "  -- (   ) --  ", "      `-'      ", "     /   \\     "}},
        {1, {"     \\   /     ", "      .-.      ", "  -- (   ) --  ", "      `-'      ", "     /   \\     "}},
        {2, {"    \\  /       ", "  _ /\"\".-.     ", "    \\_(   ).   ", "    /(___(__)  ", "               "}},
        {3, {"               ", "      .--.     ", "   .-(    ).   ", "  (___.__)__)  ", "               "}},
        {45, {"               ", "  _ - _ - _ -  ", "   _ - _ - _   ", "  _ - _ - _ -  ", "               "}},
        {61, {"  _`/\"\".-.     ", "   ,\\_(   ).   ", "    /(___(__)  ", "      ' ' ' '  ", "     ' ' ' '   "}},
        {63, {"  _`/\"\".-.     ", "   ,\\_(   ).   ", "    /(___(__)  ", "      ' ' ' '  ", "     ' ' ' '   "}},
        {65, {"  _`/\"\".-.     ", "   ,\\_(   ).   ", "    /(___(__)  ", "      ' ' ' '  ", "     ' ' ' '   "}},
        {71, {"  _`/\"\".-.     ", "   ,\\_(   ).   ", "    /(___(__)  ", "      *  *  *  ", "     *  *  *   "}},
        {95, {"  _`/\"\".-.     ", "   ,\\_(   ).   ", "    /(___(__)  ", "     ''''''    ", "     ' ' ' '   "}}
};

// consts for console keys
const int kEscapeCode = 27;
const int kNextCode = 110;
const int kPreviousCode = 112;
const int kPlusCode = 465;
const int kMinusCode = 464;
const int kMul = 463;
const int kEnter = 459;
// const for console drawing
const int kLargeBoardHeight = 11;
const int kSmallBoardHeight = 9;
const int kSpaceCode = 32;
// const for data processing
const double kPressure = 0.75006;
const double kVisibility = 1000;
// const for requests
const int kErrorCode = 200;
const int kRequestAttemps = 5;
//
const int kMaxDaysForLargeForecast = 3;
const int kMaxForecastDays = 12;


std::string getWindDirection(double numWindDirection) {
    if (numWindDirection < 25) return "S";
    else if (numWindDirection < 65) return "SW";
    else if (numWindDirection < 115) return "W";
    else if (numWindDirection < 155) return "WN";
    else if (numWindDirection < 205) return "N";
    else if (numWindDirection < 245) return "NE";
    else if (numWindDirection < 295) return "E";
    else if (numWindDirection < 335) return "SE";
    else return "S";
}

int kbhit() {
    int ch = getch();
    if (ch != ERR) {
        ungetch(ch);
        return 1;
    }
    return 0;
}

void Forecast::readConfig() {
    std::ifstream file("../config.json");
    if (!file.is_open())
        throw std::runtime_error{"Config.json was not opened"};
    nlohmann::json config = nlohmann::json::parse(file);
    forecastDays_ = config["ForecastDays"];
    updateRate_ = config["UpdateRate_minutes"];
    cities_ = std::vector<std::string>(config["Cities"]);
    std::cout << "CONFIG\n";
}

void Forecast::getCoords() {
    if (cities_.empty())
        throw std::length_error("List of cities can't be empty");

    cpr::Url url("https://api.api-ninjas.com/v1/city");
    cpr::Header header{{"X-Api-Key", api_token_}};

    for (std::string city : cities_) {
        cpr::Response r = cpr::Get(url, header, cpr::Parameters{{"name", city}});
        for (int i = 0; i < kRequestAttemps && r.status_code != kErrorCode; ++i) {
            r = cpr::Get(url, header, cpr::Parameters{{"name", city}});
        }
        if (r.status_code != kErrorCode)
            throw std::runtime_error("Server does not respond");

        nlohmann::json city_data = nlohmann::json::parse(r.text);
        if (city_data[0].empty())
            throw std::runtime_error{"Cant find this city's location, may be its written incorrect: " + city};
        else
            city_coords_.emplace_back(city_data[0]["latitude"], city_data[0]["longitude"]);
    }
    std::cout << "COORDS\n";
}

nlohmann::json Forecast::getOneCityForecast(double latitude, double longitude) {
    cpr::Url url{"https://api.open-meteo.com/v1/forecast"};
    cpr::Parameters parameters{
            {"latitude", std::to_string(latitude)},
            {"longitude", std::to_string(longitude)},
            {"current_weather", "true"},
            {"timezone", "auto"},
            {"hourly", "pressure_msl"},
            {"hourly", "visibility"},
            {"hourly", "temperature_2m"},
            {"hourly", "weathercode"},
            {"hourly", "windspeed_10m"},
            {"hourly", "winddirection_10m"},
            {"daily", "temperature_2m_min"},
            {"daily", "temperature_2m_max"},
            {"daily", "weathercode"},
            {"daily", "windspeed_10m_max"},
            {"daily", "winddirection_10m_dominant"},
            {"forecast_days", std::to_string(forecastDays_)}
    };

    cpr::Response r = cpr::Get(url, parameters);
    for (int i = 0; i < kRequestAttemps, r.status_code != kErrorCode; ++i)
        r = cpr::Get(url, parameters);
    if (r.status_code != kErrorCode)
        throw std::runtime_error{"Server does not respond"};
    nlohmann::json data = nlohmann::json::parse(r.text);

    return data;
}

std::map<std::string, double> Forecast::parseCurrentWeather(const nlohmann::json& weather_data) {
    std::map<std::string, double> data;
    data["weathercode"] = weather_data["current_weather"]["weathercode"];
    data["temperature"] = weather_data["current_weather"]["temperature"];
    data["wind_dir"] = weather_data["current_weather"]["winddirection"];
    data["windspeed"] = weather_data["current_weather"]["windspeed"];

    for (int i = 0; i < weather_data["hourly"]["time"].size(); ++i) {
        if (weather_data["hourly"]["time"][i] == weather_data["current_weather"]["time"]) {
            double visibility = weather_data["hourly"]["visibility"][i];
            double pressure = weather_data["hourly"]["pressure_msl"][i];
            data["visibility"] = visibility / kVisibility;
            data["pressure"] = pressure * kPressure;
            break;
        }
    }

    return data;
}

std::vector<std::map<std::string, double>> Forecast::parseLongDayWeather(const nlohmann::json& weather_data, int day) {
    std::vector<std::map<std::string, double>> data(4);

    for (int i = 0; i < 4; ++i) {
        int start_of_day = 24 * day;
        data[i]["temperature_max"] = -100;
        data[i]["temperature_min"] = 100;
        data[i]["windspeed_av"] = 0;
        data[i]["winddirection_av"] = 0;
        data[i]["visibility_av"] = 0;
        data[i]["pressure_av"] = 0;
        std::string date = weather_data["daily"]["time"][day];
        data[i]["day"] = std::stod(date.substr(8, 2));
        data[i]["month"] = std::stod(date.substr(5, 2));

        for (int k = start_of_day + 6 * i, j = 0; j < 6; ++k, ++j) {
            data[i]["windspeed_av"] += static_cast<double>(weather_data["hourly"]["windspeed_10m"][k]);
            data[i]["winddirection_av"] += static_cast<double>(weather_data["hourly"]["winddirection_10m"][k]);
            data[i]["visibility_av"] += static_cast<double>(weather_data["hourly"]["visibility"][k]);
            data[i]["pressure_av"] += static_cast<double>(weather_data["hourly"]["pressure_msl"][k]);
            data[i]["temperature_max"] = std::max(data[i]["temperature_max"], static_cast<double>(weather_data["hourly"]["temperature_2m"][k]));
            data[i]["temperature_min"] = std::min(data[i]["temperature_min"], static_cast<double>(weather_data["hourly"]["temperature_2m"][k]));
        }
        data[i]["weathercode"] = static_cast<double>(weather_data["hourly"]["weathercode"][start_of_day + 6 * i + 3]);
        data[i]["windspeed_av"] /= 6;
        data[i]["winddirection_av"] /= 6;
        data[i]["visibility_av"] = (data[i]["visibility_av"] / kVisibility) / 6;
        data[i]["pressure_av"] = (data[i]["pressure_av"] * kPressure) / 6;
    }

    return data;
}

std::map<std::string, double> Forecast::parseShortDayWeather(const nlohmann::json& weather_data, int day) {
    std::map<std::string, double> data;
    data["weathercode"] = weather_data["daily"]["weathercode"][day];
    data["temperature_max"] = weather_data["daily"]["temperature_2m_max"][day];
    data["temperature_min"] = weather_data["daily"]["temperature_2m_min"][day];
    data["windspeed"] = weather_data["daily"]["windspeed_10m_max"][day];
    data["winddirection"] = weather_data["daily"]["winddirection_10m_dominant"][day];
    data["visibility_av"] = 0;
    data["pressure_av"] = 0;
    int start_of_day = 24 * day;
    std::string date = weather_data["daily"]["time"][day];
    data["day"] = std::stod(date.substr(8, 2));
    data["month"] = std::stod(date.substr(5, 2));
    for (int i = start_of_day; i < start_of_day + 24; ++i) {
        data["visibility_av"] += static_cast<double>(weather_data["hourly"]["visibility"][i]);
        data["pressure_av"] += static_cast<double>(weather_data["hourly"]["pressure_msl"][i]);
    }
    data["visibility_av"] = (data["visibility_av"] / kVisibility) / 24;
    data["pressure_av"] = (data["pressure_av"] * kPressure) / 24;

    return data;
}



void Forecast::start() {
    readConfig();
    getCoords();

    int index = 0;
    bool needUpdate = true;

    system("mode 800 & mode con: lines=50 cols=160");
    initscr();
    cbreak();
    keypad(stdscr, true);
    nodelay(stdscr, true);
    noecho();

    int height, width;
    getmaxyx(stdscr, height, width);
    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        if (kbhit()){
            int key = getch();
            if (key == kMul) {
                clear();
                echo();
                printw(text_.c_str());
                while (key != KEY_ENTER) {
                    if (kbhit()) {
                        key = getch();
                        if (key == kEnter)
                            break;
                        else if (key == KEY_BACKSPACE) {
                            text_.erase(text_.end() - 1);
                            clear();

                            printw(text_.c_str());
                            refresh();
                        }
                        else {
                            text_ += static_cast<char>(key);
                        }
                    }
                }
                needUpdate = true;
                noecho();
            }
            if (key == kEscapeCode)
                break;
            else if (key == kPlusCode) {
                if (forecastDays_ != kMaxForecastDays) {
                    forecastDays_++;
                    needUpdate = true;
                }
            } else if (key == kMinusCode) {
                if (forecastDays_ != 1) {
                    forecastDays_--;
                    needUpdate = true;
                }
            } else if (key == kNextCode) {
                index = (index + 1) % cities_.size();
                needUpdate = true;
            } else if (key == kPreviousCode) {
                index = (index - 1 + cities_.size()) % cities_.size();
                needUpdate = true;
            }
        }

        auto time = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::minutes>(time - start_time).count() >= updateRate_) {
            start_time = std::chrono::steady_clock::now();
            needUpdate = true;
        }

        if (needUpdate) {
            needUpdate = false;
            nlohmann::json weather_data = getOneCityForecast(city_coords_[index].first, city_coords_[index].second);
            std::map<std::string, double> current_data = parseCurrentWeather(weather_data);

            std::string city_name = cities_[index];
            clear();
//            start_color();
//            init_pair(1, COLOR_RED, COLOR_YELLOW);
//            attron(COLOR_PAIR(1));
            printw("Weather forecast for %s", city_name.c_str());
//            attroff(COLOR_PAIR(1));


            WINDOW* current_weather_win_picture = subwin(stdscr, 5, width / 8, 2, 0);
            WINDOW* current_weather_win_data = subwin(stdscr, 5, width / 8, 2, width / 8);

            int wcode = round(current_data["weathercode"]);
            if (pictures.find(wcode) == pictures.end())
                wcode = 0;
            for (int i = 0; i < 5; ++i)
                mvwprintw(current_weather_win_picture, i, 0, pictures[wcode][i].c_str());
            mvwprintw(current_weather_win_data, 0, 0, weathercode[wcode].c_str());
            mvwprintw(current_weather_win_data, 1, 0, "%0.1f \370C", current_data["temperature"]);
            mvwprintw(current_weather_win_data, 2, 0, "%s %0.1f km/h",getWindDirection(current_data["wind_dir"]).c_str(), current_data["windspeed"]);
            mvwprintw(current_weather_win_data, 3, 0, "%0.1f km", current_data["visibility"]);
            mvwprintw(current_weather_win_data, 4, 0, "%0.1f mm Hg", current_data["pressure"]);

            const int kStartLine = 8;

            if (forecastDays_ <= kMaxDaysForLargeForecast) {

                for (int i = 0; i < forecastDays_; ++i) {
                    int current_start_line = kStartLine + (kLargeBoardHeight + 1) * i;
                    std::vector<std::map<std::string, double>> long_forecast_data = parseLongDayWeather(weather_data, i);

                    WINDOW* date_win = subwin(stdscr, kLargeBoardHeight, width, current_start_line, 0);
                    box(date_win, 0, 0);
                    mvwprintw(date_win, 1, width / 2 - 6, "%dth of %s", static_cast<int>(long_forecast_data[0]["day"]), months[static_cast<int>(long_forecast_data[0]["month"]) - 1].c_str());

                    int current_column = 0;
                    for (auto s : {"Morning", "Noon", "Evening", "Night"}) {
                        WINDOW* daytime_name_win = subwin(stdscr, 3, width / 4, current_start_line + 2, current_column);
                        current_column += width / 4;

                        if (current_column == width / 4)
                            wborder(daytime_name_win, 0, 0, 0, kSpaceCode, ACS_SSSB, ACS_TTEE, kSpaceCode, kSpaceCode);
                        else if (current_column == width)
                            wborder(daytime_name_win, 0, 0, 0, kSpaceCode, ACS_TTEE, ACS_SBSS, kSpaceCode, kSpaceCode);
                        else
                            wborder(daytime_name_win, 0, 0, 0, kSpaceCode, ACS_TTEE, ACS_TTEE, kSpaceCode, kSpaceCode);

                        mvwprintw(daytime_name_win, 1, width / 8 - 3, s);
                    }

                    // draw and fill cells for data for every daytime
                    current_column = 0;
                    for (int j = 0; j < 4; ++j) {
                        WINDOW* daytime_data_win = subwin(stdscr, 7, width / 4, current_start_line + 4, current_column);
                        current_column += width / 4;
                        wborder(daytime_data_win, 0, 0, 0, 0, ACS_SSSB, ACS_SBSS, 0, 0);
                        int wcode = round(long_forecast_data[j]["weathercode"]);
                        if (pictures.find(wcode) == pictures.end())
                            wcode = 0;
                        for (int k = 0; k < 5; ++k)
                            mvwprintw(daytime_data_win, k + 1, 1, pictures[wcode][k].c_str());

                        mvwprintw(daytime_data_win, 1, width / 8, weathercode[wcode].c_str());
                        mvwprintw(daytime_data_win, 2, width / 8, "%0.1f(%0.1f) \370C", long_forecast_data[j]["temperature_max"], long_forecast_data[j]["temperature_min"]);
                        mvwprintw(daytime_data_win, 3, width / 8, "%s %0.1f km/h", getWindDirection(long_forecast_data[j]["winddirection_av"]).c_str(), long_forecast_data[j]["windspeed_av"]);
                        mvwprintw(daytime_data_win, 4, width / 8, "%0.1f km", long_forecast_data[j]["visibility_av"]);
                        mvwprintw(daytime_data_win, 5, width / 8, "%0.1f mm Hg", long_forecast_data[j]["pressure_av"]);
                    }
                }
            } else {
                int current_column = 0;
                for (int i = 0, i_ = 0; i < forecastDays_; i += 4, ++i_) {
                    int current_start_line = kStartLine + (kSmallBoardHeight + 1) * i_;
                    int daysToDraw = std::min(forecastDays_ - i, 4);
                    for (int j = 0; j < daysToDraw; ++j) {
                        std::map<std::string, double> short_forecast_data = parseShortDayWeather(weather_data, i + j);
                        WINDOW* date_win = subwin(stdscr, 3, width / 4, current_start_line, current_column);
                        box(date_win, 0, 0);
                        mvwprintw(date_win, 1, width / 8 - 2, "%dth of %s", static_cast<int>(short_forecast_data["day"]), months[static_cast<int>(short_forecast_data["month"]) - 1].c_str());

                        WINDOW* day_data_win = subwin(stdscr, kSmallBoardHeight - 2, width / 4, current_start_line + 2, current_column);
                        wborder(day_data_win, 0, 0, 0, 0, ACS_SSSB, ACS_SBSS, 0 , 0);
                        int wcode = round(short_forecast_data["weathercode"]);
                        if (pictures.find(wcode) == pictures.end())
                                wcode = 0;
                        for (int k = 0; k < 5; ++k)
                            mvwprintw(day_data_win, k + 1, 1, pictures[wcode][k].c_str());
                        mvwprintw(day_data_win, 1, width / 8, weathercode[wcode].c_str());
                        mvwprintw(day_data_win, 2, width / 8, "%0.1f(%0.1f) \370C", short_forecast_data["temperature_max"], short_forecast_data["temperature_min"]);
                        mvwprintw(day_data_win, 3, width / 8, "%s %0.1f km/h", getWindDirection(short_forecast_data["winddirection"]).c_str(), short_forecast_data["windspeed"]);
                        mvwprintw(day_data_win, 4, width / 8, "%0.1f km", short_forecast_data["visibility_av"]);
                        mvwprintw(day_data_win, 5, width / 8, "%0.1f mm Hg", short_forecast_data["pressure_av"]);

                        current_column += width / 4;
                        if (current_column == width)
                            current_column = 0;
                    }
                }
            }
        }
        refresh();
    }

    endwin();
}
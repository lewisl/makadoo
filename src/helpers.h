//
// helpers
//

#pragma once

#include <ranges>
#include <numeric>

#include <absl/time/civil_time.h>
#include <fmt/base.h>

// #include "../src/lib_includes.h"

using std::vector;

void shifter(vector<float> &arr, const float newmin, const float newmax);

void shifter(std::array<std::array<float, 5>, 4>& arr, const float newmin, const float newmax);

void shifter(std::array<std::array<float, 5>, 6>& arr, const float newmin, const float newmax);

bool approx_equal(double a, double b, double tolerance);

absl::CivilDay parse_date(const std::string& s);

double mean(const std::vector<double>& values);

double stddev(const std::vector<double>& values);

template<typename T>
void show_type(T&&) {
    #ifdef __GNUC__
    fmt::println("{}", __PRETTY_FUNCTION__);
    #elif defined(_MSC_VER)
    fmt::println("{}", __FUNCSIG__);
    #else
    fmt::println("Compiler doesn't support type introspection");
    #endif
}

// less typing for static_cast to use uint8_t's as array indices
inline size_t idx(uint8_t i) {
    return static_cast<size_t>(i);
}

// static cast to size_t and subtract 1
// to convert 1 based indices to zero-based, copies value argument
inline size_t zidx(uint8_t i) {
    return static_cast<size_t>(i-1);
}

// static cast to size_t and add 1
// to convert 0 based indices to one-based, copies value argument
inline size_t nidx(uint8_t i) {
    return static_cast<size_t>(i+1);
}

// simple way to replace this:  std::accumulate(unexposed.begin(), unexposed.end(), 0)
template <std::ranges::input_range R>
auto sum(R &&r) {
  return std::reduce(std::ranges::begin(r), std::ranges::end(r));
}

void replace_all(std::string& s, const std::string& from, const std::string& to);

std::string sanitize_filename_component(std::string_view input);

std::string make_timestamp_token();

std::string make_timestamped_filename(std::string basename);

//
// helpers    
//


#include "lib_includes.h"

#include "helpers.h"

using std::vector;


void shifter(vector<float> &arr, const float newmin, const float newmax) {

  auto minposition = std::min_element(arr.begin(), arr.end());
  auto maxposition = std::max_element(arr.begin(), arr.end());
  const float oldmin = *minposition;
  const float oldmax = *maxposition;

  // Handle edge case: all values are identical
  if (oldmax == oldmin) {
    std::fill(arr.begin(), arr.end(), 1.0f);  // Neutral multiplier
  } else {
    for (auto &element : arr) {
      element = newmin + (newmax - newmin) / (oldmax - oldmin) * (element - oldmin); // we can put this in a function if needed
    }
  }
}

void shifter(std::array<std::array<float, 5>, 4>& arr, const float newmin, const float newmax) {
  float oldmin = arr[0][0];
  float oldmax = arr[0][0];
  for (const auto& row : arr) {
    for (const auto& element : row) {
      if (element < oldmin) oldmin = element;
      if (element > oldmax) oldmax = element;
    }
  }

  if (oldmax == oldmin) {
    for (auto& row : arr) std::fill(row.begin(), row.end(), 1.0f);
  } else {
    for (auto& row : arr) {
      for (auto& element : row) {
        element = newmin + (newmax - newmin) / (oldmax - oldmin) * (element - oldmin);
      }
    }
  }
}

void shifter(std::array<std::array<float, 5>, 6>& arr, const float newmin, const float newmax) {
  float oldmin = arr[0][0];
  float oldmax = arr[0][0];
  for (const auto& row : arr) {
    for (const auto& element : row) {
      if (element < oldmin) oldmin = element;
      if (element > oldmax) oldmax = element;
    }
  }

  if (oldmax == oldmin) {
    for (auto& row : arr) std::fill(row.begin(), row.end(), 1.0f);
  } else {
    for (auto& row : arr) {
      for (auto& element : row) {
        element = newmin + (newmax - newmin) / (oldmax - oldmin) * (element - oldmin);
      }
    }
  }
}

bool approx_equal(double a, double b, double tolerance = 1e-9) {
  return std::abs(a - b) < tolerance;
}

absl::CivilDay parse_date(const std::string& s) {
    int y, m, d;
    std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d);
    return absl::CivilDay(y, m, d);
}


double mean(const std::vector<double>& values) {
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

// we'll rewrite this as a normal loop for the exercise of doing it and for better clarity...
double stddev(const std::vector<double>& values) {
    double m = mean(values);
    double sq_sum = std::accumulate(values.begin(), values.end(), 0.0,
        [m](double acc, double val) {
            return acc + (val - m) * (val - m);
        });
    return std::sqrt(sq_sum / values.size());
}

// text helpers

void replace_all(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string sanitize_filename_component(std::string_view input) {
    std::string out;
    out.reserve(input.size());

    bool last_was_underscore = false;
    bool saw_alnum = false;
    for (unsigned char ch : input) {
        if (std::isalnum(ch) || ch == '-' || ch == '_') {
            out.push_back(static_cast<char>(ch));
            last_was_underscore = false;
            if (std::isalnum(ch)) saw_alnum = true;
        } else if (std::isspace(ch)) {
            if (!out.empty() && !last_was_underscore) {
                out.push_back('_');
                last_was_underscore = true;
            }
        }
    }

    while (!out.empty() && out.back() == '_') out.pop_back();
    if (out.empty() || !saw_alnum) return "case";
    return out;
}

std::string make_timestamp_token() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    const std::tm local_tm = *std::localtime(&now_time);

    return fmt::format("{:04}{:02}{:02}T{:02}{:02}{:02}",
                       local_tm.tm_year + 1900,
                       local_tm.tm_mon + 1,
                       local_tm.tm_mday,
                       local_tm.tm_hour,
                       local_tm.tm_min,
                       local_tm.tm_sec);
}

std::string make_timestamped_filename(std::string basename) {

    return fmt::format("{}_{}", basename, make_timestamp_token());
}
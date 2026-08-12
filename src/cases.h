#pragma once

#include "lib_includes.h"
#include "traits.h"

// forward declarations
struct ModelParams;
struct SocialParams;
class PopData;
struct AgentView;
struct AllSeries;

// A single trait column reference + its expected or new value.
// trait is the AgentView accessor name (e.g. "status", "agegrp", "cond", "variant").
// val is the integer representation of the value, parsed at load time.
struct Term {
  std::string trait;
  int32_t val;
};

// Filter: all terms must match (AND semantics) for a person to be a candidate.
// Allowed traits: "status", "agegrp", "cond", "variant", "vaxstatus", "quar", "tested", "ring"
// "tested" is derived from latest testday != 0.
struct Filter {
  std::vector<Term> terms;
};

// Change: the trait values to set on each matched candidate, and how many to affect.
// Allowed traits: "status", "cond", "duration", "variant", "vaxstatus", "quar"
// When "status"="infectious" is present, make_sick is called (preserving all invariants)
// and a "variant" term is required.
// Guard: only UNEXPOSED and RECOVERED persons may be made sick.
struct Change {
  std::vector<Term> terms;
  int count{0};
};

struct SeedCase {
  int triggerday{0};    // simulation day the changes are applied
  bool startofday{true}; // true = beginning of day, false = end of day
  Filter filter;
  Change change;

  SeedCase(int triggerday, bool startofday, Filter filt, Change chg)
      : triggerday(triggerday), startofday(startofday),
        filter(std::move(filt)), change(std::move(chg)) {}
  SeedCase() = delete;

  std::vector<size_t> operator()(PopData& pop, AllSeries& series);
};

struct SocialDistancing {
  std::string name;
  int startday{0};
  float comply{0.0f};
  std::vector<float> contact_delta;
  std::vector<float> touch_delta;
  vector<Agegrp> include_ages;
  std::array<std::array<float, 5>, 4> contactfactors {};  // 4 rows (contact_rows) × 5 cols (age groups)
  std::array<std::array<float, 5>, 6> touchfactors {};    // 6 rows (touch_rows) × 5 cols (age groups)
  int endday{INT_MAX};
};


void apply_change(AgentView person, const Change& chg, AllSeries& series);
bool matches_filter(AgentView person, const Filter& filt);
// Load a single seed case from a parsed JSON object; requires ModelParams for variant lookup.
SeedCase load_seed_case(const json& sc, const ModelParams& mp);
// Load seed cases from a parsed JSON array; requires ModelParams for variant lookup.
std::vector<SeedCase> load_seed_cases(const json& jdata, const ModelParams& mp);
// Load a single social-distancing case from a parsed JSON object.
SocialDistancing load_sd_case(const json& sdc);
// Load social-distancing cases from a parsed JSON array. Each case's
// contactfactors/touchfactors are seeded from `social` and rescaled into
// [contact_delta[0], contact_delta[1]] and [touch_delta[0], touch_delta[1]].
// Also (re)registers SDCase trait names: index 0 = "none", indices 1..N
// match case positions in the returned vector.
std::vector<SocialDistancing> load_sd_cases(const json& jdata, const SocialParams& social);
void print_sd_cases(const std::vector<SocialDistancing>& sd_cases);
// For each case k (0-based): on day == startday, tag compliers among agents
// passing filter1 (status in {UNEXPOSED, RECOVERED} OR cond in {NIL, MILD})
// and agegrp in include_ages (empty include_ages => all ages) with
// SDCase{k+1}. On day == endday, clear agents currently marked SDCase{k+1}
// back to SDCase{}. Between startday and endday: no-op (marks persist).
void apply_sd_cases_for_day(int day, const std::vector<SocialDistancing>& sd_cases, PopData& pop);

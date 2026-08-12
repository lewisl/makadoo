
#include "population.h"
#include "series.h"
#include "sim.h"
#include "random.h"
#include "helpers.h"
#include <stdexcept>
#include "disease_modeling.h"

namespace {

float require_named_factor(const vector<std::pair<string, float>>& entries,
                           const string& key, const string& context) {
  const auto it = std::find_if(entries.begin(), entries.end(),
                               [&](const auto& entry) { return entry.first == key; });
  if (it == entries.end()) {
    throw std::runtime_error(fmt::format("Missing {} factor for key '{}'", context, key));
  }
  return it->second;
}

// relies on invariant of vector order by variant
float require_ordered_factor(const vector<std::pair<string, float>>& entries, uint8_t key, const string& context) {
  if (key == 0 || key > entries.size())
      throw std::runtime_error(fmt::format("Missing {} factor for key '{}'", context, key));
  return entries[zidx(key)].second;
}

float require_effectiveness(const Vaxparam& params,
                            const string& vaxstat_txt,
                            const string& variant_name) {
  const auto shot_it = std::find_if(params.effectiveness.begin(), params.effectiveness.end(),
                                    [&](const auto& entry) { return entry.first == vaxstat_txt; });
  if (shot_it == params.effectiveness.end()) {
    throw std::runtime_error(fmt::format("Missing vaccine effectiveness for shot '{}'", vaxstat_txt));
  }

  return require_named_factor(shot_it->second, variant_name,
                              fmt::format("effectiveness('{}')", vaxstat_txt));
}

float require_ordered_effectiveness(const Vaxparam& params, uint8_t vaxstat, const string& vaxstat_txt, uint8_t variant_key) {

  if (vaxstat == 0 || vaxstat > params.effectiveness.size())
    throw std::runtime_error(fmt::format("Missing vaccine effectiveness for shot '{}'", vaxstat_txt));

  return require_ordered_factor(params.effectiveness[zidx(vaxstat)].second, variant_key, fmt::format("effectiveness('{}')", vaxstat_txt));
  }

} // namespace

// make_sick: make one person sick
// AgentView class declaration is in population.h
[[clang::always_inline]]
void AgentView::make_sick(Variant var,  AllSeries & series, Condition condition, uint8_t spr_duration) {
  auto today = sim::get_day();
  auto this_age = agegrp();
  auto r = ring().v;
  sim::history_timing.start();
  series.new_status.update(INFECTIOUS, r, this_age, today, 1);
  series.now_status.update(INFECTIOUS, r, this_age, today, 1);
  series.now_variant.update(var, r, this_age, today, 1);
  series.new_variant.update(var, r, this_age, today, 1);

  if (status() == RECOVERED) {
    series.now_status.update(RECOVERED, r, this_age, today, -1);
  } else {
    if (status() == UNEXPOSED) {
      series.now_status.update(UNEXPOSED, r, this_age, today, -1);
    }
  }
  sim::history_timing.cum();
  cond() = condition;
  duration() = spr_duration;
  status() = INFECTIOUS;
  variant() = var;
  sickday() = today;

  auto &history = variant_hist();
  auto &day_history = sickday_hist();
  const bool history_overflow = history.count >= 16;

  history.set(var);
  day_history.set(today);

  if (history_overflow && sim::debug) {
    std::cerr << "Variant and sickday overflow for person " << id
              << ". Oldest history entries lost.\n";
    std::cerr << "variant_hist.count increased to " << static_cast<int>(history.count)
              << ", sickday_hist.count increased to " << static_cast<int>(day_history.count) << "\n";
  }
}


/* 
  make_well: make one person better->recovered and uninfected
AgentView class declaration is in population.h AgentView
method applied to an AgentView instance:  person.make_well()
as a method of AgentView, the instance variable is not used to apply methods or access members 
*/
[[clang::always_inline]]
void AgentView::make_well(AllSeries & series) {    // the object is person--the implied argument
  auto today = sim::get_day();
  auto this_age = agegrp();
  auto r = ring().v;
  sim::history_timing.start();
  series.now_status.update(RECOVERED,  r, this_age, today,  1);
  series.new_status.update(RECOVERED,  r, this_age, today,  1);
  series.now_status.update(INFECTIOUS, r, this_age, today, -1);
  series.now_variant.update(variant(), r, this_age, today, -1);
  sim::history_timing.cum();

  cond() = UNINFECTED; // equivalent to person.cond() in other functions where person defined
  status() = RECOVERED; 
  duration() = 0; 
  recovday() = today;

  auto& history = recovday_hist();
  const bool history_overflow = history.count >= 16;
  history.set(today);

  if (history_overflow && sim::debug) {
    std::cerr << "Recovday overflow for person " << id
              << ". Oldest history entries lost.\n";
    std::cerr << "recovday_hist.count increased to " << static_cast<int>(history.count) << "\n";
  }
}

// this is an AgentView method:  where is the person?  called as person.make_dead(series)
[[clang::always_inline]]
void AgentView::make_dead(AllSeries & series) {
    auto today = sim::get_day();
    auto this_age = agegrp();
    auto r = ring().v;
    sim::history_timing.start();
    series.now_status.update(DEAD,       r, this_age, today,  1);
    series.new_status.update(DEAD,       r, this_age, today,  1);
    series.now_status.update(INFECTIOUS, r, this_age, today, -1);
    series.now_variant.update(variant(), r, this_age, today, -1);
    sim::history_timing.cum();

  // update the person: update deadday and status for the person
  deadday() = today;   
  status() = DEAD; // TODO will we need to set cond to uninfected for any other logic?
}


//clang-format off
uint8_t touch_map(Status target_status, Condition target_cond) {
  switch (target_status)
  {
  case UNEXPOSED:           return uint8_t{0};
  case RECOVERED:           return uint8_t{1};
  case INFECTIOUS:
    switch (target_cond)
    {
    case NIL:               return uint8_t{2};
    case MILD:              return uint8_t{3};
    case SICK:              return uint8_t{4};  
    case SEVERE:            return uint8_t{5};
    default:
      throw std::runtime_error("Invalid condition input for touch_map");
    }
  default:
    throw std::runtime_error("Invalid status input for touch_map");
  }
}
//clang-format on


float infectrisk(vector<InfectParams> &infectparams, uint8_t spr_variant,
                 uint8_t spr_duration, uint8_t contact_agegrp, float recovfactor, float vaxfactor) {
    // spreader person characteristics
    auto sendrisk = infectparams[spr_variant].sendrisk[spr_duration];

    // contact person characteristics
    auto recvrisk = infectparams[spr_variant].recvrisk[zidx(contact_agegrp)];

    auto combined_immunity = vax_recov(vaxfactor, recovfactor);

    auto combinedfactor = recvrisk * sendrisk * combined_immunity;
    auto risk = std::clamp(combinedfactor, 0.0f, 1.0f);    // required because combinedfactor could exceed 1.0

  return risk;
}
                  
bool isinfected(AgentView contact, AgentView spreader,
                vector<InfectParams> &infectparams, const VaxSet& vaxset,
                bool dovax, int thisday) {
    uint8_t spr_variant = spreader.variant();
    float recovfactor = recoveffect(contact, thisday, spr_variant, infectparams);
    float vaxfactor = dovax ? vaxeffect(thisday, contact, vaxset, spr_variant) : 1.0f;
    float risk = infectrisk(infectparams, spr_variant, spreader.duration(),
                            contact.agegrp(), recovfactor, vaxfactor);
    return xo::bernoulli(risk) == 1;  // return a bool, not 0 or 1  1.0f 0.935f 0.763f
}


/*
    recoveffect(const PopData &pop, size_t thisday, size_t contact, uint8_t 
      vector<InfectParams> &infectparams, float csig=6.0, float decay_lower=0.15)

Immunity from recovery for a single person.
defaults: csig = 6.0, decay_lower = 0.15
*/
float recoveffect(AgentView contact, size_t thisday,  uint8_t spr_variant,
                  vector<InfectParams> &infectparams, float csig, float decay_lower) {

    float factor = 1.0f; // return value

    if (contact.recovday() > 0) {
        const size_t recovday = static_cast<size_t>(contact.recovday());
        size_t days_post_recov = thisday - recovday;

        if (days_post_recov >= 0) {
            uint8_t contact_variant = contact.variant();

            // get the max immunity for the variant that target recovered from against the variant of the spreader
            float immstrength = infectparams[contact_variant].recovery_immunity[spr_variant];

            // get the declined value
            float immhalflife = infectparams[contact_variant].immunehalflife;

            // float immdecline = lindecay(days_post_recov, immhalflife, decay_lower);
            float decay = sigdecay(days_post_recov, immhalflife, csig, decay_lower);
            float rise = effect_rise(days_post_recov);
            float time_mod = rise * decay;

            factor = 1.0f - (time_mod * immstrength);
        }
    }
  return factor;
}

float vaxeffect(size_t thisday, AgentView person, const VaxSet& vaxset,
                uint8_t target_variant, float csig, float decay_lower) {
  if (person.vaxstatus() == Vaxstat::none || idx(person.vax()) == 0 || person.vaxday() == 0) {
    return 1.0f;
  }

  const auto& params = vaxset.at(person.vax());   // one instance of Vaxparam of the specific vax
  const int16_t vaxday = person.vaxday();

  if (target_variant >= Variant::names.size()) {
    throw std::runtime_error(fmt::format("Invalid variant index in vaxeffect: {}", target_variant));
  }

  const string variant_name = Variant::names[target_variant];
  const string vaxstat_txt = person.vaxstatus().show();

  // const float infectfactor = require_named_factor(params.infectfactor, variant_name, "infectfactor");
  const float infectfactor = require_ordered_factor(params.infectfactor, target_variant, "infectfactor");

  // const float vaccine_effect = require_effectiveness(params, vaxstat_txt, variant_name);
  const float vaccine_effect = require_ordered_effectiveness(params, person.vaxstatus(), vaxstat_txt, target_variant);

  const int days_after_vax = std::max<int>(static_cast<int>(thisday) - static_cast<int>(vaxday), 0);
  const int days_after_full_effect = std::max(days_after_vax - params.full_effect_days, 0);
  const float rise = effect_rise(static_cast<size_t>(days_after_vax),
                                 params.day1_effect,
                                 static_cast<float>(params.full_effect_days));
  const float decay = sigdecay(static_cast<size_t>(days_after_full_effect),
                               static_cast<float>(params.halflife),
                               csig,
                               decay_lower);
  const float time_mod = rise * decay;

  return std::max(1.0f - (time_mod * vaccine_effect * infectfactor), 0.0f);
}

float vax_recov(float vaxfactor, float recovfactor) {
  return std::min(vaxfactor, recovfactor);
}

/*
gradual decay of vaccine effectiveness or recovery immunity based on assumed half-life
and rise

    effect_rise(days_since; mineff=0.65, delay_days=14)
  
Immunity effectiveness from vaccination or recovery ramps up.
Returns a value between mineff and 1.0. Linear increase.
Default mineff = 0.65, delay_days = 14
*/
float effect_rise(size_t days_since, float mineff, float delay_days) {
    float y = 0.0f;
    if (days_since >= delay_days) y = 1.0f;
    else y = mineff + (days_since/delay_days * (1.0f - mineff));  // rises from mineff to just below 1.0
    return y;
}

float lindecay(size_t t, float h, float lower) {
    float y = 0.5 / -h * t  + 1.0;
    y = y < lower ? lower : y;
    return y;
}

float expdecay(size_t t, float h) {
    float y = exp(-(log(2) / h) * t);
    return y;
}

float sigdecay(size_t t, float h, float csig, float decay_lower) {
    float tfl = static_cast<float>(t);
    float y =
        fmax(1.0 / (1.0 + exp((tfl - h) / (tfl / csig + (h / csig)))), decay_lower);
    return y;
}

float tbrk(float h, float lower) {
    float y = 2.0f * h - (2.0f * h * lower);
    return y;
}

float intercept(size_t t, float hl) {
    float tfl = static_cast<float>(t);
    return -0.3 * tfl / hl + 1.0;
}

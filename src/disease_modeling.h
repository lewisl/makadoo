#pragma once

#include "lib_includes.h"
#include "population.h"


uint8_t touch_map(Status target_status, Condition target_cond);

float touch_probability(AgentView contact,
                        const array<array<float, 5>, 6> &touchfactors,
                        float indoor_factor);
bool istouched(AgentView contact, const array<array<float, 5>, 6> &touchfactors, float indoor_factor);

float infectrisk(vector<InfectParams> &infectparams, uint8_t spr_variant,
                 uint8_t spr_duration, uint8_t contact_agegrp, float recovfactor= 1.0, float vaxfactor= 1.0);

bool isinfected(AgentView contact, AgentView spreader,
                vector<InfectParams> &infectparams, const VaxSet& vaxset,
                bool dovax, int thisday);

float recoveffect(AgentView person, size_t thisday, uint8_t spr_variant, vector<InfectParams> &infectparams,
                  float csig = 6.0, float decay_lower = 0.15);

float vaxeffect(size_t thisday, AgentView person, const VaxSet& vaxset,
                uint8_t target_variant, float csig = 6.0, float decay_lower = 0.15);

float vax_recov(float vaxfactor, float recovfactor);

// decay and rise functions for vaccination effectiveness and partial immunity after recovery

float effect_rise(size_t days_since, float mineff = 0.65, float delay_days = 14);

float lindecay(size_t t, float h, float lower);

float expdecay(size_t t, float h);

float sigdecay(size_t t, float h, float csig = 5.0, float decay_lower = 0.1);

float tbrk(float h, float lower);

float intercept(size_t t, float hl);

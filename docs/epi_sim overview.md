
Explain codebase structure to newcomer

Apr 23·epi_sim·main

Explain the codebase to a newcomer. What is the general structure, what are the important things to know, and what are some pointers for things to learn next?

Copy

Great question — here’s a practical newcomer map of this repo.

## 1) What this codebase is (high-level)

`epi_sim` is a C++23 epidemiological simulation with:

- a command-line runner (`epi_sim` target),
- a central model setup pipeline,
- a day-by-day simulation loop (seeding, distancing, vaccination, spread, progression),
- time-series + population serialization/plot output,
- and a grouped test runner. 

It uses xmake + vcpkg packages (`csv2`, `nlohmann-json`, `fmt`, `abseil`). 

------

## 2) General structure (how files are organized)

### Core folders

- `src/`: simulation and model code.
- `test/`: test groups and test runner.
- `sample_parameters/`: JSON/CSV inputs (config, variants, social, vaccines, seed cases).
- `design/`: architecture notes and design docs.
- `docs/`: additional documentation. 

### Key source modules to know first

- **Entry point / wiring**: `src/epi_sim.cpp`
- **Model assembly**: `src/setup.h/.cpp`
- **Main simulation orchestrator**: `src/sim.cpp`
- **Population state model**: `src/population.h`
- **Parameter schemas/load containers**: `src/parameters.h` (+ loaders in `parameters.cpp`)
- **Series/time history model**: `src/series.h`
- **Scenario case system (seed + social distancing)**: `src/cases.h/.cpp`
- **Kernels**: `spread.*`, `progression.*`, `disease_modeling.*`, `vaccination.*`
- **Output**: `plot.*`, `pop_serialize.*` 

------

## 3) Runtime flow (mental model)

### Step A — CLI + config/seed loading

`main()` parses `--config`, `--seed`, optional `--sd_seed`; loads JSON; resolves relative paths; builds a `Config`; runs setup; loads seed cases and optional SD cases; then calls `runsim`. 

### Step B — model setup

`setup_sim()`:

- loads parameter sets (`GeoData`, infect/progression, social, and optionally vax/vax schedules),
- validates locale and gets population size,
- constructs `PopData`,
- builds calendar days and indoor uplift sequence,
- returns a `Model` aggregate. 

### Step C — day loop

`runsim()` does, for each day:

1. init series day slot,
2. apply seed cases scheduled for start-of-day,
3. apply social-distancing case tags/factors,
4. vaccinate (if enabled),
5. iterate persons 1..N and run spread/progression for infectious agents,
6. finalize series after loop; write CSV outputs and generate plots. 

------

## 4) Most important things to know before editing

### 1) **PopData is 1-based indexed** (critical)

All population vectors are sized `popn + 1`; index 0 is intentionally unused. Loops should be `for i = 1..popn`. This is enforced by convention and appears throughout runtime loops. 

### 2) **SoA data layout + AgentView abstraction**

Population is struct-of-arrays (`status`, `agegrp`, `cond`, etc.) for hot-loop efficiency; `AgentView` is a lightweight row-like proxy for person-oriented logic without materializing a row copy. 

### 3) **Traits are typed wrappers, not raw ints**

Status/age/condition/etc. are small typed structs (`Status`, `Agegrp`, `Condition`, `Vaxstatus`, day wrappers, etc.) with names/show helpers. This improves semantics and serialization/debugging safety. 

### 4) **Series is a first-class subsystem**

`AllSeries` tracks now/new status/vax/variant across age buckets and days (also 1-based day slots), and drives printing/serialization/plot selection via `SeriesColSpec`. 

### 5) **Case system (seeding + social distancing) is declarative**

Seed/SD cases are loaded from JSON and applied by day using trait-based filters/changes; this is the easiest extension point for scenario work. 

------

## 5) Suggested learning path (next things to read)

1. **Start with one full run path**
   - `src/epi_sim.cpp` → `src/setup.cpp` → `src/sim.cpp`.
     This gives you the orchestration and data lifecycle. 
2. **Internalize population model**
   - Read `src/population.h` + `design/popdata_design.md`; focus on SoA, AgentView, and 1-based indexing invariants. 
3. **Learn trait/value system**
   - Read `src/traits.h` to understand what values are safe/expected across columns and filters. 
4. **Read cases next**
   - `src/cases.h/.cpp`: fastest way to add new scenario logic without touching core kernels. 
5. **Then dive into kernels**
   - `disease_modeling.h`, `spread.h`, `progression.h`, `vaccination.h` and corresponding `.cpp` files.
     The design doc explicitly frames this separation of concerns. 
6. **Output + analysis workflow**
   - `series.h`, `pop_serialize.h`, `plot.h` to understand what gets exported/visualized and how to inspect model behavior. 
7. **Run tests by groups while learning**
   - Test groups are explicit (`parameters`, `series`, `setup`, etc.) and easy to run independently. 

------

## 6) Practical newcomer tips

- Keep a sticky note: **indices are 1..popn**, not 0..size-1.
- When adding person-level logic, prefer `AgentView person` in function signatures (as project guidance notes).
- If adding new outputs, plug into existing `SeriesColSpec` / `ColSpec` patterns instead of ad-hoc file writes.
- Start changes in scenario layer (`cases`) before editing core spread/progression kernels.

------

## Commands I used to inspect the repo

- ✅ `rg --files -g 'AGENTS.md'`
- ✅ `find . -maxdepth 2 -type d | sort`
- ✅ `find src -maxdepth 2 -type f | sort`
- ✅ `find test -maxdepth 1 -type f | sort`
- ✅ `find sample_parameters -maxdepth 2 -type f | sort | head -n 80`
- ✅ `sed -n '1,260p' ...` and `nl -ba ... | sed -n 'start,endp'` across core files
- ✅ `git status --short`

If you want, I can next give you a **“first week onboarding plan”** (day-by-day tasks + specific files/functions) tailored to whether you want to focus on modeling, performance, or tooling.
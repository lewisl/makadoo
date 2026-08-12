#include <absl/strings/str_split.h>
#include <cstdlib>

#include "param_init.h"
#include "setup.h"
#include "sim.h"
#include "r0_simulation.h"
#include "show_help.h"
#include "tui_commands.h"


//
// run the application with immediate commands that show information and stop or
// run the simulation with an input case
//

int main(int argc, char** argv) {

  std::string config_path;
  std::string seed_path;
  std::string sd_seed_path;
  
  std::string flag;
  std::string val;

  try {                      // must capture throws from any sub commands on cli or tui path
    if (argc == 1) run_terminal_tui();; 

    
    for (int i = 1; i < argc; i += 2) {
        flag = argv[i];
        val  = (i + 1) < argc ? argv[i + 1] : "";

        if (flag == "--set-project-dir") {
          if (val.empty()) { std::fprintf(stderr, "No value for project dir provided.\n"); std::exit(EXIT_FAILURE); }
          set_project_dir(val);
          exit(0);
        } 

        else if (flag == "--show-project-dir") {
          show_project_dir();
          exit(0);
        }

        else if (flag == "--init-case") {
          if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
          init_case(val);
          exit(0);
        }

        else if (flag == "--run-case") {
          if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
          Model model = use_managed_case(val);
          runsim(model);
        }

        else if (flag == "--show-cases") {
          show_cases();
          exit(0);
        }

        else if (flag == "--setup-dir") {
          if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
          setup_dir(val);
          exit(0);
        }

        else if (flag == "--use-dir") {
          if (val.empty()) { std::fprintf(stderr, "No value for case dir provided.\n"); std::exit(EXIT_FAILURE); }
          Model model = use_dir(val);
          runsim(model);
        }

        else if (flag == "--r0-sim") {
          if (val.empty()) { std::fprintf(stderr, "Must provide a case-label or valid case-dir.\n"); std::exit(EXIT_FAILURE); }
          auto model = r0_sim_setup(val);
          if (model) {
            float r0 = r0_sim(*model);
            fmt::println("r0 estimate: {:.2f}", r0);
          }
          else {fmt::println("Model could not be created for r0 simulation.");}
        }

        else if (flag == "--help") {
          if (val.empty()) {show_help(); exit(0);}
          if (val == "topics") {show_topics(); exit(0);}
          else { show_help(val); exit(0);}
        }

        // just for testing of the tui before fully integrated
        else if (flag == "--tui") {
          run_terminal_tui();
        }

        else {
            fmt::println(stderr, "Unknown flag: {}", flag);
            exit(1);
        }
    }

    return 0;
} catch (const std::exception & e) {
  fmt::println(stderr, "Command failed: {}", e.what());
  return EXIT_FAILURE;
}

  
}

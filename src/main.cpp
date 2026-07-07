#include "cfg/config.hpp"
#include "game/world.hpp"

int main(int argc, char* argv[]) {
    cfg = Config("config.txt", argc, argv);
    if (cfg.help_requested()) {
        cfg.print_help();
        return 0;
    }
    World world(cfg, argc, argv);
    world.run();
    return 0;
}

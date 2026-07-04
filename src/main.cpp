#include "cfg/config.hpp"
#include "game/world.hpp"

int main(int argc, char* argv[]) {
    cfg = Config("config.txt", argc, argv);
    World world(cfg, argc, argv);
    world.run();
    return 0;
}

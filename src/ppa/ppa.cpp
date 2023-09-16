#include "ppa.hpp"

std::unique_ptr<PPAEngine> pickPPAEngine(int engineId) {
    switch (engineId) {
    case 1:
        return std::make_unique<DRPPAEngine>();
        break;
    case 2:
        return std::make_unique<InterpolationPPAEngine>();
        break;
    default:
        return std::make_unique<DisabledPPAEngine>();
    }
}
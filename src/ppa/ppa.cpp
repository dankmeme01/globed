#include "ppa.hpp"

std::unique_ptr<PPAEngine> pickPPAEngine(int engineId) {
    switch (engineId) {
    case 1:
        return std::make_unique<DRPPAEngine>();
    case 2:
        return std::make_unique<InterpolationPPAEngine>();
    default:
        return std::make_unique<DisabledPPAEngine>();
    }
}
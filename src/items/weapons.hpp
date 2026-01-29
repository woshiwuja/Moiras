#include "items.hpp"
namespace moiras {
    class Weapon : public Item {
        float currentDurability;
        float maxDurability;
    };
    class Sword : public Weapon {
        
    };
}
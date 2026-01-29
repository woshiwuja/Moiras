#include "../game/game_object.h"
namespace moiras{
    class Item : public GameObject {
        public:
        unsigned int item_id;
        std::string item_name;
        std::string item_description;
        Image *item_image;
        Model *item_model;
        float weight;
        bool isOutside;
        bool stackable;
        int maxStack = 1;
        void update() override;
        void draw() override{
            BeginMode2D;
        }
    };
    class Food : public Item {
        float nutritional_value;
    };
    class Bread : public Food {

    };
}
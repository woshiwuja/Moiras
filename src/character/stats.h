namespace moiras {
class Statistic {
public:
  int currentExperience;
  int experienceToNext;
  int level;
  Statistic() {
    currentExperience = 0;
    experienceToNext = 100;
    level = 1;
  };
  void levelUp(int levels) { level += levels; }
  void addExperience(int amount) { currentExperience += amount; }
  bool checkLevelUp() { return currentExperience == experienceToNext; }
};

class Health : public Statistic {
  int maxHealth;
  int currentHealth;
  bool alive;
  bool checkAlive() { return currentHealth == 0; };
};

class Strenght : public Statistic {};
class Dexterity : public Statistic {};
class Intelligence : public Statistic {};
class X : public Statistic {};
} // namespace moiras
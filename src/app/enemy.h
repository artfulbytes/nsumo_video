#ifndef ENEMY_H
#define ENEMY_H

/* A software layer that converts the range measuremetns into discrete
 * enemy position and distances to simplify the application code */

#include <stdbool.h>

typedef enum
{
    ENEMY_POS_NONE,
    ENEMY_POS_FRONT_LEFT,
    ENEMY_POS_FRONT,
    ENEMY_POS_FRONT_RIGHT,
    ENEMY_POS_LEFT,
    ENEMY_POS_RIGHT,
    ENEMY_POS_FRONT_AND_FRONT_LEFT,
    ENEMY_POS_FRONT_AND_FRONT_RIGHT,
    ENEMY_POS_FRONT_ALL,
    ENEMY_POS_IMPOSSIBLE // Keep this for debugging
} enemy_pos_e;

typedef enum
{
    ENEMY_RANGE_NONE,
    ENEMY_RANGE_CLOSE,
    ENEMY_RANGE_MID,
    ENEMY_RANGE_FAR,
} enemy_range_e;

struct enemy
{
    enemy_pos_e position;
    enemy_range_e range;
};

void enemy_init(void);
struct enemy enemy_get(void);
bool enemy_detected(const struct enemy *enemy);
bool enemy_at_left(const struct enemy *enemy);
bool enemy_at_right(const struct enemy *enemy);
bool enemy_at_front(const struct enemy *enemy);

#endif // ENEMY_H

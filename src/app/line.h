#ifndef LINE_H
#define LINE_H

// Detect the boundary line of the circular sumobot platform

typedef enum
{
    LINE_NONE,
    LINE_FRONT,
    LINE_BACK,
    LINE_LEFT,
    LINE_RIGHT,
    LINE_FRONT_LEFT,
    LINE_FRONT_RIGHT,
    LINE_BACK_LEFT,
    LINE_BACK_RIGHT,
    LINE_DIAGONAL_LEFT,
    LINE_DIAGONAL_RIGHT
} line_e;

void line_init(void);
line_e line_get(void);

#endif // LINE_H

#ifndef IR_REMOTE_H
#define IR_REMOTE_H

// A driver that decodes the commands sent to the IR receiver (NEC protocol)

typedef enum
{
    IR_CMD_0 = 0x98,
    IR_CMD_1 = 0xA2,
    IR_CMD_2 = 0x62,
    IR_CMD_3 = 0xE2,
    IR_CMD_4 = 0x22,
    IR_CMD_5 = 0x02,
    IR_CMD_6 = 0xC2,
    IR_CMD_7 = 0xE0,
    IR_CMD_8 = 0xA8,
    IR_CMD_9 = 0x90,
    IR_CMD_STAR = 0x68,
    IR_CMD_HASH = 0xB0,
    IR_CMD_UP = 0x18,
    IR_CMD_DOWN = 0x4A,
    IR_CMD_LEFT = 0x10,
    IR_CMD_RIGHT = 0x5A,
    IR_CMD_OK = 0x38,
    IR_CMD_NONE = 0xFF
} ir_cmd_e;

void ir_remote_init(void);
ir_cmd_e ir_remote_get_cmd(void);

#endif // IR_REMOTE_H

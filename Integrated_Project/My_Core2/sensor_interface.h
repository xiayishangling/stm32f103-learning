// sensor_interface.h

#ifndef INC_SENSOR_INTERFACE_H_
#define INC_SENSOR_INTERFACE_H_

typedef struct Sensor Sensor;

struct Sensor{
    const char *name;
    const char *display_name;           // 别名 
    int (*read)(Sensor *me);            // 触发一次测量，0=成功
    float (*get_value)(Sensor *me);     // 拿最新值
    const char *(*get_uint)(Sensor *me);// 返回单位字符串
};

#endif //INC_SENSOR_INTERFACE_H_

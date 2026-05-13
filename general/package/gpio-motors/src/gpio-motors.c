#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int PAN_PINS[4];
int TILT_PINS[4];
int H_ENABLE = -1;   // 水平使能
int V_ENABLE = -1;   // 垂直使能

int STEP_SEQUENCE[8][4] = {
    {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0},
    {0, 0, 1, 0}, {0, 0, 1, 1}, {0, 0, 0, 1}, {1, 0, 0, 1}
};

int REVERSE_STEP_SEQUENCE[8][4] = {
    {1, 0, 0, 1}, {0, 0, 0, 1}, {0, 0, 1, 1}, {0, 0, 1, 0},
    {0, 1, 1, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}
};

void gpio_release(int pin) {
    if (pin < 0) return;
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    FILE *file = fopen(path, "w");
    if (file) {
        fprintf(file, "0");
        fclose(file);
    }
    file = fopen("/sys/class/gpio/unexport", "w");
    if (file) {
        fprintf(file, "%d", pin);
        fclose(file);
    }
}

void gpio_clean(int error) {
    for (int i = 0; i < 4; i++) {
        gpio_release(PAN_PINS[i]);
        gpio_release(TILT_PINS[i]);
    }
    gpio_release(H_ENABLE);
    gpio_release(V_ENABLE);
    if (error) exit(EXIT_FAILURE);
}

void gpio_export(int pin) {
    if (pin < 0) return;
    char path[64];
    FILE *file = fopen("/sys/class/gpio/export", "w");
    if (file) {
        fprintf(file, "%d", pin);
        fclose(file);
    } else {
        printf("Unable to export GPIO %d: %s\n", pin, strerror(errno));
        gpio_clean(1);
    }
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    file = fopen(path, "w");
    if (file) {
        fprintf(file, "out");
        fclose(file);
    } else {
        printf("Unable to set direction of GPIO %d: %s\n", pin, strerror(errno));
        gpio_clean(1);
    }
}

void gpio_set(int pin, int value) {
    if (pin < 0) return;
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    FILE *file = fopen(path, "w");
    if (file) {
        fprintf(file, "%d", value);
        fclose(file);
    } else {
        printf("Unable to set value of GPIO %d: %s\n", pin, strerror(errno));
        gpio_clean(1);
    }
}

void gpio_config() {
    FILE *fp = popen("fw_printenv -n gpio_motors", "r");
    if (!fp) {
        printf("Unable to run fw_printenv\n");
        exit(EXIT_FAILURE);
    }

    char line[64];
    if (fgets(line, sizeof(line), fp) == NULL) {
        printf("Error: Unable to read gpio_motors\n");
        exit(EXIT_FAILURE);
    }
    pclose(fp);

    int value[8];
    int count = 0;
    char *token = strtok(line, " ");
    while (token != NULL && count < 8) {
        value[count++] = atoi(token);
        token = strtok(NULL, " ");
    }

    if (count == 6) {
        // 6 参数模式：前 4 控制线，后 2 使能脚 (H_ENABLE, V_ENABLE)
        for (int i = 0; i < 4; i++) {
            PAN_PINS[i] = value[i];
            TILT_PINS[i] = value[i];
        }
        H_ENABLE = value[4];
        V_ENABLE = value[5];
    } else if (count == 5) {
        // 兼容旧 5 参数模式：单 SELECT_PIN，此时 TILT_PINS = PAN_PINS
        for (int i = 0; i < 4; i++) {
            PAN_PINS[i] = value[i];
            TILT_PINS[i] = value[i];
        }
        // 将原来的 SELECT_PIN 同时赋值给 H_ENABLE 和 V_ENABLE
        // 但硬件不匹配，只能打印警告
        printf("Warning: 5-value config not suitable for dual enable pins. Use 6 values.\n");
        H_ENABLE = value[4];
        V_ENABLE = value[4];
    } else if (count == 8) {
        // 兼容 8 参数模式：前后 4 个分别用于 pan 和 tilt，无使能脚
        for (int i = 0; i < 4; i++) {
            PAN_PINS[i] = value[i];
            TILT_PINS[i] = value[i + 4];
        }
        H_ENABLE = -1;
        V_ENABLE = -1;
    } else {
        printf("Error: Expected 5, 6 or 8 GPIO values, got %d\n", count);
        exit(EXIT_FAILURE);
    }
}

void axis_run(const int pins[4], int axis, int steps, int delay) {
    if (steps == 0) return;

    // 设置使能
    if (axis == 0) {  // 水平
        gpio_set(H_ENABLE, 1);
        gpio_set(V_ENABLE, 0);
    } else {          // 垂直
        gpio_set(H_ENABLE, 0);
        gpio_set(V_ENABLE, 1);
    }

    const int (*seq)[4] = (steps < 0) ? REVERSE_STEP_SEQUENCE : STEP_SEQUENCE;
    int remaining = abs(steps);
    int micro = 0;

    while (remaining > 0) {
        for (int i = 0; i < 4; i++) {
            gpio_set(pins[i], seq[micro][i]);
        }
        usleep(delay);
        if (++micro >= 8) {
            micro = 0;
            remaining--;
        }
    }

    // 控制线归零
    for (int i = 0; i < 4; i++) {
        gpio_set(pins[i], 0);
    }
    // 关闭所有使能
    gpio_set(H_ENABLE, 0);
    gpio_set(V_ENABLE, 0);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <pan steps> <tilt steps> <delay (ms)>\n", argv[0]);
        return 1;
    }

    int pan_steps = atoi(argv[1]);
    int tilt_steps = atoi(argv[2]);
    int delay = atoi(argv[3]) * 1000;   // ms 转 us

    gpio_config();

    // 导出所有用到的 GPIO
    for (int i = 0; i < 4; i++) {
        gpio_export(PAN_PINS[i]);
        gpio_export(TILT_PINS[i]);
    }
    gpio_export(H_ENABLE);
    gpio_export(V_ENABLE);

    // 初始化使能为低
    gpio_set(H_ENABLE, 0);
    gpio_set(V_ENABLE, 0);

    axis_run(PAN_PINS, 0, pan_steps, delay);
    axis_run(TILT_PINS, 1, tilt_steps, delay);

    gpio_clean(0);
    return 0;
}

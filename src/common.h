#ifndef COMMON_H_
#define COMMON_H_

#define LOG(x) \
    fprintf(stdout, "[LOG] - %s\n", (char *)x);

#define RAISE(type, err, ...)                                           \
    do {                                                                \
        fprintf(stderr, "(%s:%s:%d) ERROR: [%s] : " err "\n",           \
                __FILE__, __func__, __LINE__, type, ##__VA_ARGS__);     \
    } while (0)

void scc(int code);
void *scp(void *ptr);

#endif // COMMON_H_
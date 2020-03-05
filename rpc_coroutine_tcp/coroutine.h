#ifndef COROUTINE_H
#define COROUTINE_H

#include <ucontext.h>

#define STACK_SIZE          (8192)

enum coroutine_state{DEAD, READY, RUNNING, SUSPEND};

struct schedule;

typedef void (*coroutine_func)(struct schedule *s, void *args);

typedef struct coroutine
{
    coroutine_func func;        /* 被执行的函数 */
    void *args;                 /* 函数参数 */
    ucontext_t ctx;             /* 当前协程上下文 */
    enum coroutine_state state; /* 协程状态 */
    char stack[STACK_SIZE];     /* 栈区 */
} coroutine_t;

typedef struct schedule
{
    ucontext_t main_ctx;        /* 保存主逻辑上下文 */
    int running_coroutine;      /* 当前运行的协程ID */
    coroutine_t **coroutines;   /* 协程指针列表 */
    int coroutine_num;          /* 协程总数 */
    int max_index;              /* 当前使用的最大ID */
} schedule_t;

// 创建调度器
schedule_t * schedule_create(ssize_t coroutine_num);
// 关闭调度器
void schedule_close(schedule_t *s);
// 检查是否全部执行完成
int schedule_finished(schedule_t *s);

// 创建协程
int coroutine_create(schedule_t *s, coroutine_func func, void *args);
// 删除协程
int coroutine_delete(schedule_t *s, int id);
// 运行协程
int coroutine_run(schedule_t *s, int id);
// 切换协程
void coroutine_yield(schedule_t *s);
// 恢复协程
void coroutine_resume(schedule_t *s, int id);
// 查看协程状态
enum coroutine_state coroutine_get_state(schedule_t *s, int id);

#endif

#include <stdio.h>
#include <assert.h>
#include "rpc_types.h"

struct stu
{
    int id;
    int buf[3];
};

int main()
{
    return_type ans;
    int a = -10, b = 20;

    rpc_client_init();

    // 测试传参：int
    ans = make_remote_call("127.0.0.1",
                                       10000, "addtwo", 2,
                                       sizeof(int), (void *)(&a),
                                       sizeof(int), (void *)(&b));

    assert(ans.return_val != NULL);
    int result = *(int *)(ans.return_val);
    printf("Client, got result: %d\n", result);
    if (ans.need_free) rpc_free(&ans);

    // 测试超时
    // ans = make_remote_call("127.0.0.1",
    //     10001, "addtwo1", 2,
    //     sizeof(int), (void *)(&a),
    //     sizeof(int), (void *)(&b));

    // if (ans.return_val == NULL || ans.return_size == 0)
    //     printf("make_remote_call addtwo1 failed\n");

    // 测试错误调用 
    ans = make_remote_call("127.0.0.1",
        10000, "addtwo2", 2,
        sizeof(int), (void *)(&a),
        sizeof(int), (void *)(&b));

    if (ans.return_val == NULL || ans.return_size == 0)
        printf("make_remote_call addtwo2 failed\n");
    
    // 测试传参：数组
    int arr[3] = {1, 2, 3};
    ans = make_remote_call("127.0.0.1",
                           10000, "get_sum", 3,
                           sizeof(int), (void *)(&arr[0]),
                           sizeof(int), (void *)(&arr[1]),
                           sizeof(int), (void *)(&arr[2]));
    result = *(int *)(ans.return_val);
    printf("Client, got result: %d\n", result);
    if (ans.need_free) rpc_free(&ans);

    char arr2[3] = {1, 2, 3};
    ans = make_remote_call("127.0.0.1",
                           10000, "get_array_sum", 1,
                           sizeof(arr2), (void *)(&arr2[0]));
    result = *(int *)(ans.return_val);
    printf("Client, got result: %d\n", result);
    if (ans.need_free) rpc_free(&ans);

    // 测试传参：结构体
    struct stu mystu;
    mystu.id = 1;
    mystu.buf[0] = 4;
    mystu.buf[1] = 5;
    mystu.buf[2] = 6;
    ans = make_remote_call("127.0.0.1",
                           10000, "get_struct_sum", 1,
                           sizeof(mystu), (void *)(&mystu));

    printf("ans.return_size=%d\n", ans.return_size);
    struct stu *pstu = (struct stu *)(ans.return_val);              
    printf("stu.id=%d\n", pstu->id);
    printf("stu.buf[0]=%d\n", pstu->buf[0]);
    printf("stu.buf[1]=%d\n", pstu->buf[1]);
    printf("stu.buf[2]=%d\n", pstu->buf[2]);

    if (ans.need_free) rpc_free(&ans);

    rpc_client_close();

    return 0;
}

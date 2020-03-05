#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include "rpc_pal.h"

#define RPC_CLIENT_PORT 10069
#define RPC_RCVTIMEO_S	3


// 测试传参：int
void *test_int(void *arg)
{
    int a = -10, b = 20;
    int sockfd = rpc_create_client("127.0.0.1", RPC_SERVER_PORT, RPC_RCVTIMEO_S);
    return_type ans = make_remote_call(sockfd, 
                                        "addtwo", 2,
                                        sizeof(int), (void *)(&a),
                                        sizeof(int), (void *)(&b));

	if (ans.return_val != NULL && ans.return_size > 0)
    {
        int result = *(int *)(ans.return_val);
        printf("[%s] result: %d\n", __func__, result);
        if (ans.need_free) rpc_free(&ans);
    }

    rpc_close(sockfd);
    return NULL;
}

// 测试超时
void *test_timeout(void *arg)
{
    int a = -10, b = 20;
    int sockfd = rpc_create_client("127.0.0.1", RPC_SERVER_PORT, RPC_RCVTIMEO_S);
    return_type ans = make_remote_call(sockfd,
                                        "addtwo", 2,
                                        sizeof(int), (void *)(&a),
                                        sizeof(int), (void *)(&b));

    if (ans.return_val == NULL || ans.return_size == 0)
        printf("make_remote_call failed\n");

    rpc_close(sockfd);
    return NULL;
}

// 测试错误调用 
void *test_call_invalid(void *arg)
{
    int a = -10, b = 20;
    int sockfd = rpc_create_client("127.0.0.1", RPC_SERVER_PORT, RPC_RCVTIMEO_S);
    return_type ans = make_remote_call(sockfd,
                                        "addtwo2", 2,
                                        sizeof(int), (void *)(&a),
                                        sizeof(int), (void *)(&b));

    if (ans.return_val == NULL || ans.return_size == 0)
        printf("make_remote_call failed\n");

    rpc_close(sockfd);
    return NULL;
}

// 测试传参：数组
void *test_array(void *arg)
{
    int arr[3] = {1, 2, 3};
     int sockfd = rpc_create_client("127.0.0.1", RPC_SERVER_PORT, RPC_RCVTIMEO_S);
    return_type ans = make_remote_call(sockfd,
                                        "get_sum", 3,
                                        sizeof(int), (void *)(&arr[0]),
                                        sizeof(int), (void *)(&arr[1]),
                                        sizeof(int), (void *)(&arr[2]));
    if (ans.return_val != NULL && ans.return_size > 0)
    {
        int result = *(int *)(ans.return_val);
        printf("[%s] result: %d\n", __func__, result);
        if (ans.need_free) rpc_free(&ans);
    }
    rpc_close(sockfd);
    return NULL;
}

void *test_array2(void *arg)
{
    char arr[3] = {1, 2, 3};
    int sockfd = rpc_create_client("127.0.0.1", RPC_SERVER_PORT, RPC_RCVTIMEO_S);
    return_type ans = make_remote_call(sockfd,
                                        "get_array_sum", 1,
                                        sizeof(arr), (void *)(&arr[0]));
   	if (ans.return_val != NULL && ans.return_size > 0)
    {
        int result = *(int *)(ans.return_val);
        printf("[%s] result: %d\n", __func__, result);
        if (ans.need_free) rpc_free(&ans);
    }
    rpc_close(sockfd);
    return NULL;
}

// 测试传参：结构体
struct stu
{
    int id;
    int buf[3];
};

void *test_struct(void *arg)
{
    struct stu mystu;
    mystu.id = 1;
    mystu.buf[0] = 4;
    mystu.buf[1] = 5;
    mystu.buf[2] = 6;
    int sockfd = rpc_create_client("127.0.0.1", RPC_SERVER_PORT, RPC_RCVTIMEO_S);
    return_type ans = make_remote_call(sockfd,
                           "get_struct_sum", 1,
                           sizeof(mystu), (void *)(&mystu));

    if (ans.return_val != NULL && ans.return_size > 0)
    {
        struct stu *pstu = (struct stu *)(ans.return_val);              
        printf("stu.id=%d\n", pstu->id);
        printf("stu.buf[0]=%d\n", pstu->buf[0]);
        printf("stu.buf[1]=%d\n", pstu->buf[1]);
        printf("stu.buf[2]=%d\n", pstu->buf[2]);
        if (ans.need_free) rpc_free(&ans);
    }
    rpc_close(sockfd);
    return NULL;
}

typedef void *(*test_func)(void *);

int main()
{
    struct timeval tstart, tstop;
    gettimeofday(&tstart, NULL);
    
    test_func funcs[4] = {test_int, test_array, test_array2, test_struct};
    pthread_t tid[4];
    int i;
    for (i=0; i<4; i++)
    {
        pthread_create(&tid[i], NULL, funcs[i], NULL);
    }

    for (i=0; i<4; i++)
    {
        pthread_join(tid[i], NULL);
    }

    gettimeofday(&tstop, NULL);
    printf("used time: %ldus\n", (tstop.tv_sec - tstart.tv_sec)*1000000 + tstop.tv_usec - tstart.tv_usec); 

    return 0;
}

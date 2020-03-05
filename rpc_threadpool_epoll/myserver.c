#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpc_pal.h"

// 考虑重入性问题

return_type add(const int nparams, arg_type *a)
{
    assert(nparams == 2);
    
    int i = *(int *)(a->arg_val);
    int j = *(int *)(a->next->arg_val);

    return_type rt;
    rpc_malloc(&rt, sizeof(int));
    *((int *)(rt.return_val)) = i + j ;

    return rt;
}

return_type get_sum(const int nparams, arg_type *a)
{
    assert(nparams == 3);
    int i;
    int ret_int = 0;
    arg_type *ptr = a;
    for (i = 0; i < nparams && ptr; ++i)
    {
        // printf("arg[%d]: %d\n", i, *(int *)(ptr->arg_val));
        ret_int += *(int *)(ptr->arg_val);
        ptr = ptr->next;
    }

    return_type rt;
    rpc_malloc(&rt, sizeof(int));
    *((int *)(rt.return_val)) = ret_int;

    return rt;
}

return_type get_array_sum(const int nparams, arg_type *a)
{
    assert(nparams == 1);
    int i;
    int ret_int = 0;
    for (i = 0; i < a->arg_size; ++i)
    {
        // printf("arg[%d]: %d\n", i, *((char *)(a->arg_val) + i));
        ret_int += *((char *)(a->arg_val) + i);
    }

    return_type rt;
    rpc_malloc(&rt, sizeof(int));
    *((int *)(rt.return_val)) = ret_int;

    return rt;
}

struct stu
{
    int id;
    int buf[3];
};

return_type get_struct_sum(const int nparams, arg_type *a)
{
    assert(nparams == 1);
    struct stu *pstu = (struct stu *)a->arg_val;
    // printf("stu.id=%d\n", pstu->id);
    // printf("stu.buf[0]=%d\n", pstu->buf[0]);
    // printf("stu.buf[1]=%d\n", pstu->buf[1]);
    // printf("stu.buf[2]=%d\n", pstu->buf[2]);

    return_type rt;
    rpc_malloc(&rt, sizeof(struct stu));
    memcpy(rt.return_val, pstu, rt.return_size);
    
    return rt;
}

int main()
{
    register_procedure("addtwo", 2, add);
    register_procedure("get_sum", 3, get_sum);
    register_procedure("get_array_sum", 1, get_array_sum);
    register_procedure("get_struct_sum", 1, get_struct_sum);

    launch_server(4, 10);

    return 0;
}

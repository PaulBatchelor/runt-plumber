#ifndef PLUMBER_RUNT_H
#define PLUMBER_RUNT_H
int runt_plumber_create(plumber_data *pd, 
    sporth_stack *stack, 
    void **ud, 
    runt_int(* loader)(runt_vm *));

int runt_plumber_init(plumber_data *pd, 
    sporth_stack *stack, 
    void **ud);

int runt_plumber_compute(plumber_data *pd, sporth_stack *stack, void **ud);

int runt_plumber_destroy(plumber_data *pd, sporth_stack *stack, void **ud);
#endif

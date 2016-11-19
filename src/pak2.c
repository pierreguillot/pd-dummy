/*
// Copyright (c) 2016 Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <m_pd.h>

static t_class *pak2_class;
static t_class* pak2_inlet_class;

struct _pak2_inlet;

typedef struct _pak2
{
    t_object            x_obj;
    t_int               x_n;
    t_atom*             x_vec;
    t_atom*             x_out;
    struct _pak2_inlet*  x_ins;
} t_pak2;

typedef struct _pak2_inlet
{
    t_class*    x_pd;
    t_atom*     x_atoms;
    t_int       x_max;
    t_pak2*      x_owner;
} t_pak2_inlet;


static void *pak2_new(t_symbol *s, int argc, t_atom *argv)
{
    int i;
    char c;
    t_pak2 *x = (t_pak2 *)pd_new(pak2_class);
    t_atom defarg[2];
    if(!argc)
    {
        argv = defarg;
        argc = 2;
        SETFLOAT(&defarg[0], 0);
        SETFLOAT(&defarg[1], 0);
    }
    
    x->x_n   = argc;
    x->x_vec = (t_atom *)getbytes(argc * sizeof(*x->x_vec));
    x->x_out = (t_atom *)getbytes(argc * sizeof(*x->x_out));
    x->x_ins = (t_pak2_inlet *)getbytes(argc * sizeof(*x->x_ins));
    
    for(i = 0; i < x->x_n; ++i)
    {
        if(argv[i].a_type == A_FLOAT)
        {
            x->x_vec[i].a_type      = A_FLOAT;
            x->x_vec[i].a_w.w_float = argv[i].a_w.w_float;
            x->x_ins[i].x_pd    = pak2_inlet_class;
            x->x_ins[i].x_atoms = x->x_vec+i;
            x->x_ins[i].x_max   = x->x_n-i;
            x->x_ins[i].x_owner = x;
            inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
        }
        else if(argv[i].a_type == A_SYMBOL)
        {
            c = argv[i].a_w.w_symbol->s_name[0];
            if(c == 's')
            {
                x->x_vec[i].a_type       = A_SYMBOL;
                x->x_vec[i].a_w.w_symbol = &s_symbol;
                x->x_ins[i].x_pd    = pak2_inlet_class;
                x->x_ins[i].x_atoms = x->x_vec+i;
                x->x_ins[i].x_max   = x->x_n-i;
                x->x_ins[i].x_owner = x;
                inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
            }
            else if(c == 'p')
            {
                x->x_vec[i].a_type         = A_POINTER;
                x->x_vec[i].a_w.w_gpointer = (t_gpointer *)getbytes(sizeof(*x->x_vec[i].a_w.w_gpointer));
                gpointer_init(x->x_vec[i].a_w.w_gpointer);
                x->x_ins[i].x_pd    = pak2_inlet_class;
                x->x_ins[i].x_atoms = x->x_vec+i;
                x->x_ins[i].x_max   = x->x_n-i;
                x->x_ins[i].x_owner = x;
                inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
            }
            else
            {
                if(c != 'f')
                {
                    pd_error(x, "pak2: %s: bad type", argv[i].a_w.w_symbol->s_name);
                }
                x->x_vec[i].a_type      = A_FLOAT;
                x->x_vec[i].a_w.w_float = 0.f;
                x->x_ins[i].x_pd    = pak2_inlet_class;
                x->x_ins[i].x_atoms = x->x_vec+i;
                x->x_ins[i].x_max   = x->x_n-i;
                x->x_ins[i].x_owner = x;
                inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
            }
        }
    }
    outlet_new(&x->x_obj, &s_list);
    return (x);
}

static void pak2_bang(t_pak2 *x)
{
    int i;
    for(i = 0; i < x->x_n; ++i)
    {
        if(x->x_vec[i].a_type == A_POINTER)
        {
            if(!gpointer_check(x->x_vec[i].a_w.w_gpointer, 1))
            {
                pd_error(x, "pak2: stale pointer");
                return;
            }
        }
        x->x_out[i] = x->x_vec[i];
    }
    outlet_list(x->x_obj.ob_outlet, &s_list, x->x_n, x->x_out);
}

static void pak2_copy(t_pak2 *x, int ndest, t_atom* dest, int nsrc, t_atom* src)
{
    int i;
    for(i = 0; i < ndest && i < nsrc; ++i)
    {
        if(src[i].a_type == A_FLOAT)
        {
            if(dest[i].a_type == A_FLOAT)
            {
                dest[i].a_w.w_float = src[i].a_w.w_float;
            }
            else
            {
                pd_error(x, "pak2: wrong type (float)");
            }
        }
        else if(src[i].a_type == A_POINTER)
        {
            if(dest[i].a_type == A_POINTER)
            {
                gpointer_unset(dest[i+1].a_w.w_gpointer);
                *(dest[i+1].a_w.w_gpointer) = *(src[i].a_w.w_gpointer);
                if(dest[i+1].a_w.w_gpointer->gp_stub)
                {
                    dest[i+1].a_w.w_gpointer->gp_stub->gs_refcount++;
                }
            }
            else
            {
                pd_error(x, "pak2: wrong type (pointer)");
            }
        }
        if(src[i].a_type == A_SYMBOL)
        {
            if(dest[i].a_type == A_SYMBOL)
            {
                dest[i].a_w.w_symbol = src[i].a_w.w_symbol;
            }
            else
            {
                pd_error(x, "pak2: wrong type (symbol)");
            }
        }
    }
}

static void pak2_free(t_pak2 *x)
{
    int i;
    for(i = 0; i < x->x_n; ++i)
    {
        if(x->x_vec[i].a_type == A_POINTER)
        {
            gpointer_unset(x->x_vec[i].a_w.w_gpointer);
            freebytes(x->x_vec[i].a_w.w_gpointer, sizeof(t_gpointer));
            x->x_vec[i].a_w.w_gpointer = NULL;
        }
    }
    freebytes(x->x_vec, x->x_n * sizeof(*x->x_vec));
    freebytes(x->x_out, x->x_n * sizeof(*x->x_out));
    freebytes(x->x_ins, x->x_n * sizeof(*x->x_ins));
}











static void pak2_inlet_bang(t_pak2_inlet *x)
{
    pak2_bang(x->x_owner);
}

static void pak2_inlet_float(t_pak2_inlet *x, float f)
{
    if(x->x_atoms->a_type == A_FLOAT)
    {
        x->x_atoms->a_w.w_float = f;
        pak2_bang(x->x_owner);
    }
    else
    {
        pd_error(x, "pak2: wrong type (float)");
    }
}

static void pak2_inlet_pointer(t_pak2_inlet *x, t_gpointer *gp)
{
    if(x->x_atoms->a_type == A_POINTER)
    {
        gpointer_unset(x->x_atoms->a_w.w_gpointer);
        *(x->x_atoms->a_w.w_gpointer) = *gp;
        if(x->x_atoms->a_w.w_gpointer->gp_stub)
        {
            x->x_atoms->a_w.w_gpointer->gp_stub->gs_refcount++;
        }
        pak2_bang(x->x_owner);
    }
    else
    {
        pd_error(x, "pak2: wrong type (pointer)");
    }
}

static void pak2_inlet_symbol(t_pak2_inlet *x, t_symbol* s)
{
    if(x->x_atoms->a_type == A_SYMBOL)
    {
        x->x_atoms->a_w.w_symbol = s;
        pak2_bang(x->x_owner);
    }
    else
    {
        pd_error(x, "pak2: wrong type (symbol)");
    }
}

static void pak2_inlet_list(t_pak2_inlet *x, t_symbol* s, int argc, t_atom* argv)
{
    pak2_copy(x->x_owner, x->x_max, x->x_atoms, argc, argv);
    pak2_bang(x->x_owner);
}

static void pak2_inlet_anything(t_pak2_inlet *x, t_symbol* s, int argc, t_atom* argv)
{
    if(x->x_atoms[0].a_type == A_SYMBOL)
    {
        x->x_atoms[0].a_w.w_symbol = s;
    }
    else
    {
        pd_error(x, "pak2: wrong type (symbol)");
    }
    pak2_copy(x->x_owner, x->x_max-1, x->x_atoms+1, argc, argv);
    pak2_bang(x->x_owner);
}

static void pak2_inlet_set(t_pak2_inlet *x, t_symbol* s, int argc, t_atom* argv)
{
    pak2_copy(x->x_owner, x->x_max, x->x_atoms, argc, argv);
}








extern void pak2_setup(void)
{
    t_class* c = NULL;
    
    c = class_new(gensym("pak2-inlet"), 0, 0, sizeof(t_pak2_inlet), CLASS_PD, 0);
    if(c)
    {
        class_addbang(c,    (t_method)pak2_inlet_bang);
        class_addpointer(c, (t_method)pak2_inlet_pointer);
        class_addfloat(c,   (t_method)pak2_inlet_float);
        class_addsymbol(c,  (t_method)pak2_inlet_symbol);
        class_addlist(c,    (t_method)pak2_inlet_list);
        class_addanything(c,(t_method)pak2_inlet_anything);
        class_addmethod(c,  (t_method)pak2_inlet_set, gensym("set"), A_GIMME, 0);
    }
    pak2_inlet_class = c;
    
    c = class_new(gensym("pak2"), (t_newmethod)pak2_new, (t_method)pak2_free, sizeof(t_pak2), CLASS_NOINLET, A_GIMME, 0);
    pak2_class = c;
}

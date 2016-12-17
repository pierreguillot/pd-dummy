/*
 // Copyright (c) 2016 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>

static t_class *meanblock_tilde_class;

typedef struct _meanblock_tilde
{
    t_object    x_obj;
    
    t_sample*   t_buffer;
    t_int       t_buffer_nrows;
    t_int       t_buffer_ncols;
    t_int       t_crow;
    t_float     t_dummy;
} t_meanblock_tilde;

void meanblock_tilde_free(t_meanblock_tilde *x)
{
    if(x->t_buffer)
    {
        freebytes(x->t_buffer, x->t_buffer_ncols * x->t_buffer_nrows * sizeof(t_sample *));
        x->t_buffer = NULL;
    }
    x->t_buffer_nrows = 0;
    x->t_buffer_ncols = 0;
}

void meanblock_tilde_buffer_alloc(t_meanblock_tilde *x, t_int nrows, t_int ncols)
{
    t_int i;
    if(nrows && ncols)
    {
        meanblock_tilde_free(x);
        x->t_buffer = (t_sample*)getbytes(nrows * ncols * sizeof(t_sample));
        if(x->t_buffer)
        {
            for(i = 0; i < nrows * ncols; ++i)
            {
                x->t_buffer[i] = 0;
            }
            x->t_buffer_nrows = nrows;
            x->t_buffer_ncols = ncols;
        }
        else
        {
            pd_error(x, "meanblock~ can't allocate memory.");
        }
    }
    else
    {
        pd_error(x, "meanblock~ the numbers of rows and columns are negatives or null.");
    }
}

t_int *meanblock_tilde_perform(t_int *w)
{
    t_meanblock_tilde *x      = (t_meanblock_tilde *)(w[1]);
    t_sample        *in     = (t_sample *)(w[2]);
    t_sample        *out    = (t_sample *)(w[3]);
    t_int           n       = (t_int)(w[4]);
    t_int           nrows   = x->t_buffer_nrows;
    t_int           crow    = x->t_crow;
    
    
    t_sample* cout = out;
    t_sample* read = x->t_buffer;
    t_sample* write = read + crow * n;
    t_int           i, j;
    
    for(i = 0; i < n; ++i)
    {
        *write++ = *in++;
        *cout++ = 0.f;
    }
    
    for(i = 0; i < nrows; ++i)
    {
        cout = out;
        for(j = 0; j < n; ++j)
        {
            *cout++ = *read++;
        }
    }
    
    cout = out;
    for(i = 0; i < n; ++i)
    {
        *cout++ /= (t_sample)nrows;
    }
    
    x->t_crow++;
    if(x->t_crow >= nrows)
    {
        x->t_crow = 0;
    }
    return (w + 5);
}

void meanblock_tilde_dsp(t_meanblock_tilde *x, t_signal **sp)
{
    meanblock_tilde_buffer_alloc(x,  x->t_buffer_nrows, (t_int)sp[0]->s_n);
    dsp_add(meanblock_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void meanblock_tilde_length(t_meanblock_tilde *x, t_floatarg f)
{
    if(f > 0)
    {
        x->t_buffer_nrows = (t_int)f;
        if(x->t_buffer_ncols)
        {
            meanblock_tilde_buffer_alloc(x, x->t_buffer_nrows, x->t_buffer_ncols);
        }
    }
    else
    {
        pd_error(x, "meanblock~ please give me a positive length.");
    }
    
}

void *meanblock_tilde_new(t_floatarg f)
{
    t_meanblock_tilde *x = (t_meanblock_tilde *)pd_new(meanblock_tilde_class);
    if(x)
    {
        x->t_buffer = NULL;
        x->t_buffer = NULL;
        x->t_buffer_ncols = 0;
        if(f > 0)
        {
            x->t_buffer_nrows = (t_int)f;
        }
        else
        {
            x->t_buffer_nrows = 1;
            pd_error(x, "meanblock~ please give me a positive length.");
        }
        x->t_crow     = 0;
        outlet_new(&x->x_obj, &s_signal);
    }
    return x;
}

EXTERN void meanblock_tilde_setup(void)
{
    t_class *c = class_new(gensym("meanblock~"), (t_newmethod)meanblock_tilde_new, (t_method)meanblock_tilde_free,
                           sizeof(t_meanblock_tilde), CLASS_DEFAULT, A_DEFFLOAT, 0);
    if(c)
    {
        class_addmethod(c, (t_method)meanblock_tilde_dsp,     gensym("dsp"),      A_CANT,     0);
        class_addmethod(c, (t_method)meanblock_tilde_length,  gensym("length"),   A_FLOAT,    0);
        CLASS_MAINSIGNALIN(c, t_meanblock_tilde, t_dummy);
    }
    meanblock_tilde_class = c;
}


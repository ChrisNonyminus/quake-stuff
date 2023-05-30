/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "r_local.h"

model_t *loadmodel;
char loadname[32]; // for hunk tags

void Mod_LoadSpriteModel(model_t *mod, void *buffer);
// >>> FIX: For Nintendo DS using devkitARM
// Redeclaring so it receives a filepartdata_t instead of the whole buffer:
// void Mod_LoadBrushModel (model_t *mod, void *buffer);
// void Mod_LoadAliasModel (model_t *mod, void *buffer);
void Mod_LoadBrushModel(model_t *mod, filepartdata_t *fpdata);
void Mod_LoadAliasModel(model_t *mod, filepartdata_t *fpdata);
// <<< FIX
model_t *Mod_LoadModel(model_t *mod, qboolean crash);

byte mod_novis[MAX_MAP_LEAFS / 8];

#define MAX_MOD_KNOWN 256
model_t mod_known[MAX_MOD_KNOWN];
int mod_numknown;

// values for model_t's needload
#define NL_PRESENT 0
#define NL_NEEDS_LOADED 1
#define NL_UNREFERENCED 2

// >>> FIX: For Nintendo DS using devkitARM
// New cvars to scale down textures:
cvar_t mdl_brushtexscale = {"mdl_brushtexscale", "2", true};
cvar_t mdl_aliastexscale = {"mdl_aliastexscale", "1", true};
// <<< FIX

// >>> FIX: For Nintendo DS using devkitARM
// New cvar to load only a specific number of textures (if not playing demos):
cvar_t mdl_numtex = {"mdl_numtex", "5", true};
// <<< FIX

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (byte *in, int inwidth, int inheight, byte *out,  int outwidth, int outheight)
{
	int		i, j;
	byte	*inrow;
	unsigned	frac, fracstep;

	fracstep = inwidth*0x10000/outwidth;
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(i*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j+=1)
		{
			out[j] = inrow[frac>>16];
			frac += fracstep;
		}
	}
}

/*
===============
Mod_Init
===============
*/
void Mod_Init(void)
{
    memset(mod_novis, 0xff, sizeof(mod_novis));
    // >>> FIX: For Nintendo DS using devkitARM
    // Registering new cvars defined above:
    Cvar_RegisterVariable(&mdl_brushtexscale);
    Cvar_RegisterVariable(&mdl_aliastexscale);
    Cvar_RegisterVariable(&mdl_numtex);
    // <<< FIX
}

/*
===============
Mod_Extradata

Caches the data if needed
===============
*/
void *Mod_Extradata(model_t *mod)
{
    void *r;

    r = Cache_Check(&mod->cache);
    if (r)
        return r;

    Mod_LoadModel(mod, true);

    if (!mod->cache.data)
        Sys_Error("Mod_Extradata: caching failed");
    return mod->cache.data;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model)
{
    mnode_t *node;
    float d;
    mplane_t *plane;

    if (!model || !model->nodes)
        Sys_Error("Mod_PointInLeaf: bad model");

    node = model->nodes;
    while (1)
    {
        if (node->contents < 0)
            return (mleaf_t *)node;
        plane = node->plane;
        d = DotProduct(p, plane->normal) - plane->dist;
        if (d > 0)
            node = node->children[0];
        else
            node = node->children[1];
    }

    return NULL; // never reached
}

/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis(byte *in, model_t *model)
{
    static byte decompressed[MAX_MAP_LEAFS / 8];
    int c;
    byte *out;
    int row;

    row = (model->numleafs + 7) >> 3;
    out = decompressed;

    if (!in)
    { // no vis info, so make all visible
        while (row)
        {
            *out++ = 0xff;
            row--;
        }
        return decompressed;
    }

    do
    {
        if (*in)
        {
            *out++ = *in++;
            continue;
        }

        c = in[1];
        in += 2;
        while (c)
        {
            *out++ = 0;
            c--;
        }
    } while (out - decompressed < row);

    return decompressed;
}

byte *Mod_LeafPVS(mleaf_t *leaf, model_t *model)
{
    if (leaf == model->leafs)
        return mod_novis;
    return Mod_DecompressVis(leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll(void)
{
    int i;
    model_t *mod;

    for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
    {
        mod->needload = NL_UNREFERENCED;
        // FIX FOR CACHE_ALLOC ERRORS:
        if (mod->type == mod_sprite)
            mod->cache.data = NULL;
    }
}

/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName(char *name)
{
    int i;
    model_t *mod;
    model_t *avail = NULL;

    if (!name[0])
        Sys_Error("Mod_ForName: NULL name");

    //
    // search the currently loaded models
    //
    for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
    {
        if (!strcmp(mod->name, name))
            break;
        if (mod->needload == NL_UNREFERENCED)
            if (!avail || mod->type != mod_alias)
                avail = mod;
    }

    if (i == mod_numknown)
    {
        if (mod_numknown == MAX_MOD_KNOWN)
        {
            if (avail)
            {
                mod = avail;
                if (mod->type == mod_alias)
                    if (Cache_Check(&mod->cache))
                        Cache_Free(&mod->cache);
            }
            else
                Sys_Error("mod_numknown == MAX_MOD_KNOWN");
        }
        else
            mod_numknown++;
        strcpy(mod->name, name);
        mod->needload = NL_NEEDS_LOADED;
    }

    return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel(char *name)
{
    model_t *mod;

    mod = Mod_FindName(name);

    if (mod->needload == NL_PRESENT)
    {
        if (mod->type == mod_alias)
            Cache_Check(&mod->cache);
    }
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
model_t *Mod_LoadModel(model_t *mod, qboolean crash)
{
    unsigned *buf;
    // >>> FIX: For Nintendo DS using devkitARM
    // Deferring allocation. Stack in this device is pretty small:
    // byte	stackbuf[1024];		// avoid dirtying the cache heap
    byte *stackbuf; // avoid dirtying the cache heap
                    // <<< FIX
                    // >>> FIX: For Nintendo DS using devkitARM
                    // Declaring the file partial loading struct:
    filepartdata_t *fpdata;
    // <<< FIX

    if (mod->type == mod_alias)
    {
        if (Cache_Check(&mod->cache))
        {
            mod->needload = NL_PRESENT;
            return mod;
        }
    }
    else
    {
        if (mod->needload == NL_PRESENT)
            return mod;
    }

    //
    // because the world is so huge, load it one piece at a time
    //

    //
    // load the file
    //
    // >>> FIX: For Nintendo DS using devkitARM
    // Allocating for previous fixes (and, this time, expanding the allocated
    // size):
    stackbuf = Sys_Malloc(4096 * sizeof(byte), "Mod_LoadModel");
    fpdata = Sys_Malloc(sizeof(filepartdata_t), "Mod_LoadModel");
    // <<< FIX
    // >>> FIX: For Nintendo DS using devkitARM
    // 1) Compensating for local variable change;
    // 2) Loading only the first 4 bytes of this file, for now:
    // buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf,
    // sizeof(stackbuf));
    Q_memset(fpdata, 0, sizeof(*fpdata));
    Q_strcpy(fpdata->name, mod->name);
    fpdata->offset = 0;
    fpdata->length = 4;
    fpdata->stackbuf = stackbuf;
    fpdata->stackbuflen = 4096 * sizeof(byte);
    COM_LoadStackFilePartial(fpdata, true);
    buf = ((unsigned *)(fpdata->buf));
    // <<< FIX
    if (!buf)
    {
        if (crash)
            Sys_Error("Mod_NumForName: %s not found", mod->name);
        // >>> FIX: For Nintendo DS using devkitARM
        // Deallocating from previous fixes:
        free(fpdata);
        free(stackbuf);
        // <<< FIX
        return NULL;
    }

    //
    // allocate a new model
    //
    COM_FileBase(mod->name, loadname);

    loadmodel = mod;

    //
    // fill it in
    //

    // call the apropriate loader
    mod->needload = NL_PRESENT;

    switch (LittleLong(*(unsigned *)buf))
    {
    case IDPOLYHEADER:
        // >>> FIX: For Nintendo DS using devkitARM
        // Sending fpdata to the next call (instead of the whole buffer):
        // Mod_LoadAliasModel (mod, buf);
        Mod_LoadAliasModel(mod, fpdata);
        // <<< FIX
        break;

    case IDSPRITEHEADER:
        // >>> FIX: For Nintendo DS using devkitARM
        // Reloading the entire file for the following call:
        buf = (unsigned *)COM_LoadStackFile(mod->name, stackbuf,
                                            1024 * sizeof(byte));
        // <<< FIX
        Mod_LoadSpriteModel(mod, buf);
        break;

    default:
        // >>> FIX: For Nintendo DS using devkitARM
        // Sending fpdata to the next call (instead of the whole buffer):
        // Mod_LoadBrushModel (mod, buf);
        Mod_LoadBrushModel(mod, fpdata);
        // <<< FIX
        break;
    }

    // >>> FIX: For Nintendo DS using devkitARM
    // Deallocating from previous fixes:
    free(fpdata);
    free(stackbuf);
    // <<< FIX

    return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName(char *name, qboolean crash)
{
    model_t *mod;

    mod = Mod_FindName(name);

    return Mod_LoadModel(mod, crash);
}

/*
===============================================================================

                    BRUSHMODEL LOADING

===============================================================================
*/

byte *mod_base;

extern int integer_to_pow2(int);
/*
=================
Mod_LoadTextures
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadTextures (lump_t *l)
void Mod_LoadTextures(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    int i, j, pixels, num, max, altmax;
    miptex_t *mt;
    texture_t *tx, *tx2;
    texture_t *anims[10];
    texture_t *altanims[10];
    dmiptexlump_t *m;
    // >>> FIX: For Nintendo DS using devkitARM
    // New variables to handle loading only certain textures:
    qboolean doload;
    int tcount;
    int tloaded;
    int tlatest;
    // <<< FIX

    if (!l->filelen)
    {
        loadmodel->textures = NULL;
        return;
    }
    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // m = (dmiptexlump_t *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    m = (dmiptexlump_t *)(fpdata->buf);
    // <<< FIX

    m->nummiptex = LittleLong(m->nummiptex);

    loadmodel->numtextures = m->nummiptex;
    loadmodel->textures =
        Hunk_AllocName(m->nummiptex * sizeof(*loadmodel->textures), loadname);

    // >>> FIX: For Nintendo DS using devkitARM
    // Resetting texture counters for texture loading:
    tcount = 0;
    tloaded = 0;
    tlatest = 0;
    // <<< FIX
    for (i = 0; i < m->nummiptex; i++)
    {
        m->dataofs[i] = LittleLong(m->dataofs[i]);
        if (m->dataofs[i] == -1)
            continue;
        mt = (miptex_t *)((byte *)m + m->dataofs[i]);
        // >>> FIX: For Nintendo DS using devkitARM
        // Loading only the required textures (based on the new cvar), part 1:
        if (cls.demoplayback)
        {
            doload = true;
        }
        else if (mdl_numtex.value > 0)
        {
            doload = ((tloaded < mdl_numtex.value) ||
                      (!Q_strncmp(mt->name, "sky", 3)) ||
                      (!Q_strncmp(mt->name, "*", 1)));
            if (mt->name[0] == '+')
            {
                mt->name[0] = '_';
            };
        }
        else
        {
            doload = true;
        };
        if (doload)
        {
            tloaded++;
            tlatest = i;
            // <<< FIX
            mt->width = LittleLong(mt->width);
            mt->height = LittleLong(mt->height);
            for (j = 0; j < MIPLEVELS; j++)
                mt->offsets[j] = LittleLong(mt->offsets[j]);

            if ((mt->width & 15) || (mt->height & 15))
                Sys_Error("Texture %s is not 16 aligned", mt->name);
            {
                pixels = mt->width * mt->height / 64 * 85;
                tx = Hunk_AllocName(sizeof(texture_t) + pixels, loadname);
                loadmodel->textures[i] = tx;

                memcpy(tx->name, mt->name, sizeof(tx->name));
                tx->width = mt->width;
                tx->height = mt->height;
                for (j = 0; j < MIPLEVELS; j++)
                    tx->offsets[j] =
                        mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);
                // the pixels immediately follow the structures
                memcpy(tx + 1, mt + 1, pixels);
                if (tx->height > 32) {
                    tx->rwidth = integer_to_pow2(((double)tx->width / ((double)tx->height / 32.0)));
                    if (tx->rwidth != 8 && tx->rwidth < 8)
                        tx->rwidth = 8;
                    else if (tx->rwidth != 16 && tx->rwidth < 16)
                        tx->rwidth = 16;
                    else if (tx->rwidth != 32 && tx->rwidth < 32)
                        tx->rwidth = 32;
                    tx->resampled = Hunk_AllocName( tx->rwidth* 32, loadname);
			        GL_ResampleTexture((byte*)(tx + 1), tx->width, tx->height, tx->resampled, tx->rwidth  , 32);

                    tx->rheight = 32;
                    tx->rscale = ((double)tx->height / 32.0);
                }else {
                    tx->resampled = Hunk_AllocName(integer_to_pow2(tx->width) * integer_to_pow2(tx->height), loadname);
                    tx->rwidth = integer_to_pow2(tx->width) ; tx->rheight = integer_to_pow2(tx->height);
                    GL_ResampleTexture((byte*)(tx + 1), tx->width, tx->height, tx->resampled, tx->rwidth, tx->rheight);
                    tx->rscale = 0;
                }
            }
            // <<< FIX
            if (!Q_strncmp(mt->name, "sky", 3))
                R_InitSky(tx);
            // >>> FIX: For Nintendo DS using devkitARM
            // Loading only the required textures (based on the new cvar), part
            // 2:
        }
        else
        {
            loadmodel->textures[i] = loadmodel->textures[tcount];
            do
            {
                tcount++;
                if (tcount > tlatest)
                {
                    tcount = 0;
                };

                // hack to make sure if doesn't crash
                if (!loadmodel->textures[tcount])
                    break;
            } while (
                (!Q_strncmp(loadmodel->textures[tcount]->name, "sky", 3)) ||
                (!Q_strncmp(loadmodel->textures[tcount]->name, "*", 1)));
        };
        // <<< FIX
    }

    //
    // sequence the animations
    //
    for (i = 0; i < m->nummiptex; i++)
    {
        tx = loadmodel->textures[i];
        if (!tx || tx->name[0] != '+')
            continue;
        if (tx->anim_next)
            continue; // allready sequenced

        // find the number of frames in the animation
        memset(anims, 0, sizeof(anims));
        memset(altanims, 0, sizeof(altanims));

        max = tx->name[1];
        altmax = 0;
        if (max >= 'a' && max <= 'z')
            max -= 'a' - 'A';
        if (max >= '0' && max <= '9')
        {
            max -= '0';
            altmax = 0;
            anims[max] = tx;
            max++;
        }
        else if (max >= 'A' && max <= 'J')
        {
            altmax = max - 'A';
            max = 0;
            altanims[altmax] = tx;
            altmax++;
        }
        else
            Sys_Error("Bad animating texture %s", tx->name);

        for (j = i + 1; j < m->nummiptex; j++)
        {
            tx2 = loadmodel->textures[j];
            if (!tx2 || tx2->name[0] != '+')
                continue;
            if (strcmp(tx2->name + 2, tx->name + 2))
                continue;

            num = tx2->name[1];
            if (num >= 'a' && num <= 'z')
                num -= 'a' - 'A';
            if (num >= '0' && num <= '9')
            {
                num -= '0';
                anims[num] = tx2;
                if (num + 1 > max)
                    max = num + 1;
            }
            else if (num >= 'A' && num <= 'J')
            {
                num = num - 'A';
                altanims[num] = tx2;
                if (num + 1 > altmax)
                    altmax = num + 1;
            }
            else
                Sys_Error("Bad animating texture %s", tx->name);
        }

#define ANIM_CYCLE 2
        // link them all together
        for (j = 0; j < max; j++)
        {
            tx2 = anims[j];
            if (!tx2)
                Sys_Error("Missing frame %i of %s", j, tx->name);
            tx2->anim_total = max * ANIM_CYCLE;
            tx2->anim_min = j * ANIM_CYCLE;
            tx2->anim_max = (j + 1) * ANIM_CYCLE;
            tx2->anim_next = anims[(j + 1) % max];
            if (altmax)
                tx2->alternate_anims = altanims[0];
        }
        for (j = 0; j < altmax; j++)
        {
            tx2 = altanims[j];
            if (!tx2)
                Sys_Error("Missing frame %i of %s", j, tx->name);
            tx2->anim_total = altmax * ANIM_CYCLE;
            tx2->anim_min = j * ANIM_CYCLE;
            tx2->anim_max = (j + 1) * ANIM_CYCLE;
            tx2->anim_next = altanims[(j + 1) % altmax];
            if (max)
                tx2->alternate_anims = anims[0];
        }
    }
}

/*
=================
Mod_LoadLighting
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadLighting (lump_t *l)
void Mod_LoadLighting(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    if (!l->filelen)
    {
        loadmodel->lightdata = NULL;
        return;
    }
    loadmodel->lightdata = Hunk_AllocName(l->filelen, loadname);
    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    Q_memcpy(loadmodel->lightdata, fpdata->buf, l->filelen);
    // <<< FIX
}

/*
=================
Mod_LoadVisibility
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadVisibility (lump_t *l)
void Mod_LoadVisibility(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    if (!l->filelen)
    {
        loadmodel->visdata = NULL;
        return;
    }
    loadmodel->visdata = Hunk_AllocName(l->filelen, loadname);
    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // memcpy (loadmodel->visdata, mod_base + l->fileofs, l->filelen);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    Q_memcpy(loadmodel->visdata, fpdata->buf, l->filelen);
    // <<< FIX
}

/*
=================
Mod_LoadEntities
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadEntities (lump_t *l)
void Mod_LoadEntities(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    if (!l->filelen)
    {
        loadmodel->entities = NULL;
        return;
    }
    loadmodel->entities = Hunk_AllocName(l->filelen, loadname);
    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // memcpy (loadmodel->entities, mod_base + l->fileofs, l->filelen);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    Q_memcpy(loadmodel->entities, fpdata->buf, l->filelen);
    // <<< FIX
}

/*
=================
Mod_LoadVertexes
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadVertexes (lump_t *l)
void Mod_LoadVertexes(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    dvertex_t *in;
    mvertex_t *out;
    int i, count;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadmodel->vertexes = out;
    loadmodel->numvertexes = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        out->position[0] = LittleFloat(in->point[0]);
        out->position[1] = LittleFloat(in->point[1]);
        out->position[2] = LittleFloat(in->point[2]);
    }
}

/*
=================
Mod_LoadSubmodels
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadSubmodels (lump_t *l)
void Mod_LoadSubmodels(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    dmodel_t *in;
    dmodel_t *out;
    int i, j, count;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName ( count*sizeof(*out), loadname);

    loadmodel->submodels = out;
    loadmodel->numsubmodels = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        for (j = 0; j < 3; j++)
        { // spread the mins / maxs by a pixel
            out->mins[j] = LittleFloat(in->mins[j]) - 1;
            out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
            out->origin[j] = LittleFloat(in->origin[j]);
        }
        for (j = 0; j < MAX_MAP_HULLS; j++)
            out->headnode[j] = LittleLong(in->headnode[j]);
        out->visleafs = LittleLong(in->visleafs);
        out->firstface = LittleLong(in->firstface);
        out->numfaces = LittleLong(in->numfaces);
    }
}

/*
=================
Mod_LoadEdges
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadEdges (lump_t *l)
void Mod_LoadEdges(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    dedge_t *in;
    medge_t *out;
    int i, count;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName ( (count + 1) * sizeof(*out), loadname);

    loadmodel->edges = out;
    loadmodel->numedges = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        out->v[0] = (unsigned short)LittleShort(in->v[0]);
        out->v[1] = (unsigned short)LittleShort(in->v[1]);
    }
}

/*
=================
Mod_LoadTexinfo
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadTexinfo (lump_t *l)
void Mod_LoadTexinfo(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    texinfo_t *in;
    mtexinfo_t *out;
    int i, j, count;
    int miptex;
    float len1, len2;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadmodel->texinfo = out;
    loadmodel->numtexinfo = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        for (j = 0; j < 8; j++)
            out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
        len1 = Length(out->vecs[0]);
        len2 = Length(out->vecs[1]);
        len1 = (len1 + len2) / 2;
        if (len1 < 0.32)
            out->mipadjust = 4;
        else if (len1 < 0.49)
            out->mipadjust = 3;
        else if (len1 < 0.99)
            out->mipadjust = 2;
        else
            out->mipadjust = 1;
#if 0
		if (len1 + len2 < 0.001)
			out->mipadjust = 1;		// don't crash
		else
			out->mipadjust = 1 / floor( (len1+len2)/2 + 0.1 );
#endif

        miptex = LittleLong(in->miptex);
        out->flags = LittleLong(in->flags);

        if (!loadmodel->textures)
        {
            out->texture = r_notexture_mip; // checkerboard texture
            out->flags = 0;
        }
        else
        {
            if (miptex >= loadmodel->numtextures)
                Sys_Error("miptex >= loadmodel->numtextures");
            out->texture = loadmodel->textures[miptex];
            if (!out->texture)
            {
                out->texture = r_notexture_mip; // texture not found
                out->flags = 0;
            }
        }
    }
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents(msurface_t *s)
{
    float mins[2], maxs[2], val;
    int i, j, e;
    mvertex_t *v;
    mtexinfo_t *tex;
    int bmins[2], bmaxs[2];

    mins[0] = mins[1] = 999999;
    maxs[0] = maxs[1] = -99999;

    tex = s->texinfo;

    for (i = 0; i < s->numedges; i++)
    {
        e = loadmodel->surfedges[s->firstedge + i];
        if (e >= 0)
            v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
        else
            v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

        for (j = 0; j < 2; j++)
        {
            val = v->position[0] * tex->vecs[j][0] +
                  v->position[1] * tex->vecs[j][1] +
                  v->position[2] * tex->vecs[j][2] + tex->vecs[j][3];
            if (val < mins[j])
                mins[j] = val;
            if (val > maxs[j])
                maxs[j] = val;
        }
    }

    for (i = 0; i < 2; i++)
    {
        bmins[i] = floor(mins[i] / 16);
        bmaxs[i] = ceil(maxs[i] / 16);

        s->texturemins[i] = bmins[i] * 16;
        s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
        if (!(tex->flags & TEX_SPECIAL) && s->extents[i] > 256)
            Sys_Error("Bad surface extents");
    }
}

/*
=================
Mod_LoadFaces
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadFaces (lump_t *l)
void Mod_LoadFaces(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    dface_t *in;
    msurface_t *out;
    int i, count, surfnum;
    int planenum, side;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadmodel->surfaces = out;
    loadmodel->numsurfaces = count;

    for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
    {
        out->firstedge = LittleLong(in->firstedge);
        out->numedges = LittleShort(in->numedges);
        out->flags = 0;

        planenum = LittleShort(in->planenum);
        side = LittleShort(in->side);
        if (side)
            out->flags |= SURF_PLANEBACK;

        out->plane = loadmodel->planes + planenum;

        out->texinfo = loadmodel->texinfo + LittleShort(in->texinfo);

        CalcSurfaceExtents(out);

        // lighting info

        for (i = 0; i < MAXLIGHTMAPS; i++)
            out->styles[i] = in->styles[i];
        i = LittleLong(in->lightofs);
        if (i == -1)
            out->samples = NULL;
        else
            out->samples = loadmodel->lightdata + i;

        // set the drawing flags flag

        if (!Q_strncmp(out->texinfo->texture->name, "sky", 3)) // sky
        {
            out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
            continue;
        }

        if (!Q_strncmp(out->texinfo->texture->name, "*", 1)) // turbulent
        {
            out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
            for (i = 0; i < 2; i++)
            {
                out->extents[i] = 16384;
                out->texturemins[i] = -8192;
            }
            continue;
        }
    }
}

/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent(mnode_t *node, mnode_t *parent)
{
    node->parent = parent;
    if (node->contents < 0)
        return;
    Mod_SetParent(node->children[0], node);
    Mod_SetParent(node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadNodes (lump_t *l)
void Mod_LoadNodes(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    int i, j, count, p;
    dnode_t *in;
    mnode_t *out;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadmodel->nodes = out;
    loadmodel->numnodes = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        for (j = 0; j < 3; j++)
        {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        p = LittleLong(in->planenum);
        out->plane = loadmodel->planes + p;

        out->firstsurface = LittleShort(in->firstface);
        out->numsurfaces = LittleShort(in->numfaces);

        for (j = 0; j < 2; j++)
        {
            p = LittleShort(in->children[j]);
            if (p >= 0)
                out->children[j] = loadmodel->nodes + p;
            else
                out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
        }
    }

    Mod_SetParent(loadmodel->nodes, NULL); // sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadLeafs (lump_t *l)
void Mod_LoadLeafs(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    dleaf_t *in;
    mleaf_t *out;
    int i, j, count, p;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadmodel->leafs = out;
    loadmodel->numleafs = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        for (j = 0; j < 3; j++)
        {
            out->minmaxs[j] = LittleShort(in->mins[j]);
            out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
        }

        p = LittleLong(in->contents);
        out->contents = p;

        out->firstmarksurface =
            loadmodel->marksurfaces + LittleShort(in->firstmarksurface);
        out->nummarksurfaces = LittleShort(in->nummarksurfaces);

        p = LittleLong(in->visofs);
        if (p == -1)
            out->compressed_vis = NULL;
        else
            out->compressed_vis = loadmodel->visdata + p;
        out->efrags = NULL;

        for (j = 0; j < 4; j++)
            out->ambient_sound_level[j] = in->ambient_level[j];
    }
}

/*
=================
Mod_LoadClipnodes
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadClipnodes (lump_t *l)
void Mod_LoadClipnodes(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    dclipnode_t *in, *out;
    int i, count;
    hull_t *hull;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadmodel->clipnodes = out;
    loadmodel->numclipnodes = count;

    hull = &loadmodel->hulls[1];
    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = loadmodel->planes;
    hull->clip_mins[0] = -16;
    hull->clip_mins[1] = -16;
    hull->clip_mins[2] = -24;
    hull->clip_maxs[0] = 16;
    hull->clip_maxs[1] = 16;
    hull->clip_maxs[2] = 32;

    hull = &loadmodel->hulls[2];
    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = loadmodel->planes;
    hull->clip_mins[0] = -32;
    hull->clip_mins[1] = -32;
    hull->clip_mins[2] = -24;
    hull->clip_maxs[0] = 32;
    hull->clip_maxs[1] = 32;
    hull->clip_maxs[2] = 64;

    for (i = 0; i < count; i++, out++, in++)
    {
        out->planenum = LittleLong(in->planenum);
        out->children[0] = LittleShort(in->children[0]);
        out->children[1] = LittleShort(in->children[1]);
    }
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0(void)
{
    mnode_t *in, *child;
    dclipnode_t *out;
    int i, j, count;
    hull_t *hull;

    hull = &loadmodel->hulls[0];

    in = loadmodel->nodes;
    count = loadmodel->numnodes;
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = loadmodel->planes;

    for (i = 0; i < count; i++, out++, in++)
    {
        out->planenum = in->plane - loadmodel->planes;
        for (j = 0; j < 2; j++)
        {
            child = in->children[j];
            if (child->contents < 0)
                out->children[j] = child->contents;
            else
                out->children[j] = child - loadmodel->nodes;
        }
    }
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadMarksurfaces (lump_t *l)
void Mod_LoadMarksurfaces(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    int i, j, count;
    short *in;
    msurface_t **out;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadmodel->marksurfaces = out;
    loadmodel->nummarksurfaces = count;

    for (i = 0; i < count; i++)
    {
        j = LittleShort(in[i]);
        if (j >= loadmodel->numsurfaces)
            Sys_Error("Mod_ParseMarksurfaces: bad surface number");
        out[i] = loadmodel->surfaces + j;
    }
}

/*
=================
Mod_LoadSurfedges
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadSurfedges (lump_t *l)
void Mod_LoadSurfedges(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    int i, count;
    int *in, *out;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * sizeof(*out), loadname);

    loadmodel->surfedges = out;
    loadmodel->numsurfedges = count;

    for (i = 0; i < count; i++)
        out[i] = LittleLong(in[i]);
}

/*
=================
Mod_LoadPlanes
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Adding new parameter so it can load only what it needs, instead of relying on
// mod_base containing the whole file:
// void Mod_LoadPlanes (lump_t *l)
void Mod_LoadPlanes(lump_t *l, filepartdata_t *fpdata)
// <<< FIX
{
    int i, j;
    mplane_t *out;
    dplane_t *in;
    int count;
    int bits;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the required data, instead of relying on the contents of
    // mod_base:
    // in = (void *)(mod_base + l->fileofs);
    fpdata->offset = l->fileofs;
    fpdata->length = l->filelen;
    COM_LoadStackFilePartial(fpdata, false);
    in = (void *)(fpdata->buf);
    // <<< FIX
    if (l->filelen % sizeof(*in))
        Sys_Error("MOD_LoadBmodel: funny lump size in %s", loadmodel->name);
    count = l->filelen / sizeof(*in);
    out = Hunk_AllocName(count * 2 * sizeof(*out), loadname);

    loadmodel->planes = out;
    loadmodel->numplanes = count;

    for (i = 0; i < count; i++, in++, out++)
    {
        bits = 0;
        for (j = 0; j < 3; j++)
        {
            out->normal[j] = LittleFloat(in->normal[j]);
            if (out->normal[j] < 0)
                bits |= 1 << j;
        }

        out->dist = LittleFloat(in->dist);
        out->type = LittleLong(in->type);
        out->signbits = bits;
    }
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs)
{
    int i;
    vec3_t corner;

    for (i = 0; i < 3; i++)
    {
        corner[i] =
            fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
    }

    return Length(corner);
}

/*
=================
Mod_LoadBrushModel
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Redeclaring so it receives a filepartdata_t instead of the whole buffer:
// void Mod_LoadBrushModel (model_t *mod, void *buffer)
void Mod_LoadBrushModel(model_t *mod, filepartdata_t *fpdata)
// <<< FIX
{
    int i, j;
    dheader_t *header;
    dmodel_t *bm;

    loadmodel->type = mod_brush;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading only the header from the source file:
    // header = (dheader_t *)buffer;
    fpdata->offset = 0;
    fpdata->length = sizeof(dheader_t);
    COM_LoadStackFilePartial(fpdata, false);
    header = Sys_Malloc(sizeof(dheader_t), "Mod_LoadBrushModel");
    Q_memcpy(header, fpdata->buf, sizeof(dheader_t));
    // <<< FIX

    i = LittleLong(header->version);
    if (i != BSPVERSION)
        Sys_Error(
            "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)",
            mod->name, i, BSPVERSION);

    // swap all the lumps
    mod_base = (byte *)header;

    for (i = 0; i < sizeof(dheader_t) / 4; i++)
        ((int *)header)[i] = LittleLong(((int *)header)[i]);

    // load into heap

    // >>> FIX: For Nintendo DS using devkitARM
    // Sending the file info to all loaders, instead of relying on mod_base:
    // Mod_LoadVertexes (&header->lumps[LUMP_VERTEXES]);
    // Mod_LoadEdges (&header->lumps[LUMP_EDGES]);
    // Mod_LoadSurfedges (&header->lumps[LUMP_SURFEDGES]);
    // Mod_LoadTextures (&header->lumps[LUMP_TEXTURES]);
    // Mod_LoadLighting (&header->lumps[LUMP_LIGHTING]);
    // Mod_LoadPlanes (&header->lumps[LUMP_PLANES]);
    // Mod_LoadTexinfo (&header->lumps[LUMP_TEXINFO]);
    // Mod_LoadFaces (&header->lumps[LUMP_FACES]);
    // Mod_LoadMarksurfaces (&header->lumps[LUMP_MARKSURFACES]);
    // Mod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
    // Mod_LoadLeafs (&header->lumps[LUMP_LEAFS]);
    // Mod_LoadNodes (&header->lumps[LUMP_NODES]);
    // Mod_LoadClipnodes (&header->lumps[LUMP_CLIPNODES]);
    // Mod_LoadEntities (&header->lumps[LUMP_ENTITIES]);
    // Mod_LoadSubmodels (&header->lumps[LUMP_MODELS]);
    Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES], fpdata);
    Mod_LoadEdges(&header->lumps[LUMP_EDGES], fpdata);
    Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES], fpdata);
    Mod_LoadTextures(&header->lumps[LUMP_TEXTURES], fpdata);
    Mod_LoadLighting(&header->lumps[LUMP_LIGHTING], fpdata);
    Mod_LoadPlanes(&header->lumps[LUMP_PLANES], fpdata);
    Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO], fpdata);
    Mod_LoadFaces(&header->lumps[LUMP_FACES], fpdata);
    Mod_LoadMarksurfaces(&header->lumps[LUMP_MARKSURFACES], fpdata);
    Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY], fpdata);
    Mod_LoadLeafs(&header->lumps[LUMP_LEAFS], fpdata);
    Mod_LoadNodes(&header->lumps[LUMP_NODES], fpdata);
    Mod_LoadClipnodes(&header->lumps[LUMP_CLIPNODES], fpdata);
    Mod_LoadEntities(&header->lumps[LUMP_ENTITIES], fpdata);
    Mod_LoadSubmodels(&header->lumps[LUMP_MODELS], fpdata);
    // <<< FIX

    Mod_MakeHull0();

    mod->numframes = 2; // regular and alternate animation
    mod->flags = 0;

    //
    // set up the submodels (FIXME: this is confusing)
    //
    for (i = 0; i < mod->numsubmodels; i++)
    {
        bm = &mod->submodels[i];

        mod->hulls[0].firstclipnode = bm->headnode[0];
        for (j = 1; j < MAX_MAP_HULLS; j++)
        {
            mod->hulls[j].firstclipnode = bm->headnode[j];
            mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
        }

        mod->firstmodelsurface = bm->firstface;
        mod->nummodelsurfaces = bm->numfaces;

        VectorCopy(bm->maxs, mod->maxs);
        VectorCopy(bm->mins, mod->mins);
        mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

        mod->numleafs = bm->visleafs;

        if (i < mod->numsubmodels - 1)
        { // duplicate the basic information
            char name[10];

            sprintf(name, "*%i", i + 1);
            loadmodel = Mod_FindName(name);
            *loadmodel = *mod;
            strcpy(loadmodel->name, name);
            mod = loadmodel;
        }
    }

    // >>> FIX: For Nintendo DS using devkitARM
    // Deallocating header:
    free(header);
    // <<< FIX
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/*
=================
Mod_LoadAliasFrame
=================
*/
void *Mod_LoadAliasFrame(void *pin, int *pframeindex, int numv,
                         trivertx_t *pbboxmin, trivertx_t *pbboxmax,
                         aliashdr_t *pheader, char *name)
{
    trivertx_t *pframe, *pinframe;
    int i, j;
    daliasframe_t *pdaliasframe;

    pdaliasframe = (daliasframe_t *)pin;

    strcpy(name, pdaliasframe->name);

    for (i = 0; i < 3; i++)
    {
        // these are byte values, so we don't have to worry about
        // endianness
        pbboxmin->v[i] = pdaliasframe->bboxmin.v[i];
        pbboxmax->v[i] = pdaliasframe->bboxmax.v[i];
    }

    pinframe = (trivertx_t *)(pdaliasframe + 1);
    pframe = Hunk_AllocName(numv * sizeof(*pframe), loadname);

    *pframeindex = (byte *)pframe - (byte *)pheader;

    for (j = 0; j < numv; j++)
    {
        int k;

        // these are all byte values, so no need to deal with endianness
        pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

        for (k = 0; k < 3; k++)
        {
            pframe[j].v[k] = pinframe[j].v[k];
        }
    }

    pinframe += numv;

    return (void *)pinframe;
}

/*
=================
Mod_LoadAliasGroup
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Redeclaring so it receives a filepartdata_t instead of the whole buffer:
// void * Mod_LoadAliasGroup (void * pin, int *pframeindex, int numv,
//	trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name)
void *Mod_LoadAliasGroup(filepartdata_t *fpdata, qboolean islastframe,
                         int *pframeindex, int numv, trivertx_t *pbboxmin,
                         trivertx_t *pbboxmax, aliashdr_t *pheader, char *name)
// <<< FIX
{
    daliasgroup_t *pingroup;
    maliasgroup_t *paliasgroup;
    int i, numframes;
    daliasinterval_t *pin_intervals;
    float *poutintervals;
    void *ptemp;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading header only:
    // pingroup = (daliasgroup_t *)pin;
    fpdata->offset = fpdata->offset + fpdata->length;
    fpdata->length = sizeof(daliasgroup_t);
    COM_LoadStackFilePartial(fpdata, false);
    pingroup = ((daliasgroup_t *)(fpdata->buf));
    // <<< FIX

    numframes = LittleLong(pingroup->numframes);

    paliasgroup =
        Hunk_AllocName(sizeof(maliasgroup_t) +
                           (numframes - 1) * sizeof(paliasgroup->frames[0]),
                       loadname);

    paliasgroup->numframes = numframes;

    for (i = 0; i < 3; i++)
    {
        // these are byte values, so we don't have to worry about endianness
        pbboxmin->v[i] = pingroup->bboxmin.v[i];
        pbboxmax->v[i] = pingroup->bboxmax.v[i];
    }

    *pframeindex = (byte *)paliasgroup - (byte *)pheader;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading interval array only:
    // pin_intervals = (daliasinterval_t *)(pingroup + 1);
    fpdata->offset = fpdata->offset + fpdata->length;
    fpdata->length = numframes * sizeof(float);
    COM_LoadStackFilePartial(fpdata, false);
    pin_intervals = ((daliasinterval_t *)(fpdata->buf));
    // <<< FIX

    poutintervals = Hunk_AllocName(numframes * sizeof(float), loadname);

    paliasgroup->intervals = (byte *)poutintervals - (byte *)pheader;

    for (i = 0; i < numframes; i++)
    {
        *poutintervals = LittleFloat(pin_intervals->interval);
        if (*poutintervals <= 0.0)
            Sys_Error("Mod_LoadAliasGroup: interval<=0");

        poutintervals++;
        pin_intervals++;
    }

    // >>> FIX: For Nintendo DS using devkitARM
    // This is no longer done here since the frame data is being loaded in
    // pieces:
    // ptemp = (void *)pin_intervals;
    // <<< FIX

    for (i = 0; i < numframes; i++)
    {
        // >>> FIX: For Nintendo DS using devkitARM
        // Loading frame data (and, if corresponds, the type of the next frame)
        // only:
        fpdata->offset = fpdata->offset + fpdata->length;
        if ((i < (numframes - 1)) || (islastframe))
        {
            fpdata->length = sizeof(daliasframe_t) + numv * sizeof(trivertx_t);
        }
        else
        {
            fpdata->length = sizeof(daliasframe_t) + numv * sizeof(trivertx_t) +
                             sizeof(daliasframetype_t);
        };
        COM_LoadStackFilePartial(fpdata, false);
        ptemp = fpdata->buf;
        // <<< FIX
        ptemp =
            Mod_LoadAliasFrame(ptemp, &paliasgroup->frames[i].frame, numv,
                               &paliasgroup->frames[i].bboxmin,
                               &paliasgroup->frames[i].bboxmax, pheader, name);
    }

    return ptemp;
}

/*
=================
Mod_LoadAliasSkin
=================
*/
void *Mod_LoadAliasSkin(void *pin, int *pskinindex, int skinsize,
                        aliashdr_t *pheader)
{
    int i;
    byte *pskin, *pinskin;
    unsigned short *pusskin;

    pskin = Hunk_AllocName(skinsize * r_pixbytes, loadname);
    pinskin = (byte *)pin;
    *pskinindex = (byte *)pskin - (byte *)pheader;

    if (r_pixbytes == 1)
    {
        Q_memcpy(pskin, pinskin, skinsize);
    }
    else if (r_pixbytes == 2)
    {
        pusskin = (unsigned short *)pskin;

        for (i = 0; i < skinsize; i++)
            pusskin[i] = d_8to16table[pinskin[i]];
    }
    else
    {
        Sys_Error("Mod_LoadAliasSkin: driver set invalid r_pixbytes: %d\n",
                  r_pixbytes);
    }

    pinskin += skinsize;

    return ((void *)pinskin);
}

/*
=================
Mod_LoadAliasSkinGroup
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Redeclaring so it receives a filepartdata_t instead of the whole buffer:
// void * Mod_LoadAliasSkinGroup (void * pin, int *pskinindex, int skinsize,
//	aliashdr_t *pheader)
void *Mod_LoadAliasSkinGroup(filepartdata_t *fpdata, qboolean islastskin,
                             int *pskinindex, int skinsize, aliashdr_t *pheader)
// <<< FIX
{
    daliasskingroup_t *pinskingroup;
    maliasskingroup_t *paliasskingroup;
    int i, numskins;
    daliasskininterval_t *pinskinintervals;
    float *poutskinintervals;
    void *ptemp;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading numskins only:
    // pinskingroup = (daliasskingroup_t *)pin;
    fpdata->offset = fpdata->offset + fpdata->length;
    fpdata->length = sizeof(numskins);
    COM_LoadStackFilePartial(fpdata, false);
    pinskingroup = ((daliasskingroup_t *)(fpdata->buf));
    // <<< FIX

    numskins = LittleLong(pinskingroup->numskins);

    paliasskingroup = Hunk_AllocName(
        sizeof(maliasskingroup_t) +
            (numskins - 1) * sizeof(paliasskingroup->skindescs[0]),
        loadname);

    paliasskingroup->numskins = numskins;

    *pskinindex = (byte *)paliasskingroup - (byte *)pheader;

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading interval array only:
    // pinskinintervals = (daliasskininterval_t *)(pinskingroup + 1);
    fpdata->offset = fpdata->offset + fpdata->length;
    fpdata->length = numskins * sizeof(float);
    COM_LoadStackFilePartial(fpdata, false);
    pinskinintervals = ((daliasskininterval_t *)(fpdata->buf));
    // <<< FIX

    poutskinintervals = Hunk_AllocName(numskins * sizeof(float), loadname);

    paliasskingroup->intervals = (byte *)poutskinintervals - (byte *)pheader;

    for (i = 0; i < numskins; i++)
    {
        *poutskinintervals = LittleFloat(pinskinintervals->interval);
        if (*poutskinintervals <= 0)
            Sys_Error("Mod_LoadAliasSkinGroup: interval<=0");

        poutskinintervals++;
        pinskinintervals++;
    }

    // >>> FIX: For Nintendo DS using devkitARM
    // This is no longer done here since the skin data is being loaded in
    // pieces:
    // ptemp = (void *)pinskinintervals;
    // <<< FIX

    for (i = 0; i < numskins; i++)
    {
        // >>> FIX: For Nintendo DS using devkitARM
        // Loading skin data (and, if corresponds, the type of the next skin)
        // only:
        fpdata->offset = fpdata->offset + fpdata->length;
        if ((i < (numskins - 1)) || (islastskin))
        {
            fpdata->length = skinsize;
        }
        else
        {
            fpdata->length = skinsize + sizeof(daliasskintype_t);
        };
        COM_LoadStackFilePartial(fpdata, false);
        ptemp = fpdata->buf;
        // <<< FIX
        ptemp = Mod_LoadAliasSkin(ptemp, &paliasskingroup->skindescs[i].skin,
                                  skinsize, pheader);
    }

    return ptemp;
}

/*
=================
Mod_LoadAliasModel
=================
*/
// >>> FIX: For Nintendo DS using devkitARM
// Redeclaring so it receives a filepartdata_t instead of the whole buffer:
// void Mod_LoadAliasModel (model_t *mod, void *buffer)
void Mod_LoadAliasModel(model_t *mod, filepartdata_t *fpdata)
// <<< FIX
{
    int i;
    mdl_t *pmodel, *pinmodel;
    stvert_t *pstverts, *pinstverts;
    aliashdr_t *pheader;
    mtriangle_t *ptri;
    dtriangle_t *pintriangles;
    int version, numframes, numskins;
    int size;
    daliasframetype_t *pframetype;
    daliasskintype_t *pskintype;
    maliasskindesc_t *pskindesc;
    int skinsize;
    int start, end, total;

    start = Hunk_LowMark();

    // >>> FIX: For Nintendo DS using devkitARM
    // Loading header (and type of the first skin) only:
    // pinmodel = (mdl_t *)buffer;
    fpdata->offset = 0;
    fpdata->length = sizeof(*pinmodel) + sizeof(daliasskintype_t);
    COM_LoadStackFilePartial(fpdata, false);
    pinmodel = Sys_Malloc(sizeof(*pinmodel) + sizeof(daliasskintype_t),
                          "Mod_LoadAliasModel");
    Q_memcpy(pinmodel, fpdata->buf,
             sizeof(*pinmodel) + sizeof(daliasskintype_t));
    // <<< FIX

    version = LittleLong(pinmodel->version);
    if (version != ALIAS_VERSION)
        Sys_Error("%s has wrong version number (%i should be %i)", mod->name,
                  version, ALIAS_VERSION);

    //
    // allocate space for a working header, plus all the data except the frames,
    // skin and group info
    //
    size = sizeof(aliashdr_t) +
           (LittleLong(pinmodel->numframes) - 1) * sizeof(pheader->frames[0]) +
           sizeof(mdl_t) + LittleLong(pinmodel->numverts) * sizeof(stvert_t) +
           LittleLong(pinmodel->numtris) * sizeof(mtriangle_t);

    pheader = Hunk_AllocName(size, loadname);
    pmodel =
        (mdl_t *)((byte *)&pheader[1] + (LittleLong(pinmodel->numframes) - 1) *
                                            sizeof(pheader->frames[0]));

    //	mod->cache.data = pheader;
    mod->flags = LittleLong(pinmodel->flags);

    //
    // endian-adjust and copy the data, starting with the alias model header
    //
    pmodel->boundingradius = LittleFloat(pinmodel->boundingradius);
    pmodel->numskins = LittleLong(pinmodel->numskins);
    pmodel->skinwidth = LittleLong(pinmodel->skinwidth);
    pmodel->skinheight = LittleLong(pinmodel->skinheight);

    if (pmodel->skinheight > MAX_LBM_HEIGHT)
        Sys_Error("model %s has a skin taller than %d", mod->name,
                  MAX_LBM_HEIGHT);

    pmodel->numverts = LittleLong(pinmodel->numverts);

    if (pmodel->numverts <= 0)
        Sys_Error("model %s has no vertices", mod->name);

    if (pmodel->numverts > MAXALIASVERTS)
        Sys_Error("model %s has too many vertices", mod->name);

    pmodel->numtris = LittleLong(pinmodel->numtris);

    if (pmodel->numtris <= 0)
        Sys_Error("model %s has no triangles", mod->name);

    pmodel->numframes = LittleLong(pinmodel->numframes);
    pmodel->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
    mod->synctype = LittleLong(pinmodel->synctype);
    mod->numframes = pmodel->numframes;

    for (i = 0; i < 3; i++)
    {
        pmodel->scale[i] = LittleFloat(pinmodel->scale[i]);
        pmodel->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
        pmodel->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
    }

    numskins = pmodel->numskins;
    numframes = pmodel->numframes;

    if (pmodel->skinwidth & 0x03)
        Sys_Error("Mod_LoadAliasModel: skinwidth not multiple of 4");

    pheader->model = (byte *)pmodel - (byte *)pheader;

    //
    // load the skins
    //
    skinsize = pmodel->skinheight * pmodel->skinwidth;

    if (numskins < 1)
        Sys_Error("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

    pskintype = (daliasskintype_t *)&pinmodel[1];

    pskindesc = Hunk_AllocName(numskins * sizeof(maliasskindesc_t), loadname);

    pheader->skindesc = (byte *)pskindesc - (byte *)pheader;

    for (i = 0; i < numskins; i++)
    {
        aliasskintype_t skintype;

        skintype = LittleLong(pskintype->type);
        pskindesc[i].type = skintype;

        if (skintype == ALIAS_SKIN_SINGLE)
        {
            // >>> FIX: For Nintendo DS using devkitARM
            // Loading skin (and the type of the next skin) only:
            fpdata->offset =
                fpdata->offset + fpdata->length - sizeof(daliasskintype_t);
            if (i < (numskins - 1))
            {
                fpdata->length = sizeof(daliasskintype_t) + skinsize +
                                 sizeof(daliasskintype_t);
            }
            else
            {
                fpdata->length = sizeof(daliasskintype_t) + skinsize;
            };
            COM_LoadStackFilePartial(fpdata, false);
            pskintype = ((daliasskintype_t *)(fpdata->buf));
            // <<< FIX
            pskintype = (daliasskintype_t *)Mod_LoadAliasSkin(
                pskintype + 1, &pskindesc[i].skin, skinsize, pheader);
        }
        else
        {
            pskintype = (daliasskintype_t *)
                // >>> FIX: For Nintendo DS using devkitARM
                // Sending fpdata to the next call (instead of the whole
                // buffer):
                // Mod_LoadAliasSkinGroup (pskintype + 1,
                Mod_LoadAliasSkinGroup(fpdata, (i >= (numskins - 1)),
                                       // <<< FIX
                                       &pskindesc[i].skin, skinsize, pheader);
        }
    }

    //
    // set base s and t vertices
    //
    pstverts = (stvert_t *)&pmodel[1];
    // >>> FIX: For Nintendo DS using devkitARM
    // Loading texcoords only:
    // pinstverts = (stvert_t *)pskintype;
    fpdata->offset = fpdata->offset + fpdata->length;
    fpdata->length = pmodel->numverts * sizeof(stvert_t);
    COM_LoadStackFilePartial(fpdata, false);
    pinstverts = ((stvert_t *)(fpdata->buf));
    // <<< FIX

    pheader->stverts = (byte *)pstverts - (byte *)pheader;

    for (i = 0; i < pmodel->numverts; i++)
    {
        pstverts[i].onseam = LittleLong(pinstverts[i].onseam);
        // put s and t in 16.16 format
        pstverts[i].s = LittleLong(pinstverts[i].s) << 16;
        pstverts[i].t = LittleLong(pinstverts[i].t) << 16;
    }

    //
    // set up the triangles
    //
    ptri = (mtriangle_t *)&pstverts[pmodel->numverts];
    // >>> FIX: For Nintendo DS using devkitARM
    // Loading triangles (and type of the first frame) only:
    // pintriangles = (dtriangle_t *)&pinstverts[pmodel->numverts];
    fpdata->offset = fpdata->offset + fpdata->length;
    fpdata->length =
        pmodel->numtris * sizeof(mtriangle_t) + sizeof(daliasframetype_t);
    COM_LoadStackFilePartial(fpdata, false);
    pintriangles = ((dtriangle_t *)(fpdata->buf));
    // <<< FIX

    pheader->triangles = (byte *)ptri - (byte *)pheader;

    for (i = 0; i < pmodel->numtris; i++)
    {
        int j;

        ptri[i].facesfront = LittleLong(pintriangles[i].facesfront);

        for (j = 0; j < 3; j++)
        {
            ptri[i].vertindex[j] = LittleLong(pintriangles[i].vertindex[j]);
        }
    }

    //
    // load the frames
    //
    if (numframes < 1)
        Sys_Error("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

    pframetype = (daliasframetype_t *)&pintriangles[pmodel->numtris];

    for (i = 0; i < numframes; i++)
    {
        aliasframetype_t frametype;

        frametype = LittleLong(pframetype->type);
        pheader->frames[i].type = frametype;

        if (frametype == ALIAS_SINGLE)
        {
            // >>> FIX: For Nintendo DS using devkitARM
            // Loading frame (and the type of the next frame) only:
            fpdata->offset =
                fpdata->offset + fpdata->length - sizeof(daliasframetype_t);
            if (i < (numframes - 1))
            {
                fpdata->length = sizeof(daliasframetype_t) +
                                 sizeof(daliasframe_t) +
                                 pmodel->numverts * sizeof(trivertx_t) +
                                 sizeof(daliasframetype_t);
            }
            else
            {
                fpdata->length = sizeof(daliasframetype_t) +
                                 sizeof(daliasframe_t) +
                                 pmodel->numverts * sizeof(trivertx_t);
            };
            COM_LoadStackFilePartial(fpdata, false);
            pframetype = ((daliasframetype_t *)(fpdata->buf));
            // <<< FIX
            pframetype = (daliasframetype_t *)Mod_LoadAliasFrame(
                pframetype + 1, &pheader->frames[i].frame, pmodel->numverts,
                &pheader->frames[i].bboxmin, &pheader->frames[i].bboxmax,
                pheader, pheader->frames[i].name);
        }
        else
        {
            pframetype = (daliasframetype_t *)
                // >>> FIX: For Nintendo DS using devkitARM
                // Sending fpdata to the next call (instead of the whole
                // buffer):
                // Mod_LoadAliasGroup (pframetype + 1,
                Mod_LoadAliasGroup(fpdata, (i >= (numframes - 1)),
                                   // <<< FIX
                                   &pheader->frames[i].frame, pmodel->numverts,
                                   &pheader->frames[i].bboxmin,
                                   &pheader->frames[i].bboxmax, pheader,
                                   pheader->frames[i].name);
        }
    }

    mod->type = mod_alias;

    // FIXME: do this right
    mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
    mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;

    //
    // move the complete, relocatable alias model to the cache
    //
    end = Hunk_LowMark();
    total = end - start;

    Cache_Alloc(&mod->cache, total, loadname);
    if (!mod->cache.data)
    // >>> FIX: For Nintendo DS using devkitARM
    // Deallocating header:
    {
        free(pinmodel);
        return;
    }
    // <<< FIX
    memcpy(mod->cache.data, pheader, total);

    Hunk_FreeToLowMark(start);

    // >>> FIX: For Nintendo DS using devkitARM
    // Deallocating header:
    free(pinmodel);
    // <<< FIX
}

//=============================================================================

/*
=================
Mod_LoadSpriteFrame
=================
*/
void *Mod_LoadSpriteFrame(void *pin, mspriteframe_t **ppframe)
{
    dspriteframe_t *pinframe;
    mspriteframe_t *pspriteframe;
    int i, width, height, size, origin[2];
    unsigned short *ppixout;
    byte *ppixin;

    pinframe = (dspriteframe_t *)pin;

    width = LittleLong(pinframe->width);
    height = LittleLong(pinframe->height);
    size = width * height;

    pspriteframe =
        Hunk_AllocName(sizeof(mspriteframe_t) + size * r_pixbytes, loadname);

    Q_memset(pspriteframe, 0, sizeof(mspriteframe_t) + size);
    *ppframe = pspriteframe;

    pspriteframe->width = width;
    pspriteframe->height = height;
    origin[0] = LittleLong(pinframe->origin[0]);
    origin[1] = LittleLong(pinframe->origin[1]);

    pspriteframe->up = origin[1];
    pspriteframe->down = origin[1] - height;
    pspriteframe->left = origin[0];
    pspriteframe->right = width + origin[0];

    if (r_pixbytes == 1)
    {
        Q_memcpy(&pspriteframe->pixels[0], (byte *)(pinframe + 1), size);
    }
    else if (r_pixbytes == 2)
    {
        ppixin = (byte *)(pinframe + 1);
        ppixout = (unsigned short *)&pspriteframe->pixels[0];

        for (i = 0; i < size; i++)
            ppixout[i] = d_8to16table[ppixin[i]];
    }
    else
    {
        Sys_Error("Mod_LoadSpriteFrame: driver set invalid r_pixbytes: %d\n",
                  r_pixbytes);
    }

    return (void *)((byte *)pinframe + sizeof(dspriteframe_t) + size);
}

/*
=================
Mod_LoadSpriteGroup
=================
*/
void *Mod_LoadSpriteGroup(void *pin, mspriteframe_t **ppframe)
{
    dspritegroup_t *pingroup;
    mspritegroup_t *pspritegroup;
    int i, numframes;
    dspriteinterval_t *pin_intervals;
    float *poutintervals;
    void *ptemp;

    pingroup = (dspritegroup_t *)pin;

    numframes = LittleLong(pingroup->numframes);

    pspritegroup =
        Hunk_AllocName(sizeof(mspritegroup_t) +
                           (numframes - 1) * sizeof(pspritegroup->frames[0]),
                       loadname);

    pspritegroup->numframes = numframes;

    *ppframe = (mspriteframe_t *)pspritegroup;

    pin_intervals = (dspriteinterval_t *)(pingroup + 1);

    poutintervals = Hunk_AllocName(numframes * sizeof(float), loadname);

    pspritegroup->intervals = poutintervals;

    for (i = 0; i < numframes; i++)
    {
        *poutintervals = LittleFloat(pin_intervals->interval);
        if (*poutintervals <= 0.0)
            Sys_Error("Mod_LoadSpriteGroup: interval<=0");

        poutintervals++;
        pin_intervals++;
    }

    ptemp = (void *)pin_intervals;

    for (i = 0; i < numframes; i++)
    {
        ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i]);
    }

    return ptemp;
}

/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel(model_t *mod, void *buffer)
{
    int i;
    int version;
    dsprite_t *pin;
    msprite_t *psprite;
    int numframes;
    int size;
    dspriteframetype_t *pframetype;

    pin = (dsprite_t *)buffer;

    version = LittleLong(pin->version);
    if (version != SPRITE_VERSION)
        Sys_Error("%s has wrong version number "
                  "(%i should be %i)",
                  mod->name, version, SPRITE_VERSION);

    numframes = LittleLong(pin->numframes);

    size = sizeof(msprite_t) + (numframes - 1) * sizeof(psprite->frames);

    psprite = Hunk_AllocName(size, loadname);

    mod->cache.data = psprite;

    psprite->type = LittleLong(pin->type);
    psprite->maxwidth = LittleLong(pin->width);
    psprite->maxheight = LittleLong(pin->height);
    psprite->beamlength = LittleFloat(pin->beamlength);
    mod->synctype = LittleLong(pin->synctype);
    psprite->numframes = numframes;

    mod->mins[0] = mod->mins[1] = -psprite->maxwidth / 2;
    mod->maxs[0] = mod->maxs[1] = psprite->maxwidth / 2;
    mod->mins[2] = -psprite->maxheight / 2;
    mod->maxs[2] = psprite->maxheight / 2;

    //
    // load the frames
    //
    if (numframes < 1)
        Sys_Error("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);

    mod->numframes = numframes;
    mod->flags = 0;

    pframetype = (dspriteframetype_t *)(pin + 1);

    for (i = 0; i < numframes; i++)
    {
        spriteframetype_t frametype;

        frametype = LittleLong(pframetype->type);
        psprite->frames[i].type = frametype;

        if (frametype == SPR_SINGLE)
        {
            pframetype = (dspriteframetype_t *)Mod_LoadSpriteFrame(
                pframetype + 1, &psprite->frames[i].frameptr);
        }
        else
        {
            pframetype = (dspriteframetype_t *)Mod_LoadSpriteGroup(
                pframetype + 1, &psprite->frames[i].frameptr);
        }
    }

    mod->type = mod_sprite;
}

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print(void)
{
    int i;
    model_t *mod;

    Con_Printf("Cached models:\n");
    for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
    {
        Con_Printf("%8p : %s", mod->cache.data, mod->name);
        if (mod->needload & NL_UNREFERENCED)
            Con_Printf(" (!R)");
        if (mod->needload & NL_NEEDS_LOADED)
            Con_Printf(" (!P)");
        Con_Printf("\n");
    }
}

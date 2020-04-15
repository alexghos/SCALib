#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <pthread.h>
#include "macro.h"
#include "utils.h"
#include "graph.h"
#include "graph_utils.h"

int NPERTHREAD_V ;
int NPERTHREAD_F ;

Vnode *vnodes;
Fnode *fnodes;

uint32_t cnt_vnodes,cnt_fnodes;
pthread_mutex_t lock_vnodes,lock_fnodes;
uint32_t Nk;
uint32_t mode;
uint32_t *tab;
uint32_t *index_vnodes;
uint32_t *index_fnodes;
void print_vnode(Vnode vnode_all[],uint32_t size){
        for(int j=0;j<size;j++){
        Vnode *vnode = &vnode_all[j];
        printf("ID %d \n",vnode->id);
        printf("Ni %d \n",vnode->Ni);
        printf("Nf %d \n",vnode->Nf);
        printf("Ns %d \n",vnode->Ns);

        uint32_t i =0;
        for(i=0;i<vnode->Ns;i++){
            printf(" %.4f ",vnode->distri_orig[i]);
        }
        printf("\n");
        }
}
void print_fnode(Fnode fnode_all[],uint32_t size){
        for(int j=0;j<size;j++){
        Fnode *fnode = &fnode_all[j];
        printf("ID %d \n",fnode->id);
        printf("li %d \n",fnode->li);
        printf("offset %d \n",fnode->offset);
        printf("func_id %d \n",fnode->func_id);

        uint32_t i =0;
        for(i=0;i<(fnode->li*Nk);i++){
            printf(" %.4f ",fnode->msg[i]);
        }
        printf("\n");
        }
}
void shuffle(uint32_t *tab,uint32_t len){
    uint32_t i,j,r;
    for(i=0;i<len;i++){
        r = rand()%len;
        j = tab[r];
        tab[r] = tab[i];
        tab[i] = j;
    }
}
void* thread_vnodes_reset(void *in){
    uint32_t *lim;
    uint32_t nvnodes,nfnodes;
    uint32_t id,init;
    lim = (uint32_t *)in;
    nvnodes = lim[0];
    nfnodes = lim[1];
    Vnode *vnode;

    pthread_mutex_lock(&lock_vnodes);
    while(cnt_vnodes<nvnodes){
        init = cnt_vnodes;
        cnt_vnodes += NPERTHREAD_V;
        pthread_mutex_unlock(&lock_vnodes);
        for(id=init;(id<(init+NPERTHREAD_V)) && (id<nvnodes);id++){
            vnode = &vnodes[index_vnodes[id]];
            for(int i = 0; i<(vnode->Ni+vnode->Nf);i++){
                memcpy(&vnode->msg[index(i,0,Nk)],vnode->distri_orig,sizeof(proba_t)*Nk);
            }
        }
        pthread_mutex_lock(&lock_vnodes);
    }
    pthread_mutex_unlock(&lock_vnodes);

    pthread_exit(NULL);
    return NULL;
}
void* thread_vnodes(void *in){
    uint32_t *lim;
    uint32_t nvnodes,nfnodes;
    uint32_t id,init;
    lim = (uint32_t *)in;
    nvnodes = lim[0];
    nfnodes = lim[1];
    Vnode *vnode;

    pthread_mutex_lock(&lock_vnodes);
    while(cnt_vnodes<nvnodes){
        init = cnt_vnodes;
        cnt_vnodes += NPERTHREAD_V;
        pthread_mutex_unlock(&lock_vnodes);
        for(id=init;(id<(init+NPERTHREAD_V)) && (id<nvnodes);id++){
            vnode = &vnodes[index_vnodes[id]];
            if(mode == 0)
                update_vnode(vnode);
            else
                update_vnode_information(vnode);
        }
        pthread_mutex_lock(&lock_vnodes);
    }
    pthread_mutex_unlock(&lock_vnodes);

    pthread_exit(NULL);
    return NULL;
}
void* thread_fnodes(void *in){
    uint32_t *lim;
    uint32_t nvnodes,nfnodes;
    lim = (uint32_t *)in;
    nvnodes = lim[0];
    nfnodes = lim[1];

    uint32_t id,init;
    pthread_mutex_lock(&lock_fnodes);
    while(cnt_fnodes<nfnodes){
        init = cnt_fnodes;
        cnt_fnodes += NPERTHREAD_F;
        pthread_mutex_unlock(&lock_fnodes);
        for(id=init;(id<(init+NPERTHREAD_F)) && (id<nfnodes);id++){
            if(mode == 0)
                update_fnode(&fnodes[index_fnodes[id]]);
            else
                update_fnode_information(&fnodes[id]);

        }
        pthread_mutex_lock(&lock_fnodes);
    }
    pthread_mutex_unlock(&lock_fnodes);

    pthread_exit(NULL);
    return NULL;
}
void run_bp(Vnode * vnodes_i,
            Fnode * fnodes_i,
            uint32_t nk,
            uint32_t nvnodes,
            uint32_t nfnodes,
            uint32_t it_c,
            uint32_t nthread,
            uint32_t m,
            uint32_t *t){

    int i,j;
    uint32_t it,lim[2];
    Nk = nk;
    mode = m;
    tab = t;
    vnodes = vnodes_i;
    fnodes = fnodes_i;
    NPERTHREAD_V = max(10,nvnodes / (nthread*10));
    NPERTHREAD_F = max(10,nfnodes / (nthread*10));
    lim[0] = nvnodes;
    lim[1] = nfnodes;
    pthread_t threads[nthread];
    pthread_mutex_init(&lock_vnodes,NULL);
    pthread_mutex_init(&lock_fnodes,NULL);
    index_vnodes = (uint32_t *) malloc(sizeof(uint32_t)*nvnodes);
    for(i=0;i<nvnodes;i++)
        index_vnodes[i] = i;
    index_fnodes = (uint32_t *) malloc(sizeof(uint32_t)*nfnodes);
    for(i=0;i<nfnodes;i++){
        index_fnodes[i] = i;
    }
    
    // update vnodes
    shuffle(index_vnodes,nvnodes);
    for(j=0;j<nthread;j++){
        pthread_create(&threads[j],NULL,thread_vnodes_reset,(void*)lim);
    }
    for(j=0;j<nthread;j++){
        pthread_join(threads[j],NULL);
    }

    for(i=0;i<it_c;i++){
        // update fnodes
        cnt_fnodes = 0;
        cnt_vnodes = 0;
        shuffle(index_fnodes,nfnodes);
        for(j=0;j<nthread;j++){
            pthread_create(&threads[j],NULL,thread_fnodes,(void*)lim);
        }
        for(j=0;j<nthread;j++){
            pthread_join(threads[j],NULL);
        }
        // update vnodes
        shuffle(index_vnodes,nvnodes);
        for(j=0;j<nthread;j++){
            pthread_create(&threads[j],NULL,thread_vnodes,(void*)lim);
        }
        for(j=0;j<nthread;j++){
            pthread_join(threads[j],NULL);
        }
    }

    // destroy mutex
    pthread_mutex_destroy(&lock_fnodes);
    pthread_mutex_destroy(&lock_vnodes);

    return;
}
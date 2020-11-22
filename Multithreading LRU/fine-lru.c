/* -*- mode:c; c-file-style:"k&r"; c-basic-offset: 4; tab-width:4; indent-tabs-mode:nil; mode:auto-fill; fill-column:78; -*- */
/* vim: set ts=4 sw=4 et tw=78 fo=cqt wm=0: */

/* @* Place your name here, and any other comments *@
 * @* that deanonymize your work inside this syntax *@
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "lru.h"

/* Define the simple, singly-linked list we are going to use for tracking lru */
struct list_node {
    struct list_node* next;
    int key;
    int refcount;
    // Protects this node's contents
    pthread_mutex_t mutex;
};

static struct list_node* list_head = NULL;

/* A static mutex; protects the count and head.
 * XXX: We will have to tolerate some lag in updating the count to avoid
 * deadlock. */
static pthread_mutex_t mutex;
static int count = 0;
static pthread_cond_t cv_low, cv_high;

static volatile int done = 0;

/* Initialize the mutex. */
int init (int numthreads) {
    /* Your code here */
    int rv = 0;
    rv = pthread_mutex_init(&mutex, NULL);
    if(rv != 0) return rv;
    rv = pthread_cond_init(&cv_low, NULL);
    if(rv != 0) return rv;
    rv = pthread_cond_init(&cv_high,NULL);
    return rv;
}

/* Return 1 on success, 0 on failure.
 * Should set the reference count up by one if found; add if not.*/
int reference (int key) {
    /* Your code here */
    pthread_mutex_lock(&mutex);
    while(count >= HIGH_WATER_MARK && done != 1){
        pthread_cond_wait(&cv_high, &mutex); 
    }
    
    if(done == 1){
        pthread_cond_signal(&cv_high);
        pthread_cond_signal(&cv_low);   
        pthread_mutex_unlock(&mutex);
        return 1;
    }

    int found = 0;
    int add = 0;
    int unlocked = 0;
    struct list_node* cursor = list_head;
    struct list_node* last = NULL;

    // if list is not empty, lock cursor
    if(cursor){
        pthread_mutex_lock(&cursor->mutex);
    }

    while(cursor) {
        if (cursor->key < key) {
            // if last is not null, unlock last node
            if(last){
                pthread_mutex_unlock(&last->mutex);
                //if we passed list head
                if(!unlocked&&last!=list_head){
                    pthread_mutex_unlock(&mutex);
                    unlocked++;
                }
            }
            last = cursor;
            cursor = cursor->next;
            // lock the current lock
            if(cursor){
                pthread_mutex_lock(&cursor->mutex);
            }
        } else {
            if (cursor->key == key) {
                cursor->refcount++;
                found++;
            }
            break;
        }
    }
    // until then, we have the lock for the last and current node

    if (!found) {
        // Handle 2 cases: the list is empty/we are trying to put this at the
        // front, and we want to insert somewhere in the middle or end of the
        // list.
        struct list_node* new_node = malloc(sizeof(struct list_node));
        if (!new_node) return 0;
        add++;                              // for future count increment

        new_node->key = key;
        new_node->refcount = 1;
        new_node->next = cursor;

        // initialize the node mutex
        pthread_mutex_init(&new_node->mutex, NULL);

        if (last == NULL){
            list_head = new_node;
            if(!unlocked){
                pthread_mutex_unlock(&mutex);
                unlocked++;
            }
        }else{
            last->next = new_node;
        }      
    }

    if(last){
        pthread_mutex_unlock(&last->mutex);
    }
    if(cursor){
        pthread_mutex_unlock(&cursor->mutex);
    }
   
    if(unlocked){
        pthread_mutex_lock(&mutex);
    }

    if(add){
        count++;
    }
    if(count > LOW_WATER_MARK){
        pthread_cond_signal(&cv_low); 
    }
    pthread_mutex_unlock(&mutex);
    return 1;
   
}

/* Do a pass through all elements, either decrement the reference count,
 * or remove if it hasn't been referenced since last cleaning pass.
 *
 * check_water_mark: If 1, block until there are more elements in the cache
 * than the LOW_WATER_MARK.  This should only be 0 during self-testing or in
 * single-threaded mode.
 */
void clean(int check_water_mark) {
    /* Your code here */
    pthread_mutex_lock(&mutex);

    if(check_water_mark == 1){
        while(count <= LOW_WATER_MARK && done != 1){
            pthread_cond_wait(&cv_low, &mutex);
        }
    }

    if(done == 1){
        pthread_cond_signal(&cv_high);
        pthread_cond_signal(&cv_low);   
        pthread_mutex_unlock(&mutex);
        return;
    }
   
    struct list_node* cursor = list_head;
    struct list_node* last = NULL;
    int unlocked=0;
    int removedCount=0;

    // if list is not empty, lock cursor
    if(cursor){
        pthread_mutex_lock(&cursor->mutex);
    }

    while(cursor) {
        cursor->refcount--;
        if (cursor->refcount == 0) {
            struct list_node* tmp = cursor;
            if (last) {
                last->next = cursor->next;
                
                if(unlocked == 0 && last != list_head){
                    pthread_mutex_unlock(&mutex);
                    unlocked++;
                }
            } else {
                // pthread_mutex_lock(&mutex);
                list_head = cursor->next;
                // pthread_mutex_unlock(&mutex);
            }
            
            tmp = cursor->next;
            free(cursor);
            cursor = tmp;
            if(cursor){
                pthread_mutex_lock(&cursor->mutex);
            }
            // update count    
            removedCount++;
        } else {
            if(last){
                pthread_mutex_unlock(&last->mutex);
            }
            last = cursor;
            cursor = cursor->next;
            if(cursor){
                pthread_mutex_lock(&cursor->mutex);
            }
            
        }
    }

    if(last){
        pthread_mutex_unlock(&last->mutex);
    }
    if(cursor){
        pthread_mutex_unlock(&cursor->mutex);
    }

    if(unlocked){
        pthread_mutex_lock(&mutex);
    }
    count = count - removedCount;
    if(count < HIGH_WATER_MARK){
        pthread_cond_signal(&cv_high); 
    }
    pthread_mutex_unlock(&mutex);
}


/* Optional shut-down routine to wake up blocked threads.
   May not be required. */
void shutdown_threads (void) {
    /* Your code here */
    pthread_mutex_lock(&mutex);
    done = 1;
    pthread_cond_signal(&cv_high);
    pthread_cond_signal(&cv_low);   
    pthread_mutex_unlock(&mutex);
    return;
}

/* Print the contents of the list.  Mostly useful for debugging. */
void print (void) {
    /* Your code here */  
    int unlocked = 0; 

    pthread_mutex_lock(&mutex);
    printf("=== Starting list print ===\n");
    printf("=== Total count is %d ===\n", count);
    struct list_node* cursor = list_head;
    
    if(cursor){
        pthread_mutex_lock(&cursor->mutex);
    }

    while(cursor) {
        printf ("Key %d, Ref Count %d\n", cursor->key, cursor->refcount);
        if(cursor->next){
            pthread_mutex_lock(&cursor->next->mutex);
        }
        pthread_mutex_unlock(&cursor->mutex);
        cursor = cursor->next;
        if(!unlocked){
            pthread_mutex_unlock(&mutex);
            unlocked++;
        }
    }
    if(!unlocked){
        pthread_mutex_unlock(&mutex);
    }

    if(cursor){
        pthread_mutex_unlock(&cursor->mutex);
    }   
    

    printf("=== Ending list print ===\n");
    

}

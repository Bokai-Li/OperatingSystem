# Locking Protocol

## Summary:

Global mutex lock is used to protect the access and modification of global variables which include "count" and "list_head”, as well as the conditional variables “cv_high” and “cv_low” and the volatile variable "done".

Each list_node mutex lock is used to protect the access and modification of that specific node.

During the traversal in reference and clean, once the "last" node goes past the "list_head" node, it will release the global mutex lock because "list_head" will not be changed after that point and it is safe for other threads to access and modify. At this stage, we applied the hand over hand locking which allows the traversal thread to have at most 2 consecutive node locks at the same time. Because we used hand over hand locking, there is no need to obtain cursor->next->mutex first before assigning cursor to next because the thread should always have last one locked. That means that no other traversal thread can pass that thread in the traversal stage until the thread with more progress finishes. (Note: one exception is print which only locks the current node during traversal, the cursor->next lock need to be obtained before moving the cursor to the next node)

At the end of the traversal, the thread will release all node locks before trying to obtain the global mutex. The count will be updated after obtaining the global mutex at the end.


### reference
For reference traversal, the thread obtains the global lock in the beginning and starts the traversal after the watermark condition is checked. During traversal, it will lock the cursor first if it is not null and there are few cases that can happen:

1. It wants to reference list_head, or the cache is empty, then the global mutex needs to be held to the end of the function.
2. It wants to reference the second node, then the global mutex needs to be held to the end as well.
3. It wants to reference a node with a key between list_head and the second node, then the global mutex needs to be held to the end. Since the insert operation will modify list_head->next.
4. It wants to reference anything after the third node, or node not found in the cache, then we will release the global mutex immediately after last passes list_head, that is, being node2, because reference will not touch or modify the list_head after this stage.

node1(List_head) -> node2 -> node3 -> node4  

                      ^        ^  

                     last    cursor

Then it uses hand over hand locking for the rest of the traversal. It will initialize the node lock after the creation of the new node in reference.

Lastly, we will release all the node locks, specifically, the mutex for last and the mutex for cursor if any of them were obtained during traversal. Then obtain the global mutex again to update the count if needed.

Note: unlocked int is a flag used to avoid unlocking global mutex twice.


### clean
For clean traversal, we obtain the global lock in the beginning and start the traversal after the watermark condition is checked. During the traversal, it will lock the cursor first if it is not null and there are few cases that can happen:

1. If list_head or list_head->next needs to be removed, the global mutex will be held until the end of the function.
2. If the traversal passes the third node or nothing to be cleaned, then we will release the global mutex immediately after the "last" passes "list_head", that is, last points to node2, because after that point, clean will not touch or modify the list_head after this stage.


node1(list_head) -> node2 -> node3 -> node4

                      ^        ^

                     last    cursor

Then we used hand over hand locking for the rest of the traversal. We will initialize the node lock after the creation of the node in reference.

Lastly, we will release all the node locks, specifically, the mutex for last and the mutex for cursor if any of them are obtained during traversal. Then obtain the global mutex again to update the count if needed.

Note: unlocked int is used to avoid unlocking global mutex twice.


### shutdown
We first grab the global mutex since we need to modify "done". Then we change the value of "done" to be 1. Then we notify one waiting thread that is waiting for cv_high and one waiting thread that is waiting for cv_low. Finally, we release the global mutex.

In both reference and clean, after checking the watermark, we need to check if done == 1. If so, we want to do the same thing that is to notify one waiting thread that is waiting for cv_high and one waiting thread that is waiting for cv_low, just making sure that every action (reference or clean) will wake up at least one thread. 

### print
Print is the only exception that doesn't use hand over hand locking during traversal because we only need one node at a time. In this case, the cursor->next lock need to be obtained before updating the cursor to its next node in case the next node is modified between asigning and locking in print.

# test
We added three additional unit tests. 

### self_tests1
Instead of deterministically reference the list, the test references random keys 64 times, and clean only once. This test is used to make sure count is working in whatever cases. 

### self_tests2
This test is used to test if the conditional variable "cv_high" works properly. The test first references 96 keys from 0 to 95, resulting in a count value of 96, which is equal to the HIGH_WATER_MARK. Then it references key 96. If the conditional variable works, this thread will be hang, waiting for a notify.

### self_tests3
This test is used to test if the conditional variable "cv_low" works properly. The test first references 32 keys from 0 to 31, resulting in a count value of 32, which is equal to the LOW_WATER_MARK. Then it cleans by passing the parameter 1. If the conditional variable works, this thread will be hang, waiting for a notify.

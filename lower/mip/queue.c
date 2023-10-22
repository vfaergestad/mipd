#include <stdlib.h>
#include "queue.h"

// Define a node structure for linked list implementation
struct node {
    u_int8_t mip;
    struct mip_pdu pdu;
    struct node *next;
};

// Head of the linked list (queue)
static struct node *head = NULL;

/**
 * Used to initialize the mip_queue. 
 *
 * It sets the head of the mip_queue to NULL.
 *
 * @return: 0 after successfully initializing the mip_queue.
 * 
 */
int init_queue(void) {
    head = NULL; // Initialize the head to NULL
    return 0;
}

/**
 * Used to print the elements of the mip_queue to the debug output. It is a helpful
 * utility used for debugging purposes to check the current state of the mip_queue.
 *
 * It iterates through each node in the queue starting from the head and prints the mip value
 * of each node to the debug output.
 *
 * @return: void
 *
 */
void print_queue_to_debug(void) {
    struct node *temp = head;
    global_debug("Printing mip-queue");
    while (temp != NULL) {
        global_debug("MIP: %d\n", temp->mip);
        temp = temp->next;
    }
}

/**
 * Used to destroy the mip_queue by deallocating the memory allocated 
 * to each node of the mip_queue.
 *
 * It iterates through each node in the queue starting from the head, stores the reference to 
 * the current node, moves the head pointer to the next node and deallocates the memory of 
 * the current node using the free() function. It continues this process until the mip_queue is empty
 * (i.e., head is NULL).
 *
 * @return: 0 after successfully destroying the mip_queue.
 *
 */
int destroy_queue(void) {
    struct node *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
    return 0;
}

/**
 * Used to add a new mip_pdu to the end of the mip_queue.
 *
 * @param pdu: Pointer to the mip_pdu that should be added to the queue.
 *
 * A new node is created and allocated memory. The mip and pdu values of the new node are set 
 * to the destination address and pdu of the passed mip_pdu. The new node is then added 
 * to the end of the mip_queue. If the queue was empty, the created node becomes the head of the queue.
 * In debug-mode, the queue is printed after adding the mip_pdu.
 *
 * @return: 0 if the mip_pdu is successfully added to the queue;
 * 1 if memory allocation fails while creating a new node.
 *
 */
int enqueue_mip_pdu(struct mip_pdu *pdu) {
    struct node *new_node = malloc(sizeof(struct node));
    if (new_node == NULL) {
        return 1; // Memory allocation failed
    }
    new_node->mip = pdu->dest_addr;
    new_node->pdu = *pdu;
    new_node->next = NULL;

    if (head == NULL) {
        head = new_node;
    } else {
        struct node *temp = head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
    //print_queue_to_debug();
    return 0;
}

/**
 * Used to remove and return a mip_pdu from the mip_queue based on the passed mip argument.
 *
 * @param mip: The mip value of the mip_pdu that should be removed from the queue.
 *
 * It iterates through each node in the mip_queue. If a node with matching mip value is found, it removes 
 * the node from the queue by adjusting the next pointers of surrounding nodes and frees the memory allocated 
 * for that node. The mip_pdu of that node is returned.
 *
 * @return: Pointer to the mip_pdu if a node with matching mip value is found;
 * NULL if no matching node is found in the mip_queue.
 *
 */
struct mip_pdu *dequeue_mip_pdu(u_int8_t mip) {
    struct node *temp = head, *prev = NULL;
    while (temp != NULL) {
        if (temp->mip == mip) {
            if (prev == NULL) {
                head = temp->next;
            } else {
                prev->next = temp->next;
            }
            struct mip_pdu *pdu = malloc(sizeof(struct mip_pdu));
            *pdu = temp->pdu;
            free(temp);
            return pdu;
        }
        prev = temp;
        temp = temp->next;
    }
    //print_queue_to_debug();
    return NULL; // mip not found
}

/**
 * This function is used to view a mip_pdu from the mip_queue based on the passed mip argument.
 *
 * @param mip: The mip value of the mip_pdu that should be viewed (peeked) in the queue.
 *
 * It iterates through each node in the mip_queue. If a node with matching mip value is found,
 * a copy of its mip_pdu is created and returned, but the node remains in the mip_queue.
 *
 * @return: Pointer to the mip_pdu if a node with matching mip value
*/
struct mip_pdu *peek_mip_pdu(u_int8_t mip) {
    struct node *temp = head;
    while (temp != NULL) {
        if (temp->mip == mip) {
            struct mip_pdu *pdu = malloc(sizeof(struct mip_pdu));
            *pdu = temp->pdu;
            return pdu;
        }
        temp = temp->next;
    }
    return NULL; // mip not found
}

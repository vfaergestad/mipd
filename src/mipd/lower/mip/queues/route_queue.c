#include "route_queue.h"

/**
 * Initializes the routing queue used for managing route-related requests.
 *
 * This setup is necessary before using the queue for storing and managing routing information requests.
 *
 * Global variables:
 * - route_queue: A global structure representing the routing queue.
 *
 * The function does not take any parameters and does not return any value. It should be called before the routing queue
 * is used for the first time to ensure that it is in a consistent and predictable state.
 */
void init_route_queue() {
    route_queue.front = NULL;
    route_queue.rear = NULL;
    route_queue.size = 0;
}

/**
 * Prints the contents of the routing queue for debugging purposes.
 *
 * Global variables:
 * - route_queue: A global structure representing the routing queue. The function accesses its 'front' member to
 *                start the iteration through the queue.
 *
 * The function does not take any parameters and does not return any value. It should be used for diagnostic purposes
 * to visualize the contents of the routing queue at a given point in time.
 */
void print_queue() {
    struct queue_node *temp = route_queue.front;
    global_debug("Printing route-queue");
    while (temp != NULL) {
        global_debug("MIP: %d\n", temp->pdu.dest_addr);
        temp = temp->next;
    }
}

/**
 * Adds a new MIP PDU to the routing queue.
 *
 * @param pdu: The MIP PDU to be enqueued into the routing queue.
 *
 * Global variables:
 * - route_queue: A global structure representing the routing queue, which is modified by adding the new node.
 *
 * @return: Returns 0 on successful enqueue of the PDU; returns -1 if memory allocation for the new node fails.
 *
 */
int route_enqueue(struct mip_pdu pdu) {
    struct queue_node *new_node = (struct queue_node *)malloc(sizeof(struct queue_node));
    if (!new_node) {
        return -1;
    }

    new_node->pdu = pdu;
    new_node->next = NULL;

    if (route_queue.rear == NULL) {
        route_queue.front = route_queue.rear = new_node;
    } else {
        route_queue.rear->next = new_node;
        route_queue.rear = new_node;
    }

    route_queue.size++;
    print_queue();
    return 0;
}

/**
 * Removes and returns the front MIP PDU from the routing queue.
 *
 * Global variables:
 * - route_queue: A global structure representing the routing queue, modified by removing the front node.
 *
 * @return: The MIP PDU that was at the front of the queue.
 *
 * The function is crucial for processing PDUs in a FIFO (First In, First Out) manner within the routing mechanism, ensuring
 * that the PDUs are processed in the order they were received.
 */
struct mip_pdu route_dequeue() {
    print_queue();
    struct queue_node *temp = route_queue.front;
    struct mip_pdu pdu = temp->pdu;

    route_queue.front = route_queue.front->next;
    if (route_queue.front == NULL) {
        route_queue.rear = NULL;
    }

    free(temp);
    route_queue.size--;
    return pdu;
}

/**
 * Determines whether the routing queue is empty.
 *
 * Global variables:
 * - route_queue: A global structure representing the routing queue. The function checks its 'front' member to ascertain emptiness.
 *
 * @return: Returns 1 if the routing queue is empty (i.e., 'front' is NULL); returns 0 if there are elements in the queue.
 */
int is_route_queue_empty() {
    global_debug("Checking if route_queue is empty");
    global_debug("route_queue.size: %d", route_queue.size);
    global_debug("route_queue.front: %d", route_queue.front);
    if (route_queue.front == NULL) {
        return 1;
    }
    return 0;
}

/**
 * Clears all elements from the routing queue.
 *
 * Global variables:
 * - route_queue: A global structure representing the routing queue. It's modified by the route_dequeue() function calls.
 */
void free_route_queue() {
    while (is_route_queue_empty() == 0) {
        route_dequeue();
    }
}

